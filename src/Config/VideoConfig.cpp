#include "VideoConfig.h"

WhiteBalancePreset::WhiteBalancePreset(short inWhiteBalance, short inTint)
    : whiteBalance(inWhiteBalance)
    , tint(inTint)
{}

WhiteBalancePreset VideoConfig::kWhiteBalancePresets[] = {
    WhiteBalancePreset(5600, 10),   // Bright sunlight
    WhiteBalancePreset(3200, 0),    // Incandescent
    WhiteBalancePreset(4000, 15),   // Flourescent
    WhiteBalancePreset(4500, 15),   // Mixed light
    WhiteBalancePreset(6500, 10)    // Cloud
};

const short VideoConfig::kWhiteBalanceMin = 2500;
const short VideoConfig::kWhiteBalanceMax = 10000;
const short VideoConfig::kWhiteBalanceStep = 50;
const short VideoConfig::kTintMin = -50;
const short VideoConfig::kTintMax = 50;
const short VideoConfig::kOffSpeedFrameRateMin = 5;
const short VideoConfig::kOffSpeedFrameRateMax = 60;
const ushort VideoConfig::kSentSensorGainBase = 100;
const ushort VideoConfig::kReceivedSensorGainBase = 200;
const int VideoConfig::kISOValues[] = { 200, 400, 800, 1600 };
const int VideoConfig::kGainValues[] = { -6, 0, 6, 12 };
const double VideoConfig::kShutterAngles[] = { 11.2, 15.0, 22.5, 30.0, 37.5, 45.0, 60.0, 72.0, 75.0, 90.0, 108.0, 120.0, 144.0, 150.0, 172.8, 180.0, 216.0, 270.0, 324.0, 360.0 };
const int32_t VideoConfig::kShutterSpeeds[] = { 24, 25, 30, 50, 60, 100, 125, 200, 250, 500, 1000, 2000 };
const int VideoConfig::kShutterSpeedMin = 24;
const int VideoConfig::kShutterSpeedMax = 2000;
const double kShutterAngleMin = 5.0;
const double kShutterAngleMax = 360.0;
const double kFileFrameRates[] = { 23.976, 24, 25, 29.97, 30, 50, 59.94, 60 };

short VideoConfig::calculateIrisAV(const std::string& fnumberStr) {
    double fnumber = std::stod(fnumberStr); // Convert the string to a double
    double AV = std::log2(fnumber * fnumber); // Calculate the Aperture Value

    // Check if the calculated AV fits within the range of a short
    if (AV >= std::numeric_limits<short>::min() && AV <= std::numeric_limits<short>::max()) {
        return static_cast<short>(AV);
    } else {
        return 0; // Out of range
    }
}

int VideoConfig::GetWhiteBalancePresetFromValues(short whiteBalance, short tint)
{
    int presetIndex = 0;

    for (const auto& preset : kWhiteBalancePresets)
    {
        if (preset.whiteBalance == whiteBalance && preset.tint == tint)
            return presetIndex;

        presetIndex++;
    }

    return -1;
}
