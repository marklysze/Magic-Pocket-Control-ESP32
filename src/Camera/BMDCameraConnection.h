#ifndef BMDCAMERACONNECTION_H
#define BMDCAMERACONNECTION_H

#include "BLEDevice.h"
#include "Arduino_DebugUtils.h"
#include "BLE\SerialSecurityHandler.h"
#include "BLE\ScreenSecurityHandler.h"
#include "BLE\BMDBLEClientCallback.h"
#include "BMDCamera.h"
#include "CCU\CCUUtility.h"
#include "BMDControlSystem.h"
#include <TFT_eSPI.h>
#include "ESP32\CST816S\CST816S.h"
#include "CCU\CCUDecodingFunctions.h"
#include "Config\Versions.h"

class BMDCameraConnection
{
    public:

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
        void initialise(TFT_eSprite* windowPtr, TFT_eSprite* spritePassKeyPtr, CST816S* touchPtr, int screenWidth, int screenHeight); // Screen security pass key
        bool scan();
        // bool connect();
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