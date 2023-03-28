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

  // delete _cameraControl;
  initialised = false;
  bleDevice.deinit(true);
}

void BMDCameraConnection::initialise()
{
    if(initialised)
        return;

    appName = CODEAPPNAME;

    bleDevice.init("BMD Camera");
    bleDevice.setPower(ESP_PWR_LVL_P9);
    bleDevice.setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
    bleDevice.setSecurityCallbacks(new SerialSecurityHandler());

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
    // bleScan->setAdvertisedDeviceCallbacks(new ScanAdvertisedDeviceCallbacks()); // No longer use this class, callback done in scan
    bleScan->setInterval(1349);
    bleScan->setWindow(449);
    bleScan->setActiveScan(false);

    Serial.println("Scan starting (5 seconds).");
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

                Serial.print("Blackmagic Camera found ");
                Serial.println(device.getAddress().toString().c_str());
            }
        }
    }

    if(!instance->cameraAddresses.empty())
        instance->status = ConnectionStatus::ScanningFound;
    else
        instance->status = ConnectionStatus::ScanningNoneFound;
}



void BMDCameraConnection::connect()
{
    if(!cameraAddresses.empty())
    {
        Serial.print("Cameras found: ");
        Serial.println(cameraAddresses.size());

        // Create the client (this device)
        bleClient = bleDevice.createClient();

        delay(500);

        if(bleClient == nullptr)
        {
            Serial.println("Failed to create Client");
            status = ConnectionStatus::Disconnected;
            return;
        }

        // Handle Connect/Disconnect call backs and pass this object in so we can update status
        bleClient->setClientCallbacks(new BMDBLEClientCallback(this));

        delay(500);

        Serial.println("Created Bluetooth client and associated connect/disconnect call backs");

        // Connect to the first BLE Server (Camera)
        status = ConnectionStatus::Connecting;
        bool connectedToCamera = bleClient->connect(cameraAddresses.front()); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)

        if(connectedToCamera)
        {
            Serial.println("Connected to Bluetooth server (camera)");
        }
        else
        {
            Serial.println("Unable to connect to camera.");
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
            Serial.println("Connected to Blackmagic Camera Service");

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

            Serial.println("Connected to Incoming Camera Control Characteristic");
        }

        // Create Camera
        BMDControlSystem::getInstance()->activateCamera();

        status = ConnectionStatus::Connected;
        return;
    }
    else
    {
        Serial.print("No cameras found in scan.");
        status = ConnectionStatus::Disconnected;
    }
}

void BMDCameraConnection::disconnect()
{
    status = ConnectionStatus::Disconnected;

    if(bleClient->isConnected())
        bleClient->disconnect();

    BMDControlSystem::getInstance()->deactivateCamera();

    Serial.println("Disconnect called.");

}

// Incoming Control Notifications
void BMDCameraConnection::IncomingCameraControlNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    // Must be between 8 and 64 bytes inclusive
    if(length >= 8 && length <= 64)
    {
        // Decode the packet
        CCUDecodingFunctions::DecodeCCUPacket(pData, length);
    }
    else
        Serial.println("Invalid incoming packet length.");
}