#include "BMDCameraConnection.h"

// Update this to what you would like shown on the back of the camera
const std::string BMDCameraConnection::CODEAPPNAME ="Magic Pocket Control";

// BMD's Connection Status variable (primarily for consistency, we use our own connection status variable)
byte BMDCameraConnection::bmdConnectionStatus = 0;

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
  delete bleChar_CameraStatus;

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

                DEBUG_VERBOSE("Blackmagic Camera found %s", device.getAddress().toString().c_str());
            }
        }
    }

    if(!instance->cameraAddresses.empty())
    {
        instance->status = ConnectionStatus::ScanningFound;
    }
    else
        instance->status = ConnectionStatus::ScanningNoneFound;
}

// Clears BLE bonding, mainly for testing pass key connections: https://icircuit.net/esp-idf-bluetooth-remove-bonded-devices/3040
void BMDCameraConnection::clearBondedDevices()
{
    int dev_num = esp_ble_get_bond_device_num();
    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    esp_ble_get_bond_device_list(&dev_num, dev_list);
    for (int i = 0; i < dev_num; i++) {
        esp_ble_remove_bond_device(dev_list[i].bd_addr);
    }

    free(dev_list);
}

// Have we got a bond to the camera address on the BLE device?
bool BMDCameraConnection::isCameraBonded(BLEAddress cameraAddress)
{
    int dev_num = esp_ble_get_bond_device_num();

    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);

    esp_ble_get_bond_device_list(&dev_num, dev_list);

    bool returnValue = false;

    for (int i = 0; i < dev_num; i++)
    {  
        BLEAddress bleBondedAddress(dev_list[i].bd_addr);

        if(bleBondedAddress == cameraAddress)
        {
            DEBUG_VERBOSE("Have previously bonded to this camera.");

            returnValue = true;
            break;
        }
    }

    free(dev_list);
    return returnValue;
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
            disconnect();
            return;
        }

        // Obtain a reference to the service we are after in the remote BLE server
        bleRemoteService = bleClient->getService(Constants::UUID_BMD_BCS);
        if (bleRemoteService == nullptr)
        {
            disconnect();
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

        // Subscribe to Camera Status messages
        bleChar_CameraStatus = bleRemoteService->getCharacteristic(Constants::UUID_BMD_BCS_CAMERA_STATUS);
        if (bleChar_CameraStatus == nullptr)
        {
            DEBUG_ERROR("Could not connect to Camera Status Characteristic");
            disconnect();
            return;
        }
        else
        {
            // Connect to the notifications from the characteristic
            bleChar_CameraStatus->registerForNotify(IncomingCameraStatusNotify, true);

            DEBUG_VERBOSE("Connected to Incoming Camera Status Characteristic");
        }

        // Check if we failed the pass key entry and return if so.
        if(status == ConnectionStatus::FailedPassKey)
        {
            DEBUG_VERBOSE("Failed Pass Key, Disconnecting");
            bleClient->disconnect();
            return;
        }

        // Create Camera
        BMDControlSystem::getInstance()->activateCamera();

        status = ConnectionStatus::Connected;

        bmdConnectionStatus |= ConnectionStatusFlags::kConnected;
        bmdConnectionStatus |= ConnectionStatusFlags::kPaired;

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

    bmdConnectionStatus = ConnectionStatusFlags::kNone;
    initialPayloadTime = ULONG_MAX;
}

void BMDCameraConnection::sendCommandToOutgoing(CCUPacketTypes::Command command, bool response)
{
    std::vector<byte> data = command.serialize();

    bleChar_OutgoingCameraControl->writeValue(data.data(), data.size(), response);
}

// Primarily for testing, sends a byte array rather than a formulated and validated command
void BMDCameraConnection::sendBytesToOutgoing(std::vector<byte> data, bool response)
{
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

// Incoming Camera Status - primarily using for consistency with BMD's code
void BMDCameraConnection::IncomingCameraStatusNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    std::vector<byte> data(pData, pData + length);
    byte cameraStatus = CameraStatus::GetCameraStatusFlags(data);

    // Check camera status flags
    bool cameraIsOn = (cameraStatus & CameraStatus::Flags::CameraPowerFlag) != 0;
    bool cameraIsReady = (cameraStatus & CameraStatus::Flags::CameraReadyFlag) != 0;
    bool cameraWasReady = (bmdConnectionStatus & CameraStatus::Flags::CameraReadyFlag) != 0;
    bool alreadyReceivedInitalPayload = (bmdConnectionStatus & ConnectionStatusFlags::kInitialPayloadReceived) != 0;

    if(cameraIsReady)
    {
        if(!alreadyReceivedInitalPayload)
        {
            DEBUG_VERBOSE("IncomingCameraStatusNotify, Initial Payload Received.");

            // Access the class instance using the static member variable and set the initial payload as received
            BMDCameraConnection* instance = BMDCameraConnection::instancePtr;
            instance->initialPayloadTime = millis();
        }
        
        if(!cameraWasReady)
            DEBUG_VERBOSE("IncomingCameraStatusNotify, Camera Ready.");

        bmdConnectionStatus |= ConnectionStatusFlags::kInitialPayloadReceived;
        bmdConnectionStatus |= ConnectionStatusFlags::kCameraReady;
        bmdConnectionStatus |= ConnectionStatusFlags::kPower;
    }
    else if(cameraIsOn)
    {
        DEBUG_VERBOSE("IncomingCameraStatusNotify, Camera On.");
        bmdConnectionStatus |= ConnectionStatusFlags::kPower;
    }
    else
    {
        DEBUG_VERBOSE("IncomingCameraStatusNotify, Camera Off.");
        bmdConnectionStatus &= ~ConnectionStatusFlags::kCameraReady;
        bmdConnectionStatus &= ~ConnectionStatusFlags::kPower;
    }
}
