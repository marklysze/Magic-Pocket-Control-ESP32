#ifndef TRANSPORTINFO_H
#define TRANSPORTINFO_H

#include <Arduino.h>
#include "CCU\CCUPacketTypes.h"

class TransportInfo
{
public:
    byte transportMode; // = CCUPacketTypes::MediaTransportMode::Preview;
    byte storageMediumDisk1; // = CCUPacketTypes::ActiveStorageMedium::CFastCard;
    byte  storageMediumDisk2; // = CCUPacketTypes::ActiveStorageMedium::CFastCard;
    int8_t speed = 0;
    bool loop = false;
    bool playAll = false;
    bool disk1Active = false;
    bool disk2Active = false;
    bool timelapseRecording = false;

    byte* serialize();
};

#endif // TRANSPORTINFO_H
