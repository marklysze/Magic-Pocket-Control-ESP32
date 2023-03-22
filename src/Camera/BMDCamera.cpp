#include "BMDCamera.h"

BMDCamera::BMDCamera() {
    // constructor code here
}

void BMDCamera::onIrisReceived(short apertureNumber, short infStopIndex)
{
    // MS TO BE CONVERTED.
}

void BMDCamera::onNormalisedApertureReceived(short normalisedAperture)
{
    // MS TO BE CONVERTED.
}

void BMDCamera::testReceived(String message)
{
    Serial.print("Camera Test Received: ");
    Serial.println(message);
}