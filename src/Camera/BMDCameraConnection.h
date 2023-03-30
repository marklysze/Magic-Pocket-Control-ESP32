#ifndef BMDCAMERACONNECTION_H
#define BMDCAMERACONNECTION_H

#include "BLEDevice.h"
#include "BLE\SerialSecurityHandler.h"
#include "BLE\ScreenSecurityHandler.h"
#include "BLE\BMDBLEClientCallback.h"
#include "BMDCamera.h"
#include "CCU\CCUUtility.h"
#include "BMDControlSystem.h"
#include <TFT_eSPI.h>
#include "ESP32\CST816S\CST816S.h"

// Forward declaration
#include "CCU\CCUDecodingFunctions.h"
//class CCUDecodingFunctions;

// class SerialSecurityHandler;

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
            FailedPassKey
        };

        BMDCameraConnection();
        ~BMDCameraConnection();

        void initialise(); // Serial security pass key
        void initialise(TFT_eSprite* windowPtr, TFT_eSprite* spritePassKeyPtr, CST816S* touchPtr, int screenWidth, int screenHeight); // Screen security pass key
        bool scan();
        // bool connect();
        void connect(BLEAddress cameraAddress);
        void disconnect();
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

        // BLE Notification functions
        static void IncomingCameraControlNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);

        // For accessing the connection object from a static function
        static BMDCameraConnection* instancePtr;
};

#endif