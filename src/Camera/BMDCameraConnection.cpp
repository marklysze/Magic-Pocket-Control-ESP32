#include "BMDCameraConnection.h"

// Update this to what you would like shown on the back of the camera
const std::string BMDCameraConnection::CODEAPPNAME ="Magic Pocket Control";

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

  if(bleChar_Timecode != nullptr)
    bleChar_Timecode->registerForNotify(NULL, false); // Clear notification (housekeeping)

  if(bleChar_Timecode != nullptr)
    bleChar_Timecode->registerForNotify(NULL, false); // Clear notification (housekeeping)

  delete bleChar_IncomingCameraControl;
  delete bleChar_OutgoingCameraControl;
  delete bleChar_DeviceName;
  delete bleChar_Timecode;
  delete bleChar_ProtocolVersion;

  initialised = false;
  bleDevice.deinit(true);
}

// Initialise and use Serial security
void BMDCameraConnection::initialise()
{
    if(initialised)
        return;

    appName = CODEAPPNAME;

    bleDevice.init("MPC");
    bleDevice.setPower(ESP_PWR_LVL_P9);
    bleDevice.setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);

    SerialSecurityHandler* securityHandler = new SerialSecurityHandler(this);
    bleDevice.setSecurityCallbacks(securityHandler);

    bleSecurity = new BLESecurity();
    bleSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
    bleSecurity->setCapability(ESP_IO_CAP_IN);
    bleSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

    status = ConnectionStatus::Disconnected;
    // disconnect();

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
            disconnect();
            // status = ConnectionStatus::Disconnected;
            return;
        }

        // Handle Connect/Disconnect call backs and pass this object in so we can update status
        bleClient->setClientCallbacks(new BMDBLEClientCallback(this));

        delay(500);

        DEBUG_VERBOSE("Created Bluetooth client and associated connect/disconnect call backs");

        // Connect to the first BLE Server (Camera)
        status = ConnectionStatus::Connecting;
        DEBUG_DEBUG("connect: Connection Status set to %i", status);
        bool connectedToCamera = bleClient->connect(cameraAddress); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)

        if(!connectedToCamera)
        {
            DEBUG_ERROR("Unable to connect to camera.");
            disconnect();
            // status = ConnectionStatus::Disconnected;
            return;
        }

        // Obtain a reference to the service we are after in the remote BLE server
        bleRemoteService = bleClient->getService(Constants::UUID_BMD_BCS);
        if (bleRemoteService == nullptr)
        {
            // Serial.print("Failed to find our service UUID: ");
            // Serial.println(BmdCameraService.toString().c_str());
            // bleClient->disconnect();
            disconnect();
            // status = ConnectionStatus::Disconnected;
            return;
        }
        else
            DEBUG_VERBOSE("Connected to Blackmagic Camera Service");

        // Check the Protocol Version to make sure it's compatible
        bleChar_ProtocolVersion = bleRemoteService->getCharacteristic(Constants::UUID_BMD_BCS_PROTOCOL_VERSION);
        if(bleChar_ProtocolVersion != nullptr)
        {
            std::string cameraProtocolVersion = bleChar_ProtocolVersion->readValue();

            std::vector<int> versionNumbers = ProtocolVersionNumber::ConvertVersionStringToInts(cameraProtocolVersion.c_str());
            if(!ProtocolVersionNumber::CompatibilityVerified(versionNumbers[0], versionNumbers[1], versionNumbers[2]))
            {
                DEBUG_ERROR("Camera Protocol Version is incompatible, aborting: %s", cameraProtocolVersion.c_str());
                disconnect();
                status = ConnectionStatus::IncompatibleProtocol;
                return;
            }
        }
        else
        {
            DEBUG_ERROR("Could not access Protocol Version Characteristic, aborting.");
            disconnect();
            return;
        }

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
            DEBUG_ERROR("Could not connect to Incoming Camera Control Characteristic");
            disconnect();
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
            DEBUG_ERROR("Could not connect to Outgoing Camera Control Characteristic");
            disconnect();
            return;
        }
        else
            DEBUG_VERBOSE("Got Outgoing Camera Control Characteristic");

        // Subscribe to Incoming Camera Control messages (messages from the camera)
        bleChar_Timecode = bleRemoteService->getCharacteristic(Constants::UUID_BMD_BCS_TIMECODE);
        if (bleChar_Timecode == nullptr)
        {
            disconnect();
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
        disconnect();
    }
}

void BMDCameraConnection::disconnect()
{
    // Clear known cameras
    cameraAddresses.clear();

    if(bleClient->isConnected())
        bleClient->disconnect();

    if(BMDControlSystem::getInstance()->hasCamera())
        BMDControlSystem::getInstance()->deactivateCamera();

    status = ConnectionStatus::Disconnected;
}

void BMDCameraConnection::sendCommandToOutgoing(CCUPacketTypes::Command command, bool response = true)
{
    std::vector<byte> data = command.serialize();

    bleChar_OutgoingCameraControl->writeValue(data.data(), data.size(), response);
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