#include "SerialSecurityHandler.h"

// code snippet from jeppo7745 https://www.instructables.com/id/Magic-Button-4k-the-20USD-BMPCC4k-Remote/
// Found through BlueMagic32, thank you guys!
uint32_t SerialSecurityHandler::onPassKeyRequest()
{
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
    } while ((ch != '\n'));
    return pinCode;
}

void SerialSecurityHandler::onPassKeyNotify(uint32_t pass_key)
{
    Serial.println("onPassKeyNotify");
    pass_key += 1;
}

bool SerialSecurityHandler::onConfirmPIN(uint32_t pin)
{
    Serial.println("onConfirmPIN");
    return true;
}

bool SerialSecurityHandler::onSecurityRequest()
{
    Serial.println("onSecurityRequest");
    return true;
}

void SerialSecurityHandler::onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl)
{
    // Serial.println("onAuthenticationComplete");
    int index = 1;
}
