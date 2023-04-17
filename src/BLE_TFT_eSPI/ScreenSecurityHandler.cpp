#include "ScreenSecurityHandler.h"

// Take in all the pointers we need access to to render the screen and handle touch
ScreenSecurityHandler::ScreenSecurityHandler(BMDCameraConnection* bmdCameraConnectionPtr, TFT_eSprite* windowPtr, TFT_eSprite* spritePassKeyPtr, CST816S* touchPtr, int screenWidth, int screenHeight)
{
  _bmdCameraConnectionPtr = bmdCameraConnectionPtr;
  _windowPtr = windowPtr;
  _spritePassKeyPtr = spritePassKeyPtr;
  _touchPtr = touchPtr;
  _screenWidth = screenWidth;
  _screenHeight = screenHeight;
}

// Triggers when a Pass Key is required
uint32_t ScreenSecurityHandler::onPassKeyRequest()
{
    // Update the connection status to requesting PassKey
    _bmdCameraConnectionPtr->status = BMDCameraConnection::NeedPassKey;

    // Allow 15 seconds to enter the pass key.
    unsigned long startTime = millis();
    unsigned long currentTime = 0;

    // Draw the screen
    _spritePassKeyPtr->fillSprite(TFT_BLACK);

    // Left side
    _spritePassKeyPtr->fillRect(0, 0, 13, 170, TFT_ORANGE);
    _spritePassKeyPtr->fillRect(13, 0, 2, 170, TFT_DARKGREY);

    _spritePassKeyPtr->setTextSize(2);
    _spritePassKeyPtr->setTextColor(TFT_WHITE);
    _spritePassKeyPtr->drawString("Code:", 24, 7);

    // Draw the 11 buttons, left to right, top to bottom
    _spritePassKeyPtr->fillSmoothRoundRect(20, 30, 70, 40, 5, TFT_YELLOW, TFT_TRANSPARENT); // 7
    _spritePassKeyPtr->fillSmoothRoundRect(95, 30, 70, 40, 5, TFT_YELLOW, TFT_TRANSPARENT); // 8
    _spritePassKeyPtr->fillSmoothRoundRect(170, 30, 70, 40, 5, TFT_YELLOW, TFT_TRANSPARENT); // 9
    _spritePassKeyPtr->fillSmoothRoundRect(245, 30, 70, 40, 5, TFT_RED, TFT_TRANSPARENT); // Back

    _spritePassKeyPtr->setTextColor(TFT_BLACK);
    _spritePassKeyPtr->drawString("7", 50, 43);
    _spritePassKeyPtr->drawString("8", 125, 43);
    _spritePassKeyPtr->drawString("9", 200, 43);
    _spritePassKeyPtr->fillTriangle(269, 52, 289, 45, 289, 58, TFT_BLACK);

    _spritePassKeyPtr->fillSmoothRoundRect(20, 75, 70, 40, 5, TFT_YELLOW, TFT_TRANSPARENT); // 4
    _spritePassKeyPtr->fillSmoothRoundRect(95, 75, 70, 40, 5, TFT_YELLOW, TFT_TRANSPARENT); // 5
    _spritePassKeyPtr->fillSmoothRoundRect(170, 75, 70, 40, 5, TFT_YELLOW, TFT_TRANSPARENT); // 6
    _spritePassKeyPtr->fillSmoothRoundRect(245, 75, 70, 85, 5, TFT_YELLOW, TFT_TRANSPARENT); // 0

    _spritePassKeyPtr->drawString("4", 50, 88);
    _spritePassKeyPtr->drawString("5", 125, 88);
    _spritePassKeyPtr->drawString("6", 200, 88);
    _spritePassKeyPtr->drawString("0", 275, 111);

    _spritePassKeyPtr->fillSmoothRoundRect(20, 120, 70, 40, 5, TFT_YELLOW, TFT_TRANSPARENT); // 1
    _spritePassKeyPtr->fillSmoothRoundRect(95, 120, 70, 40, 5, TFT_YELLOW, TFT_TRANSPARENT); // 2
    _spritePassKeyPtr->fillSmoothRoundRect(170, 120, 70, 40, 5, TFT_YELLOW, TFT_TRANSPARENT); // 3

    _spritePassKeyPtr->drawString("1", 50, 134);
    _spritePassKeyPtr->drawString("2", 125, 134);
    _spritePassKeyPtr->drawString("3", 200, 134);

    _spritePassKeyPtr->pushToSprite(_windowPtr, 0, 0);
    _windowPtr->pushSprite(0, 0);

    bool pinComplete = false;
    std::vector<int> pinCodeArray;

    _windowPtr->setTextSize(2);
    _windowPtr->setTextColor(TFT_WHITE);

    do
    {  
        // Allow up to 15 seconds to enter the pass key
        currentTime = millis();

        if(currentTime - startTime >= 15000)
        {
            // Took too long!
            DEBUG_VERBOSE("15 seconds to enter pass key, time expired.");
            pinComplete = true;
            break;
        }
        else
        {
            // Use the left bar to show time left
            int reduceBar = ((currentTime - startTime) / 1000) * (_screenHeight / 15);
            _windowPtr->fillRect(0, 0, 13, reduceBar, TFT_BLACK);
        }

        // Show the pin code entered
        _windowPtr->fillRect(95, 0, 100, 21, TFT_BLACK);
        for(int count = 0; count < pinCodeArray.size(); count++)
        {
            _windowPtr->drawString(String(pinCodeArray[count]), 95 + (count * 15), 7);
        }

        _windowPtr->pushSprite(0, 0);

        // Wait for Touches
        if (_touchPtr->available()) {
    
            if(_touchPtr->data.eventID == CST816S::TOUCHEVENT::UP) // Only on finger up.
            {
                int oriented_x = _screenWidth - _touchPtr->data.y;
                int oriented_y = _touchPtr->data.x;

                if(_touchPtr->data.gestureID == CST816S::GESTURE::NONE) // Only a tap gesture counts.
                {
                    int pressedKey = KeyPressed(oriented_x, oriented_y);

                    if(pressedKey > -2)
                    {
                        if(pressedKey == -1)
                        {
                            // Backspace
                            pinCodeArray.pop_back();
                        }
                        else
                        {
                            pinCodeArray.push_back(pressedKey);
                        }
                    }
                }
            }
        }

        if(pinCodeArray.size() == 6)
            pinComplete = true;
        }

    while(!pinComplete); // && !(pinCode >= 100000 && pinCode < 999999));

   // Convert the vector array of 6 numbers into a number
    if(pinCodeArray.size() == 6)
    {
        int result = 0;
        for (int i = 0; i < 6; i++) {
            result = result * 10 + pinCodeArray[i];
        }

        return result; // Here's the pass key heading to authenticate.
    }
    else
        return 0; // Didn't enter a 6 digit pass key in time.
}

void ScreenSecurityHandler::onPassKeyNotify(uint32_t pass_key)
{
    DEBUG_VERBOSE("onPassKeyNotify");
    pass_key += 1;
}

bool ScreenSecurityHandler::onConfirmPIN(uint32_t pin)
{
    DEBUG_VERBOSE("onConfirmPIN");
    return true;
}

bool ScreenSecurityHandler::onSecurityRequest()
{
    DEBUG_VERBOSE("onSecurityRequest");
    return true;
}

void ScreenSecurityHandler::onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl)
{
    // Update the connection status based on the outcome of the authentication
    if(auth_cmpl.success)
        _bmdCameraConnectionPtr->status = BMDCameraConnection::Connected;
    else
        _bmdCameraConnectionPtr->status = BMDCameraConnection::FailedPassKey;
}

// Returns the key pressed
// 0-9 for numbers
// -1 for backspace
// -2 for nothing
int ScreenSecurityHandler::KeyPressed(int x, int y)
{
    if(x >= 20 && x <= 90 && y >= 30 && y <= 70)
        return 7;
    else if(x >= 95 && x <= 165 && y >= 30 && y <= 70)
        return 8;
    else if(x >= 170 && x <= 240 && y >= 30 && y <= 70)
        return 9;
    else if(x >= 245 && y >= 30 && y <= 70)
        return -1;

    else if(x >= 20 && x <= 90 && y >= 75 && y <= 115)
        return 4;
    else if(x >= 95 && x <= 165 && y >= 75 && y <= 115)
        return 5;
    else if(x >= 170 && x <= 240 && y >= 75 && y <= 115)
        return 6;
    else if(x >= 245 && y >= 75 && y <= 160)
        return 0;

    else if(x >= 20 && x <= 90 && y >= 120 && y <= 160)
        return 1;
    else if(x >= 95 && x <= 165 && y >= 120 && y <= 160)
        return 2;
    else if(x >= 170 && x <= 240 && y >= 120 && y <= 160)
        return 3;

    return -2;
}