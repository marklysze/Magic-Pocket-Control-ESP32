#ifndef CODECINFO_H
#define CODECINFO_H

#include "CCU/CCUPacketTypes.h"

class CodecInfo
{
    public:
        CCUPacketTypes::BasicCodec basicCodec = CCUPacketTypes::BasicCodec::BRAW;
        byte codecVariant = CCUPacketTypes::CodecVariants::kDefault;

        CodecInfo(CCUPacketTypes::BasicCodec inBasicCodec, byte inCodecVariant) : basicCodec(inBasicCodec), codecVariant(inCodecVariant) {}

        std::string to_string() const {

            std::string returnString = "";

            switch(basicCodec)
            {
                case CCUPacketTypes::BasicCodec::RAW:
                    returnString = "RAW";
                    break;

                case CCUPacketTypes::BasicCodec::DNxHD:

                    switch(codecVariant)
                    {
                        case 0:
                            returnString = "DNxHD, LL Raw";
                            break;
                        case 1:
                            returnString = "DNxHD, Raw 3:1";
                            break;
                        case 2:
                            returnString = "DNxHD, Raw 4:1";
                            break;
                    }

                    break;
                case CCUPacketTypes::BasicCodec::ProRes:

                    switch(codecVariant)
                    {
                        case 0:
                            returnString = "ProRes HQ";
                            break;
                        case 1:
                            returnString = "ProRes 422";
                            break;
                        case 2:
                            returnString = "ProRes LT";
                            break;
                        case 3:
                            returnString = "ProRes PXY";
                            break;
                        case 4:
                            returnString = "ProRes 444";
                            break;
                        case 5:
                            returnString = "ProRes 444XQ";
                            break;
                    }

                    break;
                case CCUPacketTypes::BasicCodec::BRAW:

                    switch(codecVariant)
                    {
                        case 0:
                            returnString = "BRAW Q0";
                            break;
                        case 1:
                            returnString = "BRAW Q5";
                            break;
                        case 2:
                            returnString = "BRAW 3:1";
                            break;
                        case 3:
                            returnString = "BRAW 5:1";
                            break;
                        case 4:
                            returnString = "BRAW 8:1";
                            break;
                        case 5:
                            returnString = "BRAW 12:1";
                            break;
                        case 6:
                            returnString = "BRAW 18:1"; // Guessing as the Ursa 12K has 18:1.
                            break;
                        case 7:
                            returnString = "BRAW Q1";
                            break;
                        case 8:
                            returnString = "BRAW Q3";
                            break;
                    }

                    break;
            }            
            
            return returnString;
        }
};

#endif