#ifndef LENSCONFIG_H
#define LENSCONFIG_H

#include <algorithm>
#include <string>

class LensConfig {
public:
    static double fStopValues[];
    static short apertureNumbers[];

    static short GetIndexForApertureNumber(short targetNumber);
    static std::string GetFStopString(double fStopValue);
};

#endif