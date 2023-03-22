#include "CCUDecodingFunctions.h"
#include <stdexcept>

void CCUDecodingFunctions::DecodeCCUPacket(const byte* byteArray, int length)
{
    bool isValid = CCUValidationFunctions::ValidateCCUPacket(byteArray, length);

    // BMDCameraConnection::getCamera()->testReceived("From DecodeCCUPacket");

    if (isValid)
    {
        byte commandLength = byteArray[PacketFormatIndex::CommandLength];
        // Serial.print("Command Length: ");Serial.println(commandLength);
        CCUPacketTypes::Category category = static_cast<CCUPacketTypes::Category>(byteArray[PacketFormatIndex::Category]);
        byte parameter = byteArray[PacketFormatIndex::Parameter];
        // Serial.print("Parameter: ");Serial.println(parameter);

        byte dataLength = static_cast<byte>(commandLength - CCUPacketTypes::kCCUCommandHeaderSize);
        // Serial.print("Data Length: ");Serial.println(dataLength);
        byte payloadOffset = CCUPacketTypes::kCUUPayloadOffset;
        // Serial.print("Payload Offset: ");Serial.println(payloadOffset);
        byte* payloadData = new byte[dataLength];
        memcpy(payloadData, byteArray + payloadOffset, dataLength);

        try
        {
            DecodePayloadData(category, parameter, payloadData, dataLength);
        }
        catch (const std::exception& ex)
        {
            Serial.print("Exception in DecodeCCUPacket: ");Serial.println(ex.what());
            delete[] payloadData;
            throw ex;
        }

        delete[] payloadData;
    }
}

// implementation of DecodePayloadData function
void CCUDecodingFunctions::DecodePayloadData(CCUPacketTypes::Category category, byte parameter, byte* payloadData, int payloadDataLength)
{
    if(category != CCUPacketTypes::Category::Status)
    {
        Serial.print("DecodePayloadData, Category: "); Serial.println(static_cast<byte>(category));
    }

    switch (category) {
        case CCUPacketTypes::Category::Lens:
            // DecodeLensCategory(parameter, payloadData, payloadDataLength);
            break;
        case CCUPacketTypes::Category::Video:
            DecodeVideoCategory(parameter, payloadData, payloadDataLength);
            break;
        case CCUPacketTypes::Category::Status:
            // DecodeStatusCategory(parameter, payloadData, payloadDataLength);
            break;
        case CCUPacketTypes::Category::Media:
            // DecodeMediaCategory(parameter, payloadData, payloadDataLength);
            break;
        case CCUPacketTypes::Category::Metadata:
            // DecodeMetadataCategory(parameter, payloadData, payloadDataLength);
            break;
        default:
            break;
    }
}

/*


// implementation of member functions
void CCUDecodingFunctions::DecodeLensCategory(byte parameter, byte* payloadData, int payloadLength)
{
    if(CCUUtility::byteValueExistsInArray(CCUPacketTypes::LensParameterValues, sizeof(CCUPacketTypes::LensParameterValues) / sizeof(CCUPacketTypes::LensParameterValues[0]), parameter))
    {
        CCUPacketTypes::LensParameter parameterType = static_cast<CCUPacketTypes::LensParameter>(parameter);

        switch (parameterType)
        {
        case CCUPacketTypes::LensParameter::ApertureFstop:
            DecodeApertureFStop(payloadData, payloadLength);
            break;
        case CCUPacketTypes::LensParameter::ApertureNormalised:
            // DecodeApertureNormalised(payloadData, payloadLength);
            break;
        case CCUPacketTypes::LensParameter::ApertureOrdinal:
            // Not catered for
            break;
        case CCUPacketTypes::LensParameter::AutoAperture:
            // Not catered for
            break;
        case CCUPacketTypes::LensParameter::AutoFocus:
            // Not catered for
            break;
        case CCUPacketTypes::LensParameter::ContinuousZoom:
            // Not catered for
            break;
        case CCUPacketTypes::LensParameter::Focus:
            // Not catered for
            break;
        case CCUPacketTypes::LensParameter::ImageStabilisation:
            // Not catered for
            break;
        case CCUPacketTypes::LensParameter::Zoom:
            // Not catered for
            break;
        case CCUPacketTypes::LensParameter::ZoomNormalised:
            // Not catered for
            break;
        default:
            break;
        }
    }
    else
        throw "Invalid value for LensParameter.";
}

*/

/*
template<typename T>
T* CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount(byte* data, int byteCount, int expectedCount) {
    int typeSize = sizeof(T);
    if (typeSize > byteCount) {
        Serial.println("Payload type size (" + String(typeSize) + ") is smaller than data size (" + String(byteCount) + ")");
        throw "Payload type size (" + String(typeSize) + ") is smaller than data size (" + String(byteCount) + ")";
    }

    int convertedCount = byteCount / typeSize;
    if (expectedCount != convertedCount) {
        Serial.println("Payload expected count (" + String(expectedCount) + ") not equal to converted count (" + String(convertedCount) + ")");
        throw "Payload expected count (" + String(expectedCount) + ") not equal to converted count (" + String(convertedCount) + ")";
    }

    T* payload = new T[convertedCount];
    memcpy(payload, data, byteCount);

    return payload;
}
*/

template<typename T>
std::vector<T> CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount(byte* data, int byteCount, int expectedCount) {
    int typeSize = sizeof(T);
    if (typeSize > byteCount) {
        Serial.println("Payload type size (" + String(typeSize) + ") is smaller than data size (" + String(byteCount) + ")");
        throw "Payload type size (" + String(typeSize) + ") is smaller than data size (" + String(byteCount) + ")";
    }

    int convertedCount = byteCount / typeSize;
    if (expectedCount != convertedCount) {
        Serial.println("Payload expected count (" + String(expectedCount) + ") not equal to converted count (" + String(convertedCount) + ")");
        throw "Payload expected count (" + String(expectedCount) + ") not equal to converted count (" + String(convertedCount) + ")";
    }

    std::vector<T> payload(convertedCount);
    memcpy(payload.data(), data, byteCount);

    return payload;
}

/*
void DecodeApertureFStop(byte* inData, int inDataLength)
{
    Serial.println("[DecodeApertureFStop]");

    float Exponent = (1 << 11);
    float myfStop = 16.0f;
    float result = myfStop * Exponent;

    short* data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, inDataLength, 2); // We're receiving 4 bytes, though expecting 2 bytes (equivalent to a Short / Int16), so this avoids an exception, though it's not clear why it's 4 bytes
    short apertureNumber = data[0];

    short fStopIndex = -1;
    if (apertureNumber != CCUPacketTypes::kLensAperture_NoLens) {
        fStopIndex = LensConfig::GetIndexForApertureNumber(apertureNumber);
        Serial.print("DecodeApertureFStop: fStop Index"); Serial.println(fStopIndex);
    }
    else
        Serial.println("DecodeApertureFStop: No Lens.");

    // BMDCameraConnection::getCamera()->onIrisReceived(apertureNumber, fStopIndex);

    delete[] data;
}

*/

void CCUDecodingFunctions::DecodeVideoCategory(byte parameter, byte* payloadData, int payloadLength)
{
    if(CCUUtility::byteValueExistsInArray(CCUPacketTypes::VideoParameterValues, sizeof(CCUPacketTypes::VideoParameterValues) / sizeof(CCUPacketTypes::VideoParameterValues[0]), parameter))
    {
        CCUPacketTypes::VideoParameter parameterType = static_cast<CCUPacketTypes::VideoParameter>(parameter);

        Serial.print("DecodeVideoCategory, ParameterType: "); Serial.println(static_cast<byte>(parameterType));

        switch (parameterType)
        {
            case CCUPacketTypes::VideoParameter::SensorGain:
                DecodeSensorGain(payloadData, payloadLength);
                break;
            case CCUPacketTypes::VideoParameter::ManualWB:
                DecodeManualWB(payloadData, payloadLength);
                break;
            case CCUPacketTypes::VideoParameter::Exposure:
                DecodeExposure(payloadData, payloadLength);
                break;
            case CCUPacketTypes::VideoParameter::RecordingFormat:
                DecodeRecordingFormat(payloadData, payloadLength);
                break;
            case CCUPacketTypes::VideoParameter::AutoExposureMode:
                // DecodeAutoExposureMode(payloadData);
                break;
            case CCUPacketTypes::VideoParameter::ShutterAngle:
                // DecodeShutterAngle(payloadData, ref observableBMDCamera);
                break;
            case CCUPacketTypes::VideoParameter::ShutterSpeed:
                // DecodeShutterSpeed(payloadData, ref observableBMDCamera);
                break;
            case CCUPacketTypes::VideoParameter::Gain:
                // DecodeGain(payloadData);
                break;
            case CCUPacketTypes::VideoParameter::ISO:
                DecodeISO(payloadData, payloadLength);
                break;
            case CCUPacketTypes::VideoParameter::DisplayLUT:
                // DecodeDisplayLUT(payloadData);
                break;
        default:
            break;
        }
    }
    else
        throw "Invalid value for LensParameter.";
}

void CCUDecodingFunctions::DecodeSensorGain(byte* inData, int inDataLength)
{
    std::vector<short> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, inDataLength, 1);
    short sensorGainValue = data[0];
    int sensorGain = sensorGainValue * static_cast<int>(VideoConfig::kReceivedSensorGainBase);
    Serial.print("Decoded Sensor Gain is "); Serial.println(sensorGain);
}

void CCUDecodingFunctions::DecodeManualWB(byte* inData, int inDataLength)
{
    std::vector<short> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, inDataLength, 2);
    short whiteBalance = data[0];
    short tint = data[1];
    Serial.print("Decoded White Balance is "); Serial.print(whiteBalance); Serial.print(" and Tint is "); Serial.println(tint);
}

void CCUDecodingFunctions::DecodeExposure(byte* inData, int inDataLength)
{
    std::vector<int32_t> data = ConvertPayloadDataWithExpectedCount<int32_t>(inData, inDataLength, 1);
    int32_t shutterSpeed = data[0];

    Serial.print("Decoded Exposure (Shutter Speed): "); Serial.println(shutterSpeed);
}

void CCUDecodingFunctions::DecodeRecordingFormat(byte* inData, int inDataLength)
{
    // short* data = ConvertPayloadDataWithExpectedCount<short>(inData, 5);
    std::vector<short> data = ConvertPayloadDataWithExpectedCount<short>(inData, inDataLength, 5);

    CCUPacketTypes::RecordingFormatData recordingFormatData;
    recordingFormatData.frameRate = data[0];
    recordingFormatData.offSpeedFrameRate = data[1];
    recordingFormatData.width = data[2];
    recordingFormatData.height = data[3];

    ushort flags = (ushort)data[4];
    recordingFormatData.mRateEnabled = (((int)flags & (int)CCUPacketTypes::VideoRecordingFormat::FileMRate) > 0);
    recordingFormatData.offSpeedEnabled = (((int)flags & (int)CCUPacketTypes::VideoRecordingFormat::SensorOffSpeed) > 0);
    recordingFormatData.interlacedEnabled = (((int)flags & (int)CCUPacketTypes::VideoRecordingFormat::Interlaced) > 0);
    recordingFormatData.windowedModeEnabled = (((int)flags & (int)CCUPacketTypes::VideoRecordingFormat::WindowedMode) > 0);

    Serial.print("Decoded Recording Format, Frame Rate is "); Serial.print(recordingFormatData.frameRate);
    Serial.print(", Off Speed Frame Rate is "); Serial.print(recordingFormatData.offSpeedFrameRate);
    Serial.print(", Width is "); Serial.print(recordingFormatData.width);
    Serial.print(", Height is "); Serial.print(recordingFormatData.height);
    Serial.print(", M-Rate Enabled is "); Serial.print(recordingFormatData.mRateEnabled);
    Serial.print(", Off Speed Enabled is "); Serial.print(recordingFormatData.offSpeedEnabled);
    Serial.print(", Interlaced Enabled is "); Serial.print(recordingFormatData.interlacedEnabled);
    Serial.print(", Windowed Mode Enabled is "); Serial.println(recordingFormatData.windowedModeEnabled);

    if(recordingFormatData.mRateEnabled && recordingFormatData.frameRate == 24 && !recordingFormatData.offSpeedEnabled)
        Serial.println("Frame Rate will be 23.98");
    else if(recordingFormatData.mRateEnabled && recordingFormatData.frameRate == 30 && !recordingFormatData.offSpeedEnabled)
        Serial.println("Frame Rate will be 29.97");
}

void CCUDecodingFunctions::DecodeISO(byte* inData, int inDataLength)
{
    std::vector<int32_t> data = ConvertPayloadDataWithExpectedCount<int32_t>(inData, inDataLength, 1);
    int32_t iso = data[0];

    Serial.print("Decoded ISO: "); Serial.println(iso);
}

/*

void DecodeApertureNormalised(byte* inData, int inDataLength)
{
    short* data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, inDataLength, 1); // Receiving a fixed16 number, as a short it will be 0-2,048 representing 0.0-1.0
    short apertureNormalisedNumber = data[0];

    // BMDCameraConnection::getCamera()->onNormalisedApertureReceived(apertureNormalisedNumber);

    delete[] data;
}

void CCUDecodingFunctions::DecodeStatusCategory(byte parameter, byte* payloadData, int payloadDataLength)
{
    // MS TO BE CONVERTED.
}

void CCUDecodingFunctions::DecodeMediaCategory(byte parameter, byte* payloadData, int payloadDataLength)
{
    // MS TO BE CONVERTED.
}

void CCUDecodingFunctions::DecodeMetadataCategory(byte parameter, byte* payloadData, int payloadDataLength)
{
    // MS TO BE CONVERTED.
}

*/