#ifndef SERIALSECURITYHANDLER_H
#define SERIALSECURITYHANDLER_H

#include <BLEDevice.h>
#include <Arduino.h>
#include "Arduino_DebugUtils.h"
#include "Camera/BMDCameraConnection.h"

class BMDCameraConnection;

// This class handles the BLE security callbacks, primarily for getting the PassKey from the serial console (keyboard entry)
// The aim is that there would be a handler for a screen, serial (this), and one for a physically connected keypad (TBD)
// Once a Bluetooth connection has been established between the device and camera another PassKey should not be required until either has forgotten each other.
class SerialSecurityHandler : public BLESecurityCallbacks
{
  public:
    SerialSecurityHandler(BMDCameraConnection* bmdCameraConnectionPtr);

    uint32_t onPassKeyRequest();
    void onPassKeyNotify(uint32_t pass_key);
    bool onConfirmPIN(uint32_t pin);
    bool onSecurityRequest();
    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl);

  private:
    BMDCameraConnection* _bmdCameraConnectionPtr;
};

#endif
