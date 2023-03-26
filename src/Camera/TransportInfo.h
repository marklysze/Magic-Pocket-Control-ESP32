#ifndef TRANSPORTINFO_H
#define TRANSPORTINFO_H

#include <Arduino.h>
#include "CCU\CCUPacketTypes.h"
#include <vector>

class TransportInfo
{
public:
    struct TransportInfoSlot {
        bool active = false;
        CCUPacketTypes::ActiveStorageMedium medium = CCUPacketTypes::ActiveStorageMedium::CFastCard;
    };

    CCUPacketTypes::MediaTransportMode mode; // = CCUPacketTypes::MediaTransportMode::Preview;
    // byte storageMediumDisk1; // = CCUPacketTypes::ActiveStorageMedium::CFastCard;
    // byte  storageMediumDisk2; // = CCUPacketTypes::ActiveStorageMedium::CFastCard;
    sbyte speed = 0;

    bool loop = false;
    bool playAll = false;
    bool timelapseRecording = false;

    std::vector<TransportInfoSlot> slots;

    // TransportInfo(std::vector<byte> data);

    // bool disk1Active = false;
    // bool disk2Active = false;

    byte* serialize();

    byte getActiveSlotCount();
};

#endif // TRANSPORTINFO_H
