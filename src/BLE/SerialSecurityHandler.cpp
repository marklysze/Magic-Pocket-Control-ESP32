#include "SerialSecurityHandler.h"

// Take in the 
SerialSecurityHandler::SerialSecurityHandler(BMDCameraConnection* bmdCameraConnectionPtr)
{
  _bmdCameraConnectionPtr = bmdCameraConnectionPtr;
}

// code snippet from jeppo7745 https://www.instructables.com/id/Magic-Button-4k-the-20USD-BMPCC4k-Remote/
// Found through BlueMagic32, thank you guys!
uint32_t SerialSecurityHandler::onPassKeyRequest()
{
    // Update the connection status to requesting PassKey
    _bmdCameraConnectionPtr->status = BMDCameraConnection::NeedPassKey;

    Serial.println("---> PLEASE ENTER 6 DIGIT PIN (end with ENTER) : ");
    int pinCode = 0;
    char ch;
    do
    {
        while (!Serial.available())
        {
            vTaskDelay(1);
        }
        ch = Serial.read();
        if (ch >= '0' && ch <= '9')
        {
            pinCode = pinCode * 10 + (ch - '0');
            Serial.print(ch);
        }
    } while ((ch != '\n') && !(pinCode >= 100000 && pinCode < 999999)); // Go through if 6 numbers
    return pinCode;
}

void SerialSecurityHandler::onPassKeyNotify(uint32_t pass_key)
{
    DEBUG_VERBOSE("onPassKeyNotify");
    pass_key += 1;
}

bool SerialSecurityHandler::onConfirmPIN(uint32_t pin)
{
    DEBUG_VERBOSE("onConfirmPIN");
    return true;
}

bool SerialSecurityHandler::onSecurityRequest()
{
    DEBUG_VERBOSE("onSecurityRequest");
    return true;
}

void SerialSecurityHandler::onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl)
{
    DEBUG_VERBOSE("onAuthenticationComplete");
    
    // Update the connection status based on the outcome of the authentication
    if(auth_cmpl.success)
        _bmdCameraConnectionPtr->status = BMDCameraConnection::Connected;
    else
        _bmdCameraConnectionPtr->status = BMDCameraConnection::FailedPassKey;
}