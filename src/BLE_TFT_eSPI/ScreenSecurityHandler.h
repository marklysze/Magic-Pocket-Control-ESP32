#ifndef PASSKEYSCREEN_H
#define PASSKEYSCREEN_H

#include <BLEDevice.h>
#include <Arduino.h>
#include "Arduino_DebugUtils.h"
#include "Camera/BMDCameraConnection.h"
#include <TFT_eSPI.h>
#include "ESP32/CST816S/CST816S.h"

class BMDCameraConnection;

// This class handles the BLE security callbacks, primarily for getting the PassKey from the screen with touch screen
// The aim is that there would be a handler for a screen (this), serial, and one for a physically connected keypad (TBD)
// Once a Bluetooth connection has been established between the device and camera another PassKey should not be required until either has forgotten each other.
class ScreenSecurityHandler : public BLESecurityCallbacks
{
  public:
    ScreenSecurityHandler(BMDCameraConnection* bmdCameraConnectionPtr, TFT_eSprite* windowPtr, TFT_eSprite* spritePassKeyPtr, CST816S* touchPtr, int screenWidth, int screenHeight);

    uint32_t onPassKeyRequest();
    void onPassKeyNotify(uint32_t pass_key);
    bool onConfirmPIN(uint32_t pin);
    bool onSecurityRequest();
    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl);

  private:
    int KeyPressed(int x, int y);

    BMDCameraConnection* _bmdCameraConnectionPtr;
    TFT_eSprite* _windowPtr;
    TFT_eSprite* _spritePassKeyPtr;
    CST816S* _touchPtr;
    int _screenWidth;
    int _screenHeight;
};

#endif
