#include "BMDCamera.h"

BMDCamera::BMDCamera() {
    // constructor code here
    setModelName("TBD");
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

void BMDCamera::setModelName(std::string inModelName)
{
    Serial.print("[setModelName Start: ");
    Serial.println(modelName.c_str());
    modelName = inModelName;
    Serial.print("[setModelName End: ");
    Serial.println(modelName.c_str());
}

std::string BMDCamera::getModelName()
{
    return modelName;
}