#ifndef CCUVALIDATIONFUNCTIONS_H
#define CCUVALIDATIONFUNCTIONS_H

#include <Arduino.h>
#include <stddef.h>
#include <type_traits>
#include <vector>
#include "Camera\ConstantsTypes.h"
#include "CCUUtility.h"
#include "CCUPacketTypes.h"

class CCUValidationFunctions {
    public:
        /*
        static bool ValidateCCUPacket(std::vector<byte> byteArray) {

            byte packetSize = static_cast<byte>(byteArray.size());
            bool isSizeValid = (packetSize >= CCUPacketTypes::kPacketSizeMin && packetSize <= CCUPacketTypes::kPacketSizeMax);
            if (!isSizeValid) {
                Serial.println("ValidateCCUPacket: Size is not valid.");
                //Logger.LogWithInfo("CCU packet (" + String(packetSize) + " bytes) is not between " + String(CCUPacketTypes::kPacketSizeMin) + " and " + String(CCUPacketTypes::kPacketSizeMax) + " bytes.");
                return false;
            }

            byte commandLength = byteArray[PacketFormatIndex::CommandLength];
            byte expectedPayloadSize = commandLength - CCUPacketTypes::kCCUCommandHeaderSize;
            byte actualPayloadSize = packetSize - (CCUPacketTypes::kCCUPacketHeaderSize + CCUPacketTypes::kCCUCommandHeaderSize);
            if (actualPayloadSize < expectedPayloadSize) {
                Serial.println("ValidateCCUPacket: Actual payload size less than expected payload size.");
                //Logger.LogWithInfo("Payload (" + String(actualPayloadSize) + ") is smaller than expected (" + String(expectedPayloadSize) + ").");
                return false;
            }

            byte categoryValue = byteArray[PacketFormatIndex::Category];
            CCUPacketTypes::Category category = static_cast<CCUPacketTypes::Category>(categoryValue);
            if ((byte)category == 0) {
                Serial.println("ValidateCCUPacket: CCU packet has invalid category.");
                //Logger.LogWithInfo("CCU packet has invalid category (" + String(categoryValue) + ").");
                return false;
            }

            bool isParamterValid = false;
            byte parameterValue = byteArray[PacketFormatIndex::Parameter];
            switch (category) {
                case CCUPacketTypes::Category::Lens:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::LensParameterValues, sizeof(CCUPacketTypes::LensParameterValues) / sizeof(CCUPacketTypes::LensParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Video:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::VideoParameterValues, sizeof(CCUPacketTypes::VideoParameterValues) / sizeof(CCUPacketTypes::VideoParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Audio:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::AudioParameterValues, sizeof(CCUPacketTypes::AudioParameterValues) / sizeof(CCUPacketTypes::AudioParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Output:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::OutputParameterValues, sizeof(CCUPacketTypes::OutputParameterValues) / sizeof(CCUPacketTypes::OutputParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Display:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::DisplayParameterValues, sizeof(CCUPacketTypes::DisplayParameterValues) / sizeof(CCUPacketTypes::DisplayParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Tally:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::TallyParameterValues, sizeof(CCUPacketTypes::TallyParameterValues) / sizeof(CCUPacketTypes::TallyParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Reference:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::ReferenceParameterValues, sizeof(CCUPacketTypes::ReferenceParameterValues) / sizeof(CCUPacketTypes::ReferenceParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Configuration:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::ConfigurationParameterValues, sizeof(CCUPacketTypes::ConfigurationParameterValues) / sizeof(CCUPacketTypes::ConfigurationParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::ColorCorrection:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::ColorCorrectionParameterValues, sizeof(CCUPacketTypes::ColorCorrectionParameterValues) / sizeof(CCUPacketTypes::ColorCorrectionParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Status:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::StatusParameterValues, sizeof(CCUPacketTypes::StatusParameterValues) / sizeof(CCUPacketTypes::StatusParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Media:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::MediaParameterValues, sizeof(CCUPacketTypes::MediaParameterValues) / sizeof(CCUPacketTypes::MediaParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::ExternalDeviceControl:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::ExDevControlParameterValues, sizeof(CCUPacketTypes::ExDevControlParameterValues) / sizeof(CCUPacketTypes::ExDevControlParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Metadata:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::MetadataParameterValues, sizeof(CCUPacketTypes::MetadataParameterValues) / sizeof(CCUPacketTypes::MetadataParameterValues[0]), parameterValue);
                    break;
                default:
                    isParamterValid = false;
                    break;
            }

            if (!isParamterValid) {
                Serial.println("ValidateCCUPacket: CCU packet has invalid parameter for categery.");
                //Logger.LogWithInfo("CCU packet has invalid parameter " + String(parameterValue) + " for category " + category.getString() + ".");
                return false;
            }

            return true;
        }
        */

       static bool ValidateCCUPacket(const byte* byteArray, const size_t size) {
            byte packetSize = static_cast<byte>(size);
            bool isSizeValid = (packetSize >= CCUPacketTypes::kPacketSizeMin && packetSize <= CCUPacketTypes::kPacketSizeMax);
            if (!isSizeValid) {
                Serial.println("ValidateCCUPacket: Size is not valid.");
                //Logger.LogWithInfo("CCU packet (" + String(packetSize) + " bytes) is not between " + String(CCUPacketTypes::kPacketSizeMin) + " and " + String(CCUPacketTypes::kPacketSizeMax) + " bytes.");
                return false;
            }

            byte commandLength = byteArray[PacketFormatIndex::CommandLength];
            byte expectedPayloadSize = commandLength - CCUPacketTypes::kCCUCommandHeaderSize;
            byte actualPayloadSize = packetSize - (CCUPacketTypes::kCCUPacketHeaderSize + CCUPacketTypes::kCCUCommandHeaderSize);
            if (actualPayloadSize < expectedPayloadSize) {
                Serial.println("ValidateCCUPacket: Actual payload size less than expected payload size.");
                //Logger.LogWithInfo("Payload (" + String(actualPayloadSize) + ") is smaller than expected (" + String(expectedPayloadSize) + ").");
                return false;
            }

            byte categoryValue = byteArray[PacketFormatIndex::Category];
            CCUPacketTypes::Category category = static_cast<CCUPacketTypes::Category>(categoryValue);
            if ((byte)category == 0) {
                Serial.println("ValidateCCUPacket: CCU packet has invalid category.");
                //Logger.LogWithInfo("CCU packet has invalid category (" + String(categoryValue) + ").");
                return false;
            }

            bool isParamterValid = false;
            byte parameterValue = byteArray[PacketFormatIndex::Parameter];
            switch (category) {
                case CCUPacketTypes::Category::Lens:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::LensParameterValues, sizeof(CCUPacketTypes::LensParameterValues) / sizeof(CCUPacketTypes::LensParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Video:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::VideoParameterValues, sizeof(CCUPacketTypes::VideoParameterValues) / sizeof(CCUPacketTypes::VideoParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Audio:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::AudioParameterValues, sizeof(CCUPacketTypes::AudioParameterValues) / sizeof(CCUPacketTypes::AudioParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Output:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::OutputParameterValues, sizeof(CCUPacketTypes::OutputParameterValues) / sizeof(CCUPacketTypes::OutputParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Display:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::DisplayParameterValues, sizeof(CCUPacketTypes::DisplayParameterValues) / sizeof(CCUPacketTypes::DisplayParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Tally:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::TallyParameterValues, sizeof(CCUPacketTypes::TallyParameterValues) / sizeof(CCUPacketTypes::TallyParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Reference:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::ReferenceParameterValues, sizeof(CCUPacketTypes::ReferenceParameterValues) / sizeof(CCUPacketTypes::ReferenceParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Configuration:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::ConfigurationParameterValues, sizeof(CCUPacketTypes::ConfigurationParameterValues) / sizeof(CCUPacketTypes::ConfigurationParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::ColorCorrection:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::ColorCorrectionParameterValues, sizeof(CCUPacketTypes::ColorCorrectionParameterValues) / sizeof(CCUPacketTypes::ColorCorrectionParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Status:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::StatusParameterValues, sizeof(CCUPacketTypes::StatusParameterValues) / sizeof(CCUPacketTypes::StatusParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Media:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::MediaParameterValues, sizeof(CCUPacketTypes::MediaParameterValues) / sizeof(CCUPacketTypes::MediaParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::ExternalDeviceControl:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::ExDevControlParameterValues, sizeof(CCUPacketTypes::ExDevControlParameterValues) / sizeof(CCUPacketTypes::ExDevControlParameterValues[0]), parameterValue);
                    break;
                case CCUPacketTypes::Category::Metadata:
                    isParamterValid = CCUUtility::byteValueExistsInArray(CCUPacketTypes::MetadataParameterValues, sizeof(CCUPacketTypes::MetadataParameterValues) / sizeof(CCUPacketTypes::MetadataParameterValues[0]), parameterValue);
                    break;
                default:
                    isParamterValid = false;
                    break;
            }

            if (!isParamterValid) {
                Serial.println("ValidateCCUPacket: CCU packet has invalid parameter for category.");
                //Logger.LogWithInfo("CCU packet has invalid parameter " + String(parameterValue) + " for category " + category.getString() + ".");
                return false;
            }

            return true;
        }
};

#endif