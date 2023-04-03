#include "BMDCameraConnection.h"

static const std::string CODEAPPNAME ="BMDCameraESP32";

BMDCameraConnection::BMDCameraConnection() {}

BMDCameraConnection::~BMDCameraConnection()
{
  delete bleClient;
  delete bleScan;
  delete bleSecurity;
  delete bleAdvertisedDevice;
  delete bleRemoteService;

  if(bleChar_IncomingCameraControl != nullptr)
    bleChar_IncomingCameraControl->registerForNotify(NULL, false); // Clear notification (housekeeping)
  
  delete bleChar_IncomingCameraControl;
  delete bleChar_OutgoingCameraControl;
  delete bleChar_DeviceName;
  delete bleChar_Timecode;

  // delete _cameraControl;
  initialised = false;
  bleDevice.deinit(true);
}

// Initialise and use Serial security
void BMDCameraConnection::initialise()
{
    if(initialised)
        return;

    appName = CODEAPPNAME;

    bleDevice.init("BMD Camera");
    bleDevice.setPower(ESP_PWR_LVL_P9);
    bleDevice.setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);

    SerialSecurityHandler* securityHandler = new SerialSecurityHandler(this);
    bleDevice.setSecurityCallbacks(securityHandler);

    // bleDevice.setSecurityCallbacks(new SerialSecurityHandler());

    bleSecurity = new BLESecurity();
    bleSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
    bleSecurity->setCapability(ESP_IO_CAP_IN);
    bleSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

    status = ConnectionStatus::Disconnected;

    initialised = true;
}

void BMDCameraConnection::initialise(TFT_eSprite* windowPtr, TFT_eSprite* spritePassKeyPtr, CST816S* touchPtr, int screenWidth, int screenHeight)
{
    if(initialised)
        return;

    appName = CODEAPPNAME;

    bleDevice.init("BMD Camera");
    bleDevice.setPower(ESP_PWR_LVL_P9);
    bleDevice.setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);

    ScreenSecurityHandler* securityHandler = new ScreenSecurityHandler(this, windowPtr, spritePassKeyPtr, touchPtr, screenWidth, screenHeight);
    bleDevice.setSecurityCallbacks(securityHandler);

    bleSecurity = new BLESecurity();
    bleSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
    bleSecurity->setCapability(ESP_IO_CAP_IN);
    bleSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

    status = ConnectionStatus::Disconnected;

    initialised = true;
}

bool BMDCameraConnection::scan()
{
    status = ConnectionStatus::Scanning;

    bleScan = bleDevice.getScan();
    bleScan->setInterval(1349);
    bleScan->setWindow(449);
    bleScan->setActiveScan(false);

    DEBUG_VERBOSE("Scan starting (5 seconds).");
    bleScan->start(5, BMDCameraConnection::connectCallback, false);

    return true;
}

void BMDCameraConnection::connectCallback(BLEScanResults scanResults) {
    // Access the class instance using the static member variable
    BMDCameraConnection* instance = BMDCameraConnection::instancePtr;

    instance->cameraAddresses.clear();

    for(int i = 0; i < scanResults.getCount(); i++)
    {
        BLEAdvertisedDevice device = scanResults.getDevice(i);

        if (device.haveServiceUUID() && device.isAdvertisingService(Constants::UUID_BMD_BCS))
        {
            // Add the camera address to our list if it doesn't already exist in there
            auto it = std::find(instance->cameraAddresses.begin(), instance->cameraAddresses.end(), device.getAddress());
            if (it == instance->cameraAddresses.end())
            {
                instance->cameraAddresses.push_back(device.getAddress());

                DEBUG_VERBOSE("Blackmagic Camera found %s", device.getAddress().toString());
            }
        }
    }

    if(!instance->cameraAddresses.empty())
        instance->status = ConnectionStatus::ScanningFound;
    else
        instance->status = ConnectionStatus::ScanningNoneFound;
}



void BMDCameraConnection::connect(BLEAddress cameraAddress)
{
    if(!cameraAddresses.empty())
    {
        DEBUG_VERBOSE("Cameras found: %i", cameraAddresses.size());

        // Create the client (this device)
        bleClient = bleDevice.createClient();

        delay(500);

        if(bleClient == nullptr)
        {
            DEBUG_ERROR("Failed to create Client");
            status = ConnectionStatus::Disconnected;
            return;
        }

        // Handle Connect/Disconnect call backs and pass this object in so we can update status
        bleClient->setClientCallbacks(new BMDBLEClientCallback(this));

        delay(500);

        DEBUG_VERBOSE("Created Bluetooth client and associated connect/disconnect call backs");

        // Connect to the first BLE Server (Camera)
        status = ConnectionStatus::Connecting;
        bool connectedToCamera = bleClient->connect(cameraAddress); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)

        if(!connectedToCamera)
        {
            DEBUG_ERROR("Unable to connect to camera.");
            status = ConnectionStatus::Disconnected;
            return;
        }

        // Obtain a reference to the service we are after in the remote BLE server
        bleRemoteService = bleClient->getService(Constants::UUID_BMD_BCS);
        if (bleRemoteService == nullptr)
        {
            // Serial.print("Failed to find our service UUID: ");
            // Serial.println(BmdCameraService.toString().c_str());
            bleClient->disconnect();
            status = ConnectionStatus::Disconnected;
            return;
        }
        else
            DEBUG_VERBOSE("Connected to Blackmagic Camera Service");

        // Subscribe to Device Name to send our device name
        bleChar_DeviceName = bleRemoteService->getCharacteristic(Constants::UUID_BMD_BCS_DEVICE_NAME);
        if (bleChar_DeviceName != nullptr)
        {
            // Write the name we want shown on the camera
            bleChar_DeviceName->writeValue(appName, true);
        }

        // Subscribe to Incoming Camera Control messages (messages from the camera)
        bleChar_IncomingCameraControl = bleRemoteService->getCharacteristic(Constants::UUID_BMD_BCS_INCOMING_CAMERA_CONTROL);
        if (bleChar_IncomingCameraControl == nullptr)
        {
        // Serial.print("Failed to find our characteristic UUID: ");
        // Serial.println(OutgoingCameraControl.toString().c_str());
            bleClient->disconnect();
            status = ConnectionStatus::Disconnected;
            return;
        }
        else
        {
            // Connect to the notifications from the characteristic
            bleChar_IncomingCameraControl->registerForNotify(IncomingCameraControlNotify, false);

            DEBUG_VERBOSE("Connected to Incoming Camera Control Characteristic");
        }

        // Subscribe to Outgoing Camera Control messages (messages to the camera)
        bleChar_OutgoingCameraControl = bleRemoteService->getCharacteristic(Constants::UUID_BMD_BCS_OUTGOING_CAMERA_CONTROL);
        if (bleChar_OutgoingCameraControl == nullptr)
        {
            DEBUG_ERROR("Failed to find our characteristic UUID: %s", Constants::UUID_BMD_BCS_OUTGOING_CAMERA_CONTROL);

            bleClient->disconnect();
            status = ConnectionStatus::Disconnected;
            return;
        }
        else
        {
            DEBUG_VERBOSE("Got Outgoing Camera Control Characteristic");

            /* This demonstrates sending a record command using this outgoing characteristic. From BlueMagic32.
            Serial.println("Trying to hit record.");
            delay(1000);
            uint8_t data[12] = {255, 5, 0, 0, 10, 1, 1, 0, 0, 0, 0, 0};
            data[8] = 2;
            bleChar_OutgoingCameraControl->writeValue(data, 12, true);
            */
        }

        // Subscribe to Incoming Camera Control messages (messages from the camera)
        bleChar_Timecode = bleRemoteService->getCharacteristic(Constants::UUID_BMD_BCS_TIMECODE);
        if (bleChar_Timecode == nullptr)
        {
            bleClient->disconnect();
            status = ConnectionStatus::Disconnected;
            return;
        }
        else
        {
            // Connect to the notifications from the characteristic
            bleChar_Timecode->registerForNotify(IncomingTimecodeNotify, true);

            DEBUG_VERBOSE("Connected to Timecode Characteristic");
        }


        // Create Camera
        BMDControlSystem::getInstance()->activateCamera();

        status = ConnectionStatus::Connected;
        return;
    }
    else
    {
        DEBUG_VERBOSE("No cameras found in scan.");
        status = ConnectionStatus::Disconnected;
    }
}

void BMDCameraConnection::disconnect()
{
    // Clear known cameras
    cameraAddresses.clear();

    status = ConnectionStatus::Disconnected;

    if(bleClient->isConnected())
        bleClient->disconnect();

    BMDControlSystem::getInstance()->deactivateCamera();

    // Serial.println("Disconnect called.");
}

void BMDCameraConnection::sendCommandToOutgoing(CCUPacketTypes::Command command)
{
    std::vector<byte> data = command.serialize();

    /*
    for(int i = 0; i < data.size(); i++)
    {
        Serial.print(data[i]);
        Serial.print(" ");
    }
    */
   
    Serial.println("");

    bleChar_OutgoingCameraControl->writeValue(data.data(), data.size(), true);
}

// Incoming Control Notifications
void BMDCameraConnection::IncomingCameraControlNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    // Must be between 8 and 64 bytes inclusive
    if(length >= 8 && length <= 64)
    {
        // Convert data to vector
        std::vector<byte> data(pData, pData + length);

        // Decode the packet
        // CCUDecodingFunctions::DecodeCCUPacket(pData, length);
        CCUDecodingFunctions::DecodeCCUPacket(data);
    }
    else
        DEBUG_ERROR("Invalid incoming packet length.");
}

// Incoming Timecode
void BMDCameraConnection::IncomingTimecodeNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    // Must be 12 byte
    if(length == 12 ) //>= 8 && length <= 64)
    {
        // Convert data to vector
        std::vector<byte> data(pData, pData + length);

        // We take the last 4 bytes as they contain the timecode values
        std::vector<byte> output(data.end() - 4, data.end());

        // Decode the packet
        CCUDecodingFunctions::TimecodeToString(output);
    }
    else
        DEBUG_ERROR("IncomingTimecodeNotify: Invalid incoming packet length.");
}