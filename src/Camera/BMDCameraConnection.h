#ifndef BMDCAMERACONNECTION_H
#define BMDCAMERACONNECTION_H

#include "BLEDevice.h"
#include "Arduino_DebugUtils.h"

// Include screen and screen security handler only if we're using TFT_eSPI
#if USING_TFT_ESPI == 1
    #include <TFT_eSPI.h>
    #include "BLE\ScreenSecurityHandler.h"
#endif

#include "BLE\SerialSecurityHandler.h"
#include "BLE\BMDBLEClientCallback.h"
#include "BMDCamera.h"
#include "CCU\CCUUtility.h"
#include "BMDControlSystem.h"

#include "ESP32\CST816S\CST816S.h"
#include "CCU\CCUDecodingFunctions.h"
#include "Config\Versions.h"

class BMDCameraConnection
{
    public:

        // Update this to what you would like shown on the back of the camera
        static const std::string CODEAPPNAME;

        enum ConnectionStatus
        {
            Disconnected,
            Connected,
            Connecting,
            Scanning,
            ScanningFound,
            ScanningNoneFound,
            NeedPassKey,
            FailedPassKey,
            IncompatibleProtocol
        };

        BMDCameraConnection();
        ~BMDCameraConnection();

        void initialise(); // Serial security pass key

        #if USING_TFT_ESPI == 1

            // Defined in here as wouldn't link when in the cpp file and using the preprocessor directive
            void initialise(TFT_eSprite* windowPtr, TFT_eSprite* spritePassKeyPtr, CST816S* touchPtr, int screenWidth, int screenHeight) // Screen security pass key
            {
                if(initialised)
                    return;

                appName = CODEAPPNAME;

                bleDevice.init("MPC");
                bleDevice.setPower(ESP_PWR_LVL_P9);
                bleDevice.setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);

                ScreenSecurityHandler* securityHandler = new ScreenSecurityHandler(this, windowPtr, spritePassKeyPtr, touchPtr, screenWidth, screenHeight);
                bleDevice.setSecurityCallbacks(securityHandler);

                bleSecurity = new BLESecurity();
                bleSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
                bleSecurity->setCapability(ESP_IO_CAP_IN);
                bleSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

                status = ConnectionStatus::Disconnected;
                // disconnect();

                initialised = true;
            }
            
        #endif
        
        bool scan();
        void connect(BLEAddress cameraAddress);
        void disconnect();
        void sendCommandToOutgoing(CCUPacketTypes::Command command);

        ConnectionStatus status;
        std::vector<BLEAddress> cameraAddresses;

        static void connectCallback(BLEScanResults scanResults);

    private:
        std::string appName;
        bool initialised = false;
        bool scanned = false;

        BLEDevice bleDevice;
        BLEClient* bleClient;
        BLEScan* bleScan;
        BLESecurity* bleSecurity;
        BLEAdvertisedDevice* bleAdvertisedDevice;
        BLERemoteService* bleRemoteService;

        // Characteristics
        BLERemoteCharacteristic* bleChar_IncomingCameraControl;
        BLERemoteCharacteristic* bleChar_OutgoingCameraControl;
        BLERemoteCharacteristic* bleChar_DeviceName;
        BLERemoteCharacteristic* bleChar_Timecode;
        BLERemoteCharacteristic* bleChar_ProtocolVersion;

        // BLE Notification functions
        static void IncomingCameraControlNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
        static void IncomingTimecodeNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);

        // For accessing the connection object from a static function
        static BMDCameraConnection* instancePtr;
};

#endif