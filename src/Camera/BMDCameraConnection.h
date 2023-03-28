#ifndef BMDCAMERACONNECTION_H
#define BMDCAMERACONNECTION_H

#include "BLEDevice.h"
#include "BLE\SerialSecurityHandler.h"
//#include "BLE\ScanAdvertisedDeviceCallbacks.h"
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
            Connecting,
            Scanning,
            ScanningFound,
            ScanningNoneFound
        };

        BMDCameraConnection();
        ~BMDCameraConnection();

        void initialise();
        bool scan();
        // bool connect();
        void connect();
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

        // BLE Notification functions
        static void IncomingCameraControlNotify(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);

        // For accessing the connection object from a static function
        static BMDCameraConnection* instancePtr;
};

#endif