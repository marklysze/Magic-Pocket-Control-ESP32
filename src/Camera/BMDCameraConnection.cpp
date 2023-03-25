#include "BMDCameraConnection.h"

static const std::string CODEAPPNAME ="BMDCameraESP32";

BMDCameraConnection::BMDCameraConnection() {}

BMDCameraConnection::~BMDCameraConnection()
{
  // delete _cameraStatus, _deviceName, _timecode, _outgoingCameraControl, _incomingCameraControl, _device;
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

    bleDevice.init("MarkHello");
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
    bleScan = bleDevice.getScan();
    bleScan->setAdvertisedDeviceCallbacks(new ScanAdvertisedDeviceCallbacks());
    bleScan->setInterval(1349);
    bleScan->setWindow(449);
    bleScan->setActiveScan(false);

    Serial.println("Scan starting (5 seconds).");
    bleScan->start(5);

    return true;
}

BMDCamera *BMDCameraConnection::connect()
{
    status = ConnectionStatus::Disconnected;

    if(!ScanAdvertisedDeviceCallbacks::cameraAddresses.empty())
    {
        Serial.print("Cameras found: ");
        Serial.println(ScanAdvertisedDeviceCallbacks::cameraAddresses.size());

        // Create the client (this device)
        bleClient = bleDevice.createClient();

        if(bleClient == nullptr)
        {
            Serial.println("Failed to create Client");
            return nullptr;
        }

        // Handle Connect/Disconnect call backs and pass this object in so we can update status
        bleClient->setClientCallbacks(new BMDBLEClientCallback(this));

        Serial.println("Created Bluetooth client and associated connect/disconnect call backs");

        // Connect to the first BLE Server (Camera)
        status = ConnectionStatus::Connecting;
        bool connectedToCamera = bleClient->connect(ScanAdvertisedDeviceCallbacks::cameraAddresses.front());  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)

        if(connectedToCamera)
        {
            Serial.println("Connected to Bluetooth server (camera)");
        }
        else
        {
            Serial.println("Unable to connect to camera.");
            status = ConnectionStatus::Disconnected;
            return nullptr;
        }

        // Obtain a reference to the service we are after in the remote BLE server
        bleRemoteService = bleClient->getService(Constants::UUID_BMD_BCS);
        if (bleRemoteService == nullptr)
        {
            // Serial.print("Failed to find our service UUID: ");
            // Serial.println(BmdCameraService.toString().c_str());
            bleClient->disconnect();
            return nullptr;
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
            return nullptr;
        }
        else
        {
            // Connect to the notifications from the characteristic
            bleChar_IncomingCameraControl->registerForNotify(IncomingCameraControlNotify, false);

            Serial.println("Connected to Incoming Camera Control Characteristic");
        }

        status = ConnectionStatus::Connected;
    }
    else
        Serial.print("No cameras found in scan.");

    return nullptr; // Unsuccessful.
}

void BMDCameraConnection::disconnect()
{
    status = ConnectionStatus::Disconnected;

    if(bleClient->isConnected())
        bleClient->disconnect();

    delete camera;
    camera = nullptr;

    Serial.println("Disconnect called.");

}

/*
BMDCamera* BMDCameraConnection::getCamera()
{
    return camera;
}
*/

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