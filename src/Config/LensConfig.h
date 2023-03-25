#ifndef LENSCONFIG_H
#define LENSCONFIG_H

#include <algorithm>
#include <string>
#include <cmath>

class LensConfig {
public:
    static double fStopValues[];
    static short apertureNumbers[];

    enum ApertureUnits : short
    {
        Fstops = 0,
        Tstops = 1
    };

    static short GetIndexForApertureNumber(short targetNumber);
    static std::string GetFStopString(float fStopValue, ApertureUnits apertureUnits);
};

#endif