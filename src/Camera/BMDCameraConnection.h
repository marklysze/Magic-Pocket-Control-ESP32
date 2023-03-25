#ifndef BMDCAMERACONNECTION_H
#define BMDCAMERACONNECTION_H

#include "BLEDevice.h"
#include "BLE\SerialSecurityHandler.h"
#include "BLE\ScanAdvertisedDeviceCallbacks.h"
#include "BLE\BMDBLEClientCallback.h"
#include "BMDCamera.h"
#include "CCU\CCUUtility.h"
#include "BMDControlSystem.h"

// Forward declaration
#include "CCU\CCUDecodingFunctions.h"
//class CCUDecodingFunctions;

class BMDCameraConnection
{
    public:

        enum ConnectionStatus
        {
            Disconnected,
            Connected,
            Connecting
        };

        BMDCameraConnection();
        ~BMDCameraConnection();

        void initialise();
        bool scan();
        BMDCamera *connect();
        void disconnect();
        ConnectionStatus status;

        // static BMDCamera* getCamera(); // Public access for the Camera;

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

        // BMDCamera *camera;

        // BLE Notification functions
        static void IncomingCameraControlNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
};

#endif