#ifndef CCUVALIDATIONFUNCTIONS_H
#define CCUVALIDATIONFUNCTIONS_H

#include <Arduino.h>
#include <stddef.h>
#include <type_traits>
#include <vector>
#include "Arduino_DebugUtils.h"
#include "Camera/ConstantsTypes.h"
#include "CCUUtility.h"
#include "CCUPacketTypes.h"

class CCUValidationFunctions {
    public:
       static bool ValidateCCUPacket(std::vector<byte> byteArray) {
            byte packetSize = byteArray.size();
            bool isSizeValid = (packetSize >= CCUPacketTypes::kPacketSizeMin && packetSize <= CCUPacketTypes::kPacketSizeMax);
            if (!isSizeValid) {
                DEBUG_ERROR("ValidateCCUPacket: Size is not valid.");
                return false;
            }

            byte commandLength = byteArray[PacketFormatIndex::CommandLength];
            byte expectedPayloadSize = commandLength - CCUPacketTypes::kCCUCommandHeaderSize;
            byte actualPayloadSize = packetSize - (CCUPacketTypes::kCCUPacketHeaderSize + CCUPacketTypes::kCCUCommandHeaderSize);
            if (actualPayloadSize < expectedPayloadSize) {
                DEBUG_ERROR("ValidateCCUPacket: Actual payload size less than expected payload size.");
                return false;
            }

            byte categoryValue = byteArray[PacketFormatIndex::Category];
            CCUPacketTypes::Category category;
            try
            {
                category = static_cast<CCUPacketTypes::Category>(categoryValue);                
            }
            catch(const std::exception& e)
            {
                DEBUG_ERROR("ValidateCCUPacket: CCU packet has invalid category.");
                return false;
            }

            bool isParameterValid = false;
            byte parameterValue = byteArray[PacketFormatIndex::Parameter];
            bool isCategoryValid = true;
            switch (category) {
                case CCUPacketTypes::Category::Lens:
                    isParameterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::LensParameterValues, sizeof(CCUPacketTypes::LensParameterValues) / sizeof(CCUPacketTypes::LensParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Video:
                    isParameterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::VideoParameterValues, sizeof(CCUPacketTypes::VideoParameterValues) / sizeof(CCUPacketTypes::VideoParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Audio:
                    isParameterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::AudioParameterValues, sizeof(CCUPacketTypes::AudioParameterValues) / sizeof(CCUPacketTypes::AudioParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Output:
                    isParameterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::OutputParameterValues, sizeof(CCUPacketTypes::OutputParameterValues) / sizeof(CCUPacketTypes::OutputParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Display:
                    isParameterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::DisplayParameterValues, sizeof(CCUPacketTypes::DisplayParameterValues) / sizeof(CCUPacketTypes::DisplayParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Tally:
                    isParameterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::TallyParameterValues, sizeof(CCUPacketTypes::TallyParameterValues) / sizeof(CCUPacketTypes::TallyParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Reference:
                    isParameterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::ReferenceParameterValues, sizeof(CCUPacketTypes::ReferenceParameterValues) / sizeof(CCUPacketTypes::ReferenceParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Configuration:
                    isParameterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::ConfigurationParameterValues, sizeof(CCUPacketTypes::ConfigurationParameterValues) / sizeof(CCUPacketTypes::ConfigurationParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::ColorCorrection:
                    isParameterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::ColorCorrectionParameterValues, sizeof(CCUPacketTypes::ColorCorrectionParameterValues) / sizeof(CCUPacketTypes::ColorCorrectionParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Status:
                    isParameterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::StatusParameterValues, sizeof(CCUPacketTypes::StatusParameterValues) / sizeof(CCUPacketTypes::StatusParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Media:
                    isParameterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::MediaParameterValues, sizeof(CCUPacketTypes::MediaParameterValues) / sizeof(CCUPacketTypes::MediaParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::ExternalDeviceControl:
                    isParameterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::ExDevControlParameterValues, sizeof(CCUPacketTypes::ExDevControlParameterValues) / sizeof(CCUPacketTypes::ExDevControlParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Metadata:
                    isParameterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::MetadataParameterValues, sizeof(CCUPacketTypes::MetadataParameterValues) / sizeof(CCUPacketTypes::MetadataParameterValues[0]), parameterValue);
                    break;
                default:
                    isCategoryValid = false;
                    isParameterValid = false;
                    break;
            }

            if(!isCategoryValid)
            {
                if(categoryValue != 255) // Ignore outputting Category ID 255
                {
                    DEBUG_ERROR("ValidateCCUPacket: CCU packet has invalid category and parameter, category %i, Parameter Value: %i", categoryValue, parameterValue);
                }

                return false;
            }
            else if (!isParameterValid) {
                DEBUG_ERROR("ValidateCCUPacket: CCU packet has invalid parameter for category: %i, Parameter Value: %i", categoryValue, parameterValue);
                return false;
            }

            return true;
        }
};

#endif