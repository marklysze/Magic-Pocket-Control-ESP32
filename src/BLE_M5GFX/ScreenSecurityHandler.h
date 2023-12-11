#ifndef PASSKEYSCREEN_H
#define PASSKEYSCREEN_H

#include <BLEDevice.h>
#include <Arduino.h>
#include "Arduino_DebugUtils.h"
#include "Camera/BMDCameraConnection.h"
#include "M5GFX.h"
#include "ESP32/CST816S/CST816S.h"
#include "lgfx/v1/Touch.hpp"

class BMDCameraConnection;

// This class handles the BLE security callbacks, primarily for getting the PassKey from the screen with touch screen
// The aim is that there would be a handler for a screen (this), serial, and one for a physically connected keypad (TBD)
// Once a Bluetooth connection has been established between the device and camera another PassKey should not be required until either has forgotten each other.
class ScreenSecurityHandler : public BLESecurityCallbacks
{
  public:
    ScreenSecurityHandler(BMDCameraConnection* bmdCameraConnectionPtr, lgfx::v1::ITouch* touchPtr, M5GFX* windowPtr, int screenWidth, int screenHeight);

    uint32_t onPassKeyRequest();
    void onPassKeyNotify(uint32_t pass_key);
    bool onConfirmPIN(uint32_t pin);
    bool onSecurityRequest();
    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl);

  private:
    int KeyPressed(int x, int y);
    uint_fast8_t getTouchRaw(lgfx::v1::touch_point_t* tp, uint_fast8_t count); // Replicating the touch function to get touches while on the security page

    BMDCameraConnection* _bmdCameraConnectionPtr;
    M5GFX* _windowPtr;
    lgfx::v1::ITouch* _touchPtr;
    int _screenWidth;
    int _screenHeight;
};

#endif
