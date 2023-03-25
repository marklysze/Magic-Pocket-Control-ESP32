#include "TransportInfo.h"

byte* TransportInfo::serialize()
{
    byte flags = 0;
    // if (loop) { flags |= static_cast<byte>(CCUPacketTypes::MediaTransportFlag::Loop); }
    // if (playAll) { flags |= static_cast<byte>(CCUPacketTypes::MediaTransportFlag::PlayAll); }
    // if (disk1Active) { flags |= static_cast<byte>(CCUPacketTypes::MediaTransportFlag::Disk1Active); }
    // if (disk2Active) { flags |= static_cast<byte>(CCUPacketTypes::MediaTransportFlag::Disk2Active); }
    // if (timelapseRecording) { flags |= static_cast<byte>(CCUPacketTypes::MediaTransportFlag::TimelapseRecording); }

    byte* data = new byte[5]{ 0, 0, 0, 0, 0 };
    // data[0] = transportMode;
    data[1] = speed;
    data[2] = flags;
    // data[3] = storageMediumDisk1;
    // data[4] = storageMediumDisk2;

    return data;
}



byte TransportInfo::getActiveSlotCount()
{
    byte count = 0;

    for(const Slot& slot : slots)
    {
        if(slot.active)
            count++;
    }

    return count;
}