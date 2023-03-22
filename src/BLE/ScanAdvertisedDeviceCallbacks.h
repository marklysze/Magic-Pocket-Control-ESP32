#ifndef SCAN_ADVERTISED_DEVICE_CALLBACKS_H
#define SCAN_ADVERTISED_DEVICE_CALLBACKS_H

#include <BLEDevice.h>
#include <Arduino.h>
#include <algorithm>
#include "Camera\ConstantsTypes.h"

// This class provides feedback to the BLEScan object scanning for devices
// It looks specifically for BMD Cameras and keeps a track of their addresses in "cameraAddresses" so they can be connected to
class ScanAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    public:
        ScanAdvertisedDeviceCallbacks();
        void onResult(BLEAdvertisedDevice advertisedDevice);
        static std::vector<BLEAddress> cameraAddresses;

    private:
        BLEAdvertisedDevice* bleCamera = nullptr;
        int indexCounter = 0;
};

#endif
