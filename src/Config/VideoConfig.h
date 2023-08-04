#ifndef VIDEOCONFIG_H
#define VIDEOCONFIG_H

#include "Camera/ConstantsTypes.h"
#include <cmath>
#include <limits>

struct WhiteBalancePreset
{
    short whiteBalance;
    short tint;

    WhiteBalancePreset(short inWhiteBalance, short inTint);
};

class VideoConfig
{
public:
    static WhiteBalancePreset kWhiteBalancePresets[];
    static const short kWhiteBalanceMin;
    static const short kWhiteBalanceMax;
    static const short kWhiteBalanceStep;
    static const short kTintMin;
    static const short kTintMax;
    static const short kOffSpeedFrameRateMin;
    static const short kOffSpeedFrameRateMax;
    static const ushort kSentSensorGainBase;
    static const ushort kReceivedSensorGainBase;
    static const int kISOValues[];
    static const int kGainValues[];
    static const double kShutterAngles[];
    static const int32_t kShutterSpeeds[];
    static const int kShutterSpeedMin;
    static const int kShutterSpeedMax;
    static const double kShutterAngleMin;
    static const double kShutterAngleMax;
    static const double kFileFrameRates[];

    static short calculateIrisAV(const std::string& fnumberStr);
    static int GetWhiteBalancePresetFromValues(short whiteBalance, short tint);
};

#endif