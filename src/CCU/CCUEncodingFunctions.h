#ifndef CCUENCODINGFUNCTIONS_H
#define CCUENCODINGFUNCTIONS_H

#include "CCUPacketTypes.h"
#include "CCUUtility.h"
#include "Camera\TransportInfo.h"

class CCUEncodingFunctions
{
    public:
        CCUPacketTypes::Command CreateVideoWhiteBalanceCommand(short whiteBalance, short tint);
        CCUPacketTypes::Command CreateVoidCommand(CCUPacketTypes::Category category, byte parameter);
        CCUPacketTypes::Command CreateVideoSetAutoWBCommand();
        CCUPacketTypes::Command CreateRecordingFormatCommand(CCUPacketTypes::RecordingFormatData recordingFormatData);
        CCUPacketTypes::Command CreateRecordingFormatStatusCommand();
        CCUPacketTypes::Command CreateFixed16Command(short value, CCUPacketTypes::Category category, byte parameter);
        template <typename T>
        CCUPacketTypes::Command CreateCommand(T value, CCUPacketTypes::Category category, byte parameter);
        CCUPacketTypes::Command CreateVideoSensorGainCommand(byte value);
        CCUPacketTypes::Command CreateVideoISOCommand(int value);
        CCUPacketTypes::Command CreateTransportInfoCommand(TransportInfo transportInfo);
};

#endif