#include "ScanAdvertisedDeviceCallbacks.h"

std::vector<BLEAddress> ScanAdvertisedDeviceCallbacks::cameraAddresses; // Stores the indexes of BMD cameras in the scan

//const std::string Constants::UUID_BMD_BCS = "291D567A-6D75-11E6-8B77-86F30CA893D3";

ScanAdvertisedDeviceCallbacks::ScanAdvertisedDeviceCallbacks()
{
    cameraAddresses.clear();
}

// Scan for BMD Cameras
void ScanAdvertisedDeviceCallbacks::onResult(BLEAdvertisedDevice advertisedDevice)
{
    // Serial.print("BLE Advertised Device found: ");
    // Serial.println(advertisedDevice.toString().c_str());

  if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(Constants::UUID_BMD_BCS))
  {
    // Add the camera address to our list if it doesn't already exist in there
    auto it = std::find(cameraAddresses.begin(), cameraAddresses.end(), advertisedDevice.getAddress());
    if (it == cameraAddresses.end())
    {
        cameraAddresses.push_back(advertisedDevice.getAddress());

        Serial.print("Blackmagic Camera found ");
        Serial.println(advertisedDevice.getAddress().toString().c_str());
    }

    /*
    Serial.print("Found Blackmagic Camera Service, stopping the scan.");

    BLEDevice::getScan()->stop();
    bleCamera = new BLEAdvertisedDevice(advertisedDevice);
    */
  }
  // else
    // Serial.println("<Non Blackmagic Device Found>");
}