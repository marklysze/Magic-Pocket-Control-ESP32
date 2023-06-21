#ifndef CCUPACKETTYPESSTRING_H
#define CCUPACKETTYPESSTRING_H

#include "CCUPacketTypes.h"
#include "Config/LensConfig.h"

class CCUPacketTypesString
{
    public:

        static const std::string GetEnumString(CCUPacketTypes::ActiveStorageMedium value)
        {
            switch(value)
            {
                case CCUPacketTypes::ActiveStorageMedium::CFastCard:
                    return "CFAST";
                case CCUPacketTypes::ActiveStorageMedium::SDCard:
                    return "SD";
                case CCUPacketTypes::ActiveStorageMedium::SSDRecorder:
                    return "SSD";
                case CCUPacketTypes::ActiveStorageMedium::USB:
                    return "USB";
                default:
                    return Constants::UnknownEnumValue;
            }
        }

        static const std::string GetEnumString(CCUPacketTypes::MediaStatus value)
        {
            switch(value)
            {
                case CCUPacketTypes::MediaStatus::None:
                    return "None";
                case CCUPacketTypes::MediaStatus::Ready:
                    return "Ready";
                case CCUPacketTypes::MediaStatus::MountError:
                    return "Mount Error";
                case CCUPacketTypes::MediaStatus::RecordError:
                    return "Record Error";
                default:
                    return Constants::UnknownEnumValue;
            }
        }

        static const std::string GetEnumString(LensConfig::ApertureUnits value)
        {
            switch(value)
            {
                case LensConfig::ApertureUnits::Fstops:
                    return "f/";
                case LensConfig::ApertureUnits::Tstops:
                    return "T";
                default:
                    return Constants::UnknownEnumValue;
            }
        }
};

#endif