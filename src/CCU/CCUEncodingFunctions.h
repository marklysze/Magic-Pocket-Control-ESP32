#ifndef CCUENCODINGFUNCTIONS_H
#define CCUENCODINGFUNCTIONS_H

#include "CCUPacketTypes.h"
#include "CCUUtility.h"
#include "Camera\TransportInfo.h"

class CCUEncodingFunctions
{
    public:
        static CCUPacketTypes::Command CreateVideoWhiteBalanceCommand(short whiteBalance, short tint);
        static CCUPacketTypes::Command CreateVoidCommand(CCUPacketTypes::Category category, byte parameter);
        static CCUPacketTypes::Command CreateVideoSetAutoWBCommand();
        static CCUPacketTypes::Command CreateRecordingFormatCommand(CCUPacketTypes::RecordingFormatData recordingFormatData);
        static CCUPacketTypes::Command CreateFixed16Command(short value, CCUPacketTypes::Category category, byte parameter);
        template <typename T>
        static CCUPacketTypes::Command CreateCommand(T value, CCUPacketTypes::Category category, byte parameter);
        static CCUPacketTypes::Command CreateVideoSensorGainCommand(byte value);
        static CCUPacketTypes::Command CreateVideoISOCommand(int value);
        static CCUPacketTypes::Command CreateShutterAngleCommand(int value);
        static CCUPacketTypes::Command CreateTransportInfoCommand(TransportInfo transportInfo);
};

#endif