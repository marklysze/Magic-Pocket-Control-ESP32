#include "TransportInfo.h"

// Convert TransportInfo object to a byte array for transport
std::vector<byte> TransportInfo::toArray()
{
    byte flags = 0;

    if (loop) {
        flags |= static_cast<byte>(CCUPacketTypes::MediaTransportFlag::Loop);
    }
    if (playAll) {
        flags |= static_cast<byte>(CCUPacketTypes::MediaTransportFlag::PlayAll);
    }
    if (timelapseRecording) {
        flags |= static_cast<byte>(CCUPacketTypes::MediaTransportFlag::TimelapseRecording);
    }
    for (auto i = 0u; i < slots.size(); ++i) {
        if (slots[i].active) {
            flags |= CCUPacketTypes::slotActiveMasks[i];
        }
    }

    std::vector<uint8_t> data;
    data.push_back(static_cast<uint8_t>(mode));
    data.push_back(static_cast<uint8_t>(speed));
    data.push_back(flags);
    for (auto const& slot : slots) {
        data.push_back(static_cast<uint8_t>(slot.medium));
    }

    return data;
}

byte TransportInfo::getActiveSlotCount()
{
    byte count = 0;

    for(const TransportInfoSlot& slot : slots)
    {
        if(slot.active)
            count++;
    }

    return count;
}