#ifndef POWERCONTROL_H
#define POWERCONTROL_H

#include <cstdint>
#include <vector>
#include "ConstantsTypes.h"

class CameraStatus {
public:
    struct Flags {
        static const byte None = 0x00;
        static const byte CameraPowerFlag = 0x01;
        static const byte CameraReadyFlag = 0x02;
    };

    static const std::vector<byte> kPowerOff;
    static const std::vector<byte> kPowerOn;

    static byte GetCameraStatusFlags(const std::vector<byte>& data) {
        if (!data.empty()) {
            return data[0];
        }

        return 0;
    }
};

#endif