#ifndef TRANSPORTINFO_H
#define TRANSPORTINFO_H

#include <Arduino.h>
#include "CCU/CCUPacketTypes.h"
#include <vector>

class TransportInfo
{
public:
    struct TransportInfoSlot {
        bool active = false;
        CCUPacketTypes::ActiveStorageMedium medium = CCUPacketTypes::ActiveStorageMedium::CFastCard;
    };

    CCUPacketTypes::MediaTransportMode mode;
    sbyte speed = 0;

    bool loop = false;
    bool playAll = false;
    bool timelapseRecording = false;

    std::vector<TransportInfoSlot> slots;

    std::vector<byte> toArray();

    byte getActiveSlotCount();
};

#endif