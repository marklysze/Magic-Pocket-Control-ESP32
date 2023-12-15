#include "CCUPacketTypes.h"

ccu_fixed_t CCUPacketTypes::CCUFixedFromFloat(double f) {

    return static_cast<ccu_fixed_t>(f * 2048.0);
}

double CCUPacketTypes::CCUFloatFromFixed(ccu_fixed_t f) {
    return static_cast<double>(f) / 2048.0;
}

double CCUPacketTypes::CCUFloatFromFixed(uint16_t f) {
    return static_cast<double>(f) / 2048.0;
}

int32_t CCUPacketTypes::CCUPercentFromFixed(ccu_fixed_t f) {
    return (static_cast<int32_t>(f) * 100) / 2048;
}

// From: https://github.com/kasperskaarhoj/SKAARHOJ-Open-Engineering/blob/master/ArduinoLibs/BMDSDIControl/include/BMDSDICameraControl.h#L289
int16_t CCUPacketTypes::toFixed16(float value)
{
    value *= (1 << 11);

    if (value > 32767)
        value = 32767;
    else if (value < -32768)
        value = -32768;

    return value;
}

const std::array<int, 4> CCUPacketTypes::slotActiveMasks =
{
    0x20, //static_cast<int>(MediaTransportFlag::Disk1Active),
    0x40, //static_cast<int>(MediaTransportFlag::Disk2Active),
    0x10, //static_cast<int>(MediaTransportFlag::Disk3Active),
    0
};

// Used to validate the data that relates to enum's is valid, lists the enums and their valid values
const byte CCUPacketTypes::LensParameterValues[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
const byte CCUPacketTypes::VideoParameterValues[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
const byte CCUPacketTypes::AudioParameterValues[7] = {0, 1, 2, 3, 4, 5, 6};
const byte CCUPacketTypes::OutputParameterValues[4] = {0, 1, 2, 3};
const byte CCUPacketTypes::DisplayParameterValues[7] = {0, 1, 2, 3, 4, 5, 7};
const byte CCUPacketTypes::TallyParameterValues[3] = {0, 1, 2};
const byte CCUPacketTypes::ReferenceParameterValues[2] = {0, 1};
const byte CCUPacketTypes::ConfigurationParameterValues[4] = {0, 1, 2, 3};
const byte CCUPacketTypes::ColorCorrectionParameterValues[8] = {0, 1, 2, 3, 4, 5, 6, 7};
const byte CCUPacketTypes::StatusParameterValues[9] = {0, 1, 2, 3, 4, 5, 6, 7, 8}; // 2023-04-22 MS Added last last value, unknown what it is.
const byte CCUPacketTypes::MediaParameterValues[2] = {0, 1};
const byte CCUPacketTypes::ExDevControlParameterValues[1] = {0};
const byte CCUPacketTypes::MetadataParameterValues[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

int PacketFormatIndex::Destination = 0;
int PacketFormatIndex::CommandLength = 1;
int PacketFormatIndex::CommandId = 2;
int PacketFormatIndex::Source = 3;
int PacketFormatIndex::Category = 4;
int PacketFormatIndex::Parameter = 5;
int PacketFormatIndex::DataType = 6;
int PacketFormatIndex::OperationType = 7;
int PacketFormatIndex::PayloadStart = 8;