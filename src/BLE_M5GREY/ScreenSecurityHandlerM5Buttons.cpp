#include "ScreenSecurityHandlerM5Buttons.h"
#include "Fonts/Lato_Regular11pt7b.h" // Standard font
#include "Fonts/Lato_Regular12pt7b.h"
#include "Fonts/AgencyFB_Bold9pt7b.h" // Agency FB small for above buttons

// Take in all the pointers we need access to to render the screen and handle touch
ScreenSecurityHandlerM5Buttons::ScreenSecurityHandlerM5Buttons(BMDCameraConnection* bmdCameraConnectionPtr, M5Display* displayPtr, int screenWidth, int screenHeight)
{
  _bmdCameraConnectionPtr = bmdCameraConnectionPtr;
  _displayPtr = displayPtr;
  _screenWidth = screenWidth;
  _screenHeight = screenHeight;
}

// Triggers when a Pass Key is required
uint32_t ScreenSecurityHandlerM5Buttons::onPassKeyRequest()
{
    // Update the connection status to requesting PassKey
    _bmdCameraConnectionPtr->status = BMDCameraConnection::NeedPassKey;

    // Allow X seconds to enter the pass key.
    unsigned long startTime = millis();
    unsigned long currentTime = 0;

    // Draw the screen
    _displayPtr->fillScreen(TFT_BLACK);

    // Left side
    _displayPtr->fillRect(0, 0, 13, _screenHeight, TFT_ORANGE);
    _displayPtr->fillRect(13, 0, 2, _screenHeight, TFT_DARKGREY);

    // Physical Button Labels
    _displayPtr->setTextColor(TFT_WHITE);
    _displayPtr->fillSmoothRoundRect(30, 210, 80, 40, 3, TFT_DARKCYAN);
    _displayPtr->drawCenterString("+1", 70, 217, &AgencyFB_Bold9pt7b);

    _displayPtr->fillSmoothRoundRect(120, 210, 80, 40, 3, TFT_DARKCYAN);
    _displayPtr->drawCenterString("NEXT", 160, 217, &AgencyFB_Bold9pt7b);

    // Text description
    _displayPtr->drawString("Enter 6 digit passcode", 24, 7, &Lato_Regular11pt7b);

    // 6 Underlines
    _displayPtr->drawLine(50, 100, 70, 100, TFT_WHITE);
    _displayPtr->drawLine(80, 100, 100, 100, TFT_WHITE);
    _displayPtr->drawLine(110, 100, 130, 100, TFT_WHITE);
    _displayPtr->drawLine(140, 100, 160, 100, TFT_WHITE);
    _displayPtr->drawLine(170, 100, 190, 100, TFT_WHITE);
    _displayPtr->drawLine(200, 100, 220, 100, TFT_WHITE);

    bool pinComplete = false;
    std::vector<int> pinCodeArray;

    pinCodeArray.push_back(1); // Start with a 1

    unsigned long lastTapTime = 0; // Ensure we don't count a tap as a double entry
    bool submitNumber = false; // When they've finished, submit
    bool refreshButtonLabels = false;

    do
    {  
        // Allow up to 50 seconds to enter the pass key
        currentTime = millis();

        if(currentTime - startTime >= (timeAllowance * 1000))
        {
            // Took too long!
            DEBUG_VERBOSE("30 seconds to enter pass key, time expired.");
            pinComplete = true;
            break;
        }
        else
        {
            // Use the left bar to show time left
            int reduceBar = ((currentTime - startTime) / 1000) * (_screenHeight / timeAllowance);
            _displayPtr->fillRect(0, 0, 13, reduceBar, TFT_BLACK);
        }

        M5.update();

        if(M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed())
        {
            _displayPtr->fillRect(50, 80, 200, 19, TFT_BLACK);

            if(M5.BtnA.wasPressed())
            {
                // Increment the current position
                int num = pinCodeArray[pinCodeArray.size() - 1];
                if(num < 9)
                    pinCodeArray[pinCodeArray.size() - 1]++;
                else
                    pinCodeArray[pinCodeArray.size() - 1] = 0;
            }
            else if(M5.BtnB.wasPressed())
            {
                // Next number
                if(pinCodeArray.size() < 6)
                    pinCodeArray.push_back(1); // Move to the next number
                else
                    submitNumber = true;
                
                refreshButtonLabels = true;
            }
            else if(M5.BtnC.wasPressed())
            {
                // Go back a number
                if(pinCodeArray.size() > 1)
                    pinCodeArray.pop_back(); // Remove the last number
                
                refreshButtonLabels = true;
            }

            if(refreshButtonLabels)
            {
                _displayPtr->fillSmoothRoundRect(120, 210, 80, 40, 3, TFT_DARKCYAN);
                _displayPtr->drawCenterString(pinCodeArray.size() < 6 ? "NEXT" : "SUBMIT", 160, 217, &AgencyFB_Bold9pt7b);
                
                if(pinCodeArray.size() > 1)
                {
                    _displayPtr->fillSmoothRoundRect(210, 210, 80, 40, 3, TFT_DARKCYAN);
                    _displayPtr->drawCenterString("DEL", 250, 217, &AgencyFB_Bold9pt7b);
                }
                else
                    _displayPtr->fillRect(210, 210, 80, 40, TFT_BLACK);
            }
        }

        // Display the pinCode
        for(int i = 0; i < pinCodeArray.size(); i++)
        {
            _displayPtr->drawCentreString(std::to_string(pinCodeArray[i]).c_str(), 60 + (30 * i), 80, &Lato_Regular12pt7b);
        }

        if(submitNumber)
            pinComplete = true;
        }

    while(!pinComplete);

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

void ScreenSecurityHandlerM5Buttons::onPassKeyNotify(uint32_t pass_key)
{
    DEBUG_VERBOSE("onPassKeyNotify");
    pass_key += 1;
}

bool ScreenSecurityHandlerM5Buttons::onConfirmPIN(uint32_t pin)
{
    DEBUG_VERBOSE("onConfirmPIN");
    return true;
}

bool ScreenSecurityHandlerM5Buttons::onSecurityRequest()
{
    DEBUG_VERBOSE("onSecurityRequest");
    return true;
}

void ScreenSecurityHandlerM5Buttons::onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl)
{
    // Update the connection status based on the outcome of the authentication
    if(auth_cmpl.success)
        _bmdCameraConnectionPtr->status = BMDCameraConnection::Connected;
    else
        _bmdCameraConnectionPtr->status = BMDCameraConnection::FailedPassKey;
}