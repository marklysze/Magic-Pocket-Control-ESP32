#include "LensConfig.h"

double LensConfig::fStopValues[] = { 1.2, 1.4, 1.8, 2.0, 2.2, 2.4, 2.6, 2.8, 3.2, 3.5, 3.7, 4.0, 4.5, 4.8, 5.2, 5.6, 6.2, 6.7, 7.3, 8.0, 8.7, 9.5, 10.0, 11.0, 12.0, 14.0, 15.0, 16.0, 17.0, 19.0, 21.0, 22.0 };

short LensConfig::apertureNumbers[] = { 1077, 1988, 3473, 4096, 4659, 5173, 5646, 6084, 6873, 7402, 7731, 8192, 8888, 9269, 9742, 10180, 10781, 11240, 11746, 12288, 12783, 13303, 13606, 14169, 14684, 15594, 16002, 16384, 16742, 17399, 17990, 18265 };

short LensConfig::GetIndexForApertureNumber(short targetNumber) {
    if(targetNumber > apertureNumbers[sizeof(apertureNumbers)/sizeof(apertureNumbers[0]) - 1])
        return apertureNumbers[sizeof(apertureNumbers)/sizeof(apertureNumbers[0]) - 1];

    if (std::find(std::begin(apertureNumbers), std::end(apertureNumbers), targetNumber) != std::end(apertureNumbers))
        return (short)std::distance(apertureNumbers, std::find(std::begin(apertureNumbers), std::end(apertureNumbers), targetNumber));
    else
        return -1;
}

std::string LensConfig::GetFStopString(float fStopValue, ApertureUnits apertureUnits)
{
    std::string unitPrefix = "";

    switch (apertureUnits) {
        case ApertureUnits::Fstops:
            unitPrefix = "f";
            break;
        case ApertureUnits::Tstops:
            unitPrefix = "T";
            break;
    }

    // std::string fStopString = std::to_string(static_cast<int>(fStopValue * 10 + 0.5) / 10.0);

    char fStopBuffer[8];
    std::snprintf(fStopBuffer, sizeof(fStopBuffer), "%.1f", fStopValue);
    std::string fStopString(fStopBuffer);

    return unitPrefix + fStopString;

    /*
    bool isFractionalNumber = fStopValue != std::floor(fStopValue);
    bool isLessThanTen = fStopValue < 10.0;
    if(isFractionalNumber || isLessThanTen) {
        return unitPrefix + std::to_string(fStopValue).substr(0, 3);
    }

    return unitPrefix + std::to_string((int)fStopValue);
    */
}
