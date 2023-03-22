#ifndef CODECINFO_H
#define CODECINFO_H

#include "CCU\CCUPacketTypes.h"

class CodecInfo
{
    public:
        CCUPacketTypes::BasicCodec basicCodec = CCUPacketTypes::BasicCodec::BRAW;
        byte codecVariant = CCUPacketTypes::CodecVariants::kDefault;
};

#endif