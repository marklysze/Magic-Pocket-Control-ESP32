#ifndef BMDCAMERACONNECTION_H
#define BMDCAMERACONNECTION_H

#include "BLEDevice.h"
#include "Arduino_DebugUtils.h"

// Include screen and screen security handler only if we're using TFT_eSPI
#if USING_TFT_ESPI == 1
    #include <TFT_eSPI.h>
    #include "BLE_TFT_eSPI/ScreenSecurityHandler.h"
#elif USING_M5_BUTTONS == 1
    #include <ESP32-Chimera-Core.h>
    #include "BLE_M5GREY/ScreenSecurityHandlerM5Buttons.h"
#elif USING_M5GFX == 1
    #include "BLE_M5GFX/ScreenSecurityHandler.h"
#endif

#include "BLE/SerialSecurityHandler.h"
#include "BLE/BMDBLEClientCallback.h"
#include "BMDCamera.h"
#include "CCU/CCUUtility.h"
#include "BMDControlSystem.h"

#if USING_TFT_ESPI == 1
    #include "ESP32/CST816S/CST816S.h"
#endif

#include "CCU/CCUDecodingFunctions.h"
#include "Config/Versions.h"
#include "PowerControl.h"

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
            IncompatibleProtocol,
            ReceivedInitialPayload
        };

        // BMD Camera Status characteristic flags and connection status to align with BMD's code for connection status
        static byte bmdConnectionStatus;
        struct ConnectionStatusFlags {
            static const byte kNone = 0x00;
            static const byte kPower = 0x01;
            static const byte kConnected = 0x02;
            static const byte kPaired = 0x04;
            static const byte kVersionsVerified = 0x08;
            static const byte kInitialPayloadReceived = 0x10;
            static const byte kCameraReady = 0x20;
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

        #elif USING_M5_BUTTONS == 1

            // Defined in here as wouldn't link when in the cpp file and using the preprocessor directive
            void initialise(M5Display* displayPtr, int screenWidth, int screenHeight) // Screen security pass key
            {
                if(initialised)
                    return;

                appName = CODEAPPNAME;

                bleDevice.init("MPC");
                bleDevice.setPower(ESP_PWR_LVL_P9);
                bleDevice.setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);

                ScreenSecurityHandlerM5Buttons* securityHandler = new ScreenSecurityHandlerM5Buttons(this, displayPtr, screenWidth, screenHeight);
                bleDevice.setSecurityCallbacks(securityHandler);

                bleSecurity = new BLESecurity();
                bleSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
                bleSecurity->setCapability(ESP_IO_CAP_IN);
                bleSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

                status = ConnectionStatus::Disconnected;

                initialised = true;
            }

        #elif USING_M5GFX == 1

            // Defined in here as wouldn't link when in the cpp file and using the preprocessor directive
            void initialise(lgfx::v1::ITouch* touchPtr, M5GFX* windowPtr, int screenWidth, int screenHeight) // Screen security pass key
            {
                if(initialised)
                    return;

                appName = CODEAPPNAME;

                bleDevice.init("MPC");
                bleDevice.setPower(ESP_PWR_LVL_P9);
                bleDevice.setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);

                ScreenSecurityHandler* securityHandler = new ScreenSecurityHandler(this, touchPtr, windowPtr, screenWidth, screenHeight);
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
        void sendCommandToOutgoing(CCUPacketTypes::Command command, bool response = true); // Sends the command to the camera
        void sendBytesToOutgoing(std::vector<byte> data, bool response = true); // Primarily for testing, sends a byte array rather than a formulated and validated command

        ConnectionStatus status;
        std::vector<BLEAddress> cameraAddresses;

        static void connectCallback(BLEScanResults scanResults);

        static void clearBondedDevices(); // Clears BLE bonding so we will require pass key for the next connection
        static bool isCameraBonded(BLEAddress cameraAddress); // Have we got a bond to the camera address on the BLE device?
        unsigned long getInitialPayloadTime() { return initialPayloadTime; } // Have we received the initial payload of information from the camera?

    private:
        std::string appName;
        bool initialised = false;
        bool scanned = false;
        unsigned long initialPayloadTime = ULONG_MAX;

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
        BLERemoteCharacteristic* bleChar_CameraStatus;

        // BLE Notification functions
        static void IncomingCameraControlNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
        static void IncomingTimecodeNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
        static void IncomingCameraStatusNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);

        // For accessing the connection object from a static function
        static BMDCameraConnection* instancePtr;
};

#endif