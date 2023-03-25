#ifndef BMDCAMERA_H
#define BMDCAMERA_H

#include <Arduino.h>

class BMDCamera
{
public:
    BMDCamera();

    void onIrisReceived(short apertureNumber, short infStopIndex);
    void onNormalisedApertureReceived(short normalisedAperture);
    void testReceived(String message);

    void setModelName(std::string inModelName);
    std::string getModelName();

private:
    std::string modelName;
};

#endif