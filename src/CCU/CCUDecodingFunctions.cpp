#include "CCUDecodingFunctions.h"
#include <stdexcept>

void CCUDecodingFunctions::DecodeCCUPacket(std::vector<byte> byteArray)
{
    bool isValid = CCUValidationFunctions::ValidateCCUPacket(byteArray);

    if (isValid)
    {
        byte commandLength = byteArray[PacketFormatIndex::CommandLength];
        CCUPacketTypes::Category category = static_cast<CCUPacketTypes::Category>(byteArray[PacketFormatIndex::Category]);
        byte parameter = byteArray[PacketFormatIndex::Parameter];

        byte dataLength = static_cast<byte>(commandLength - CCUPacketTypes::kCCUCommandHeaderSize);
        byte payloadOffset = CCUPacketTypes::kCUUPayloadOffset;
        std::vector<byte> payloadData(byteArray.begin() + payloadOffset, byteArray.begin() + payloadOffset + dataLength);

        try
        {
            // Need to check packets coming in at the individual bytes, use this block of code
            // Output decoded packet bytes, but ignore battery is it comes in too regularly
            // Example of filtering to category 10 (Media) and parameter 0 (Codec)
            /*
            if(category != CCUPacketTypes::Category::Status && parameter != (byte)CCUPacketTypes::StatusParameter::Battery)
            {
                if(category == CCUPacketTypes::Category::Media && parameter == (byte)CCUPacketTypes::MediaParameter::Codec)
                {
                    DEBUG_DEBUG("DecodeCCUPacket: ");
                    for(int index = 0; index < byteArray.size(); index++)
                    {
                        DEBUG_DEBUG("%i: %i", index, byteArray[index]);
                        
                    }
                }
            }
            */

            DecodePayloadData(category, parameter, payloadData);
        }
        catch (const std::exception& ex)
        {
            DEBUG_ERROR("Exception in DecodeCCUPacket: %s", ex.what());
            throw ex;
        }
    }
}

// implementation of DecodePayloadData function
void CCUDecodingFunctions::DecodePayloadData(CCUPacketTypes::Category category, byte parameter, std::vector<byte> payloadData)
{
    switch (category) {
        case CCUPacketTypes::Category::Lens:
            DecodeLensCategory(parameter, payloadData);
            break;
        case CCUPacketTypes::Category::Video:
            DecodeVideoCategory(parameter, payloadData);
            break;
        case CCUPacketTypes::Category::Status:
            DecodeStatusCategory(parameter, payloadData);
            break;
        case CCUPacketTypes::Category::Media:
            DecodeMediaCategory(parameter, payloadData);
            break;
        case CCUPacketTypes::Category::Metadata:
            DecodeMetadataCategory(parameter, payloadData);
            break;
        case CCUPacketTypes::Category::Display:
            DecodeDisplayCategory(parameter, payloadData);
            break;
        case CCUPacketTypes::Category::Audio:
            DEBUG_VERBOSE("DecodePayloadData, Audio Category Not Supported Yet");
            break;
        case CCUPacketTypes::Category::Output:
            DEBUG_VERBOSE("DecodePayloadData, Output Category Not Supported Yet");
            break;
        case CCUPacketTypes::Category::Tally:
            DEBUG_VERBOSE("DecodePayloadData, Tally Category Not Supported Yet");
            break;
        case CCUPacketTypes::Category::Reference:
            DEBUG_VERBOSE("DecodePayloadData, Reference Category Not Supported Yet");
            break;
        case CCUPacketTypes::Category::Configuration:
            DEBUG_VERBOSE("DecodePayloadData, Configuration Category Not Supported Yet");
            break;
        case CCUPacketTypes::Category::ColorCorrection:
            DEBUG_VERBOSE("DecodePayloadData, ColorCorrection Category Not Supported Yet");
            break;
        case CCUPacketTypes::Category::ExternalDeviceControl:
            DEBUG_VERBOSE("DecodePayloadData, ExternalDeviceControl Category Not Supported Yet");
            break;
        default:            
            break;
    }
}

// implementation of member functions
void CCUDecodingFunctions::DecodeLensCategory(byte parameter, std::vector<byte> payloadData)
{
    if(CCUUtility::byteValueExistsInArray(CCUPacketTypes::LensParameterValues, sizeof(CCUPacketTypes::LensParameterValues) / sizeof(CCUPacketTypes::LensParameterValues[0]), parameter))
    {
        CCUPacketTypes::LensParameter parameterType = static_cast<CCUPacketTypes::LensParameter>(parameter);

        // Serial.print("DecodeLensCategory, ParameterType: "); Serial.println(static_cast<byte>(parameterType));

        switch (parameterType)
        {
        case CCUPacketTypes::LensParameter::ApertureFstop:
            DecodeApertureFStop(payloadData);
            break;
        case CCUPacketTypes::LensParameter::ApertureNormalised:
            DecodeApertureNormalised(payloadData);
            break;
        case CCUPacketTypes::LensParameter::ApertureOrdinal:
            // Not catered for
            DEBUG_VERBOSE("DecodeLensCategory, ParameterType ApertureOrdinal not catered for: %i", static_cast<byte>(parameterType));
            break;
        case CCUPacketTypes::LensParameter::AutoAperture:
            // Not catered for
            DEBUG_VERBOSE("DecodeLensCategory, ParameterType AutoAperture not catered for: %i", static_cast<byte>(parameterType));
            break;
        case CCUPacketTypes::LensParameter::AutoFocus:
            DecodeAutoFocus(payloadData);
            break;
        case CCUPacketTypes::LensParameter::ContinuousZoom:
            // Not catered for
            DEBUG_VERBOSE("DecodeLensCategory, ParameterType ContinuousZoom not catered for: %i", static_cast<byte>(parameterType));
            break;
        case CCUPacketTypes::LensParameter::Focus:
            // Not catered for -- does not appear to come in with the focus value
            DEBUG_VERBOSE("DecodeLensCategory, ParameterType Focus not catered for: %i", static_cast<byte>(parameterType));
            break;
        case CCUPacketTypes::LensParameter::ImageStabilisation:
            DecodeImageStabilisation(payloadData);
            break;
        case CCUPacketTypes::LensParameter::Zoom:
            DecodeZoom(payloadData);
            break;
        case CCUPacketTypes::LensParameter::ZoomNormalised:
            // Not catered for
            DEBUG_VERBOSE("DecodeLensCategory, ParameterType ZoomNormalised not catered for: %i", static_cast<byte>(parameterType));
            break;
        default:
            DEBUG_VERBOSE("DecodeLensCategory, ParameterType #%i not catered for", static_cast<byte>(parameterType));
            break;
        }
    }
    else
        throw "Invalid value for Lens Parameter.";
}

// Returns the number of elements of the type T in the data (e.g. 2 byte data type and 4 bytes of data will return 4 / 2 = 2)
template<typename T>
int CCUDecodingFunctions::GetCount(std::vector<byte> data)
{
    int typeSize = sizeof(T);
    int byteCount = data.size();
    int convertedCount = byteCount / typeSize;

    return convertedCount;
}

template<typename T>
std::vector<T> CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount(std::vector<byte> data, int expectedCount) {
    int typeSize = sizeof(T);
    int byteCount = data.size();
    if (typeSize > byteCount) {
        DEBUG_ERROR("Payload type size (%i) is smaller than data size (%i)", typeSize, byteCount);
        throw "Payload type size (" + std::to_string(typeSize) + ") is smaller than data size (" + std::to_string(byteCount) + ")";
    }

    int convertedCount = byteCount / typeSize;
    if (expectedCount != convertedCount) {
        DEBUG_ERROR("Payload expected count (%i) not equal to converted count (%i)", expectedCount, convertedCount);
        throw "Payload expected count (" + std::to_string(expectedCount) + ") not equal to converted count (" + std::to_string(convertedCount) + ")";
    }

    std::vector<T> payload(convertedCount);
    memcpy(payload.data(), data.data(), byteCount);

    return payload;
}
/*
std::vector<T> CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount(byte* data, int byteCount, int expectedCount) {
    int typeSize = sizeof(T);
    if (typeSize > byteCount) {
        DEBUG_ERROR("Payload type size (%i) is smaller than data size (%i)", typeSize, byteCount);
        throw "Payload type size (" + String(typeSize) + ") is smaller than data size (" + String(byteCount) + ")";
    }

    int convertedCount = byteCount / typeSize;
    if (expectedCount != convertedCount) {
        DEBUG_ERROR("Payload expected count (%i) not equal to converted count (%i)", expectedCount, convertedCount);
        throw "Payload expected count (" + String(expectedCount) + ") not equal to converted count (" + String(convertedCount) + ")";
    }

    std::vector<T> payload(convertedCount);
    memcpy(payload.data(), data, byteCount);

    return payload;
}
*/

std::string CCUDecodingFunctions::ConvertPayloadDataToString(std::vector<byte> data) {
    return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}

/*
std::string CCUDecodingFunctions::ConvertPayloadDataToString(byte* data, int byteCount)
{
    return std::string(reinterpret_cast<const char*>(data), byteCount);
}
*/
// For DecodeApertureFStop
float CCUDecodingFunctions::ConvertCCUApertureToFstop(int16_t ccuAperture)
{
    float fstop = 0.0;
    if (ccuAperture != CCUPacketTypes::kLensAperture_NoLens) {
        fstop = pow(2, CCUFloatFromFixed(ccuAperture) / 2);
    }
    return fstop;
}

// For DecodeApertureFStop
float CCUDecodingFunctions::CCUFloatFromFixed(ccu_fixed_t f) {
    return static_cast<float>(f) / 2048.0;
}

void CCUDecodingFunctions::DecodeApertureFStop(std::vector<byte> inData)
{
    std::vector<ccu_fixed_t> data;
    LensConfig::ApertureUnits apertureUnits = LensConfig::ApertureUnits::Fstops;
    
    if(inData.size() == 4)
    {
        data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<ccu_fixed_t>(inData, 2); // First two bytes are the aperture value and the last two are the aperture units (FStop = 0, TStop = 1)
        apertureUnits = static_cast<LensConfig::ApertureUnits>(data[1]);

        DEBUG_DEBUG("DecodeApertureFStop Units:");
        DEBUG_DEBUG(std::to_string(apertureUnits).c_str());
    }
    else
    {
        // We may only get the fstops not the F stops vs T stops component.
        data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<ccu_fixed_t>(inData, 1); // Two bytes are the aperture value
    }

    ccu_fixed_t apertureNumber = data[0];

    DEBUG_DEBUG("DecodeApertureFStop number:");
    DEBUG_DEBUG(std::to_string(apertureNumber).c_str());

    if(apertureNumber != CCUPacketTypes::kLensAperture_NoLens)
    {
        BMDControlSystem::getInstance()->getCamera()->onHasLens(true); // A lens is attached

        BMDControlSystem::getInstance()->getCamera()->onApertureUnitsReceived(apertureUnits);
        BMDControlSystem::getInstance()->getCamera()->onApertureFStopStringReceived(LensConfig::GetFStopString(ConvertCCUApertureToFstop(apertureNumber), apertureUnits));
    }
    else
    {
        BMDControlSystem::getInstance()->getCamera()->onHasLens(false); // No Lens
    }
}

void CCUDecodingFunctions::DecodeApertureNormalised(std::vector<byte> inData)
{
    std::vector<short> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, 1); // Receiving a fixed16 number, as a short it will be 0-2,048 representing 0.0-1.0
    short apertureNormalisedNumber = data[0];

    if(apertureNormalisedNumber != CCUPacketTypes::kLensAperture_NoLens)
    {
        float apertureValue = static_cast<float>(apertureNormalisedNumber) / 2048.0f; // Convert to float and perform division
        // std::string apertureString = "Decode Aperture Normalised: " + std::to_string(static_cast<int>(apertureValue * 100.0f)) + "%"; // Multiply by 100 to get percentage and convert to string

        BMDControlSystem::getInstance()->getCamera()->onHasLens(true); // A lens is attached

        BMDControlSystem::getInstance()->getCamera()->onApertureNormalisedReceived(static_cast<int>(apertureValue * 100.0f));
    }
    else
    {
        BMDControlSystem::getInstance()->getCamera()->onHasLens(false); // No Lens
    }
}

void CCUDecodingFunctions::DecodeAutoFocus(std::vector<byte> inData)
{
    bool instantaneousAutoFocusPressed = true;

    BMDControlSystem::getInstance()->getCamera()->onAutoFocusPressed();
}

void CCUDecodingFunctions::DecodeZoom(std::vector<byte> inData)
{
    std::vector<ccu_fixed_t> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, 1);
    ccu_fixed_t focalLengthMM = data[0];

    if(focalLengthMM != 0)
    {
        BMDControlSystem::getInstance()->getCamera()->onHasLens(true); // A lens is attached

        BMDControlSystem::getInstance()->getCamera()->onFocalLengthMMReceived(focalLengthMM);
    }
    else
    {
        // Serial.println("Decode Zoom: No Lens Information");
    }
}

void CCUDecodingFunctions::DecodeImageStabilisation(std::vector<byte> inData)
{
    bool imageStabilisationOn = inData[0] == 1;

    DEBUG_DEBUG("Received Image Stabilisation: %s", (imageStabilisationOn ? "YES" : "NO"));

    BMDControlSystem::getInstance()->getCamera()->OnImageStabilisationReceived(imageStabilisationOn);
}


void CCUDecodingFunctions::DecodeVideoCategory(byte parameter, std::vector<byte> payloadData)
{
    if(CCUUtility::byteValueExistsInArray(CCUPacketTypes::VideoParameterValues, sizeof(CCUPacketTypes::VideoParameterValues) / sizeof(CCUPacketTypes::VideoParameterValues[0]), parameter))
    {
        CCUPacketTypes::VideoParameter parameterType = static_cast<CCUPacketTypes::VideoParameter>(parameter);

        // Serial.print("DecodeVideoCategory, ParameterType: "); Serial.println(static_cast<byte>(parameterType));

        switch (parameterType)
        {
            case CCUPacketTypes::VideoParameter::SensorGain:
                DecodeSensorGainISO(payloadData);
                break;
            case CCUPacketTypes::VideoParameter::ManualWB:
                DecodeManualWB(payloadData);
                break;
            case CCUPacketTypes::VideoParameter::Exposure:
                DecodeExposure(payloadData);
                break;
            case CCUPacketTypes::VideoParameter::RecordingFormat:
                DecodeRecordingFormat(payloadData);
                break;
            case CCUPacketTypes::VideoParameter::AutoExposureMode:
                DecodeAutoExposureMode(payloadData);
                break;
            case CCUPacketTypes::VideoParameter::ShutterAngle:
                DecodeShutterAngle(payloadData);
                break;
            case CCUPacketTypes::VideoParameter::ShutterSpeed:
                DecodeShutterSpeed(payloadData);
                break;
            case CCUPacketTypes::VideoParameter::Gain:
                DecodeGain(payloadData);
                break;
            case CCUPacketTypes::VideoParameter::ISO:
                DecodeISO(payloadData);
                break;
            case CCUPacketTypes::VideoParameter::DisplayLUT:
                DecodeDisplayLUT(payloadData);
                break;
            default:
                DEBUG_VERBOSE("DecodeVideoCategory, ParameterType #%i not catered for", static_cast<byte>(parameterType));
                break;
        }
    }
    else
        throw "Invalid value for Video Parameter.";
}

void CCUDecodingFunctions::DecodeSensorGainISO(std::vector<byte> inData)
{
    std::vector<short> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, 1);
    short sensorGainValue = data[0];
    int sensorGain = sensorGainValue * static_cast<int>(VideoConfig::kReceivedSensorGainBase);
    
    // Serial.print("Decoded Sensor Gain is "); Serial.println(sensorGain);

    BMDControlSystem::getInstance()->getCamera()->onSensorGainISOReceived(sensorGain);
}

void CCUDecodingFunctions::DecodeManualWB(std::vector<byte> inData)
{
    std::vector<short> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, 2);
    short whiteBalance = data[0];
    short tint = data[1];

    // Serial.print("Decoded White Balance is "); Serial.print(whiteBalance); Serial.print(" and Tint is "); Serial.println(tint);

    BMDControlSystem::getInstance()->getCamera()->onWhiteBalanceReceived(whiteBalance);
    BMDControlSystem::getInstance()->getCamera()->onTintReceived(tint);
}

void CCUDecodingFunctions::DecodeExposure(std::vector<byte> inData)
{
    std::vector<int32_t> data = ConvertPayloadDataWithExpectedCount<int32_t>(inData, 1);
    int32_t shutterSpeedMS = data[0]; // Time in microseconds: us

    // Serial.print("Decoded Exposure (Shutter Speed): "); Serial.println(shutterSpeed);

    BMDControlSystem::getInstance()->getCamera()->onShutterSpeedMSReceived(shutterSpeedMS);
}

void CCUDecodingFunctions::DecodeRecordingFormat(std::vector<byte> inData)
{
    std::vector<short> data = ConvertPayloadDataWithExpectedCount<short>(inData, 5);

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
    recordingFormatData.sensorMRateEnabled = (((int)flags & (int)CCUPacketTypes::VideoRecordingFormat::SensorMRate) > 0);

    Serial.print("Decoded Recording Format, Frame Rate is "); Serial.print(recordingFormatData.frameRate);
    Serial.print(", Off Speed Frame Rate is "); Serial.print(recordingFormatData.offSpeedFrameRate);
    Serial.print(", Width is "); Serial.print(recordingFormatData.width);
    Serial.print(", Height is "); Serial.print(recordingFormatData.height);
    Serial.print(", M-Rate Enabled is "); Serial.print(recordingFormatData.mRateEnabled);
    Serial.print(", Off Speed Enabled is "); Serial.print(recordingFormatData.offSpeedEnabled);
    Serial.print(", Interlaced Enabled is "); Serial.print(recordingFormatData.interlacedEnabled);
    Serial.print(", Windowed Mode Enabled is "); Serial.println(recordingFormatData.windowedModeEnabled);

    if(recordingFormatData.mRateEnabled && recordingFormatData.frameRate == 24) // && !recordingFormatData.offSpeedEnabled)
        Serial.println("Frame Rate will be 23.98");
    else if(recordingFormatData.mRateEnabled && recordingFormatData.frameRate == 30) //  && !recordingFormatData.offSpeedEnabled)
        Serial.println("Frame Rate will be 29.97");
    else if(recordingFormatData.mRateEnabled && recordingFormatData.frameRate == 60) //  && !recordingFormatData.offSpeedEnabled)
        Serial.println("Frame Rate will be 59.94");

    /*
    DEBUG_DEBUG("Recording Format, Width x Height [Windowed] [interlaced] [mrate] [sensor mrate] [offspeed]: %i x %i [%s] [%s] [%s] [%s] [%s]", recordingFormatData.width, recordingFormatData.height, recordingFormatData.windowedModeEnabled ? "Yes" : "No", recordingFormatData.interlacedEnabled ? "Yes" : "No", recordingFormatData.mRateEnabled ? "Yes" : "No", recordingFormatData.sensorMRateEnabled ? "Yes" : "No", recordingFormatData.offSpeedEnabled ? "Yes" : "No");

    // When sending commands, debug.
    DEBUG_DEBUG("DecodeRecordingFormat (flags at index 4): ");
    for(int index = 0; index < data.size(); index++)
    {
        DEBUG_DEBUG("%i: %i", index, data[index]);
        
    }
    */

   BMDControlSystem::getInstance()->getCamera()->onRecordingFormatReceived(recordingFormatData);
}

void CCUDecodingFunctions::DecodeAutoExposureMode(std::vector<byte> inData)
{
    std::vector<sbyte> data = ConvertPayloadDataWithExpectedCount<sbyte>(inData, 1);
    CCUPacketTypes::AutoExposureMode autoExposureMode = static_cast<CCUPacketTypes::AutoExposureMode>(data[0]);
   
   BMDControlSystem::getInstance()->getCamera()->onAutoExposureModeReceived(autoExposureMode);
}

void CCUDecodingFunctions::DecodeShutterAngle(std::vector<byte> inData)
{
    std::vector<int32_t> data = ConvertPayloadDataWithExpectedCount<int32_t>(inData, 1);
    int32_t shutterAngleX100 = data[0];

    BMDControlSystem::getInstance()->getCamera()->onShutterAngleReceived(shutterAngleX100);
}

void CCUDecodingFunctions::DecodeShutterSpeed(std::vector<byte> inData)
{
    std::vector<int32_t> data = ConvertPayloadDataWithExpectedCount<int32_t>(inData, 1);
    int32_t shutterSpeed = data[0]; // Result is the denominator in 1/X, e.g. shutterSpeed = 24 is a shutter speed of 1/24

    BMDControlSystem::getInstance()->getCamera()->onShutterSpeedReceived(shutterSpeed);
}

void CCUDecodingFunctions::DecodeGain(std::vector<byte> inData)
{
    std::vector<byte> data = ConvertPayloadDataWithExpectedCount<byte>(inData, 1);
    byte gain = data[0];

    BMDControlSystem::getInstance()->getCamera()->onSensorGainDBReceived(gain);
}

void CCUDecodingFunctions::DecodeISO(std::vector<byte> inData)
{
    std::vector<int32_t> data = ConvertPayloadDataWithExpectedCount<int32_t>(inData, 1);
    int32_t iso = data[0];

    BMDControlSystem::getInstance()->getCamera()->onSensorGainISOValueReceived(iso);
}

void CCUDecodingFunctions::DecodeDisplayLUT(std::vector<byte> inData)
{
    std::vector<byte> data = ConvertPayloadDataWithExpectedCount<byte>(inData, 2);
    CCUPacketTypes::SelectedLUT selectedLut = static_cast<CCUPacketTypes::SelectedLUT>(data[0]);
    bool enabled = data[1] == 1;

   BMDControlSystem::getInstance()->getCamera()->onSelectedLUTReceived(selectedLut);
   BMDControlSystem::getInstance()->getCamera()->onSelectedLUTEnabledReceived(enabled);
}

void CCUDecodingFunctions::DecodeStatusCategory(byte parameter, std::vector<byte> payloadData)
{
    if(CCUUtility::byteValueExistsInArray(CCUPacketTypes::StatusParameterValues, sizeof(CCUPacketTypes::StatusParameterValues) / sizeof(CCUPacketTypes::StatusParameterValues[0]), parameter))
    {
        CCUPacketTypes::StatusParameter parameterType = static_cast<CCUPacketTypes::StatusParameter>(parameter);

        switch (parameterType)
        {
            case CCUPacketTypes::StatusParameter::Battery:
                // Not catered for
                // Attempting to cater for it
                DecodeBattery(payloadData);
                break;
            case CCUPacketTypes::StatusParameter::CameraSpec:
                // Not catered for
                DecodeCameraSpec(payloadData);
                break;
            case CCUPacketTypes::StatusParameter::DisplayParameters:
                // Not catered for
                // Serial.print("DecodeStatusCategory, ParameterType DisplayParameters not catered for: "); Serial.println(static_cast<byte>(parameterType));
                break;
            case CCUPacketTypes::StatusParameter::DisplayThresholds:
                // Not catered for
                // Serial.print("DecodeStatusCategory, ParameterType DisplayThresholds not catered for: "); Serial.println(static_cast<byte>(parameterType));
                break;
            case CCUPacketTypes::StatusParameter::DisplayTimecode:
                // Not catered for
                DEBUG_VERBOSE("DecodeStatusCategory, ParameterType DisplayTimecode not catered for: %i", static_cast<byte>(parameterType));
                break;
            case CCUPacketTypes::StatusParameter::MediaStatus:
                DecodeMediaStatus(payloadData);
                break;
            case CCUPacketTypes::StatusParameter::RemainingRecordTime:
                DecodeRemainingRecordTime(payloadData);
                break;
            case CCUPacketTypes::StatusParameter::SwitcherStatus:
                // Not catered for
                // Serial.print("DecodeStatusCategory, ParameterType SwitcherStatus not catered for: "); Serial.println(static_cast<byte>(parameterType));
                break;
            case CCUPacketTypes::StatusParameter::NewParameterTBD:
                DEBUG_VERBOSE("DecodeStatusCategory, ParameterType ID %i is new. Payload Byte Count is %i. Two bytes are %i and %i", static_cast<byte>(parameterType), GetCount<byte>(payloadData), payloadData[0], payloadData[1]);
                break;
            default:
                DEBUG_VERBOSE("DecodeStatusCategory, ParameterType #%i not catered for", static_cast<byte>(parameterType));
                break;
        }
    }
    else
        throw "Invalid value for Status Parameter.";
}

void CCUDecodingFunctions::DecodeBattery(std::vector<byte> inData)
{
    std::vector<short> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, 3);

    CCUPacketTypes::BatteryStatusData batteryStatusData;
    batteryStatusData.batteryLevelX1000 = data[0];

    // Not sure what data[1] is - value is typically 100

    ushort flags = (ushort)data[2];

    batteryStatusData.batteryPresent = static_cast<bool>((static_cast<int>(flags) & static_cast<int>(CCUPacketTypes::BatteryStatus::BatteryPresent)) > 0);
    batteryStatusData.ACPresent = static_cast<bool>((static_cast<int>(flags) & static_cast<int>(CCUPacketTypes::BatteryStatus::ACPresent)) > 0);
    batteryStatusData.batteryIsCharging = static_cast<bool>((static_cast<int>(flags) & static_cast<int>(CCUPacketTypes::BatteryStatus::BatteryIsCharging)) > 0);
    batteryStatusData.chargeRemainingPercentageIsEstimated = static_cast<bool>((static_cast<int>(flags) & static_cast<int>(CCUPacketTypes::BatteryStatus::ChargeRemainingPercentageIsEstimated)) > 0);
    batteryStatusData.preferVoltageDisplay = static_cast<bool>((static_cast<int>(flags) & static_cast<int>(CCUPacketTypes::BatteryStatus::PreferVoltageDisplay)) > 0);

    /* Runs almost once a second, so only display if needed.
    Serial.print("Decoded Battery Level is "); Serial.print(batteryStatusData.batteryLevelX1000);
    Serial.print(", Battery Present is "); Serial.print(batteryStatusData.batteryPresent);
    Serial.print(", AC Present is "); Serial.print(batteryStatusData.ACPresent);
    Serial.print(", Battery is Charging is "); Serial.print(batteryStatusData.batteryIsCharging);
    Serial.print(", Charge Remaining Percentage is Estimated is "); Serial.print(batteryStatusData.chargeRemainingPercentageIsEstimated);
    Serial.print(", Prefer Voltage Display is "); Serial.println(batteryStatusData.preferVoltageDisplay);
    */

   BMDControlSystem::getInstance()->getCamera()->onBatteryReceived(batteryStatusData);
}

void CCUDecodingFunctions::DecodeCameraSpec(std::vector<byte> inData)
{
    std::vector<byte> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<byte>(inData, 4);

    if(data[1] < CameraModels::NumberOfCameraModels)
    {
        CameraModel cameraModel = CameraModels::fromValue(data[1]); // This is the camera model, 14 = Pocket 6K.

        std::string cameraName = CameraModels::modelToName.at(cameraModel);

        // Update Camera object
        BMDControlSystem::getInstance()->getCamera()->onModelNameReceived(cameraName);
        BMDControlSystem::getInstance()->getCamera()->onIsPocketReceived(CameraModels::isPocket(cameraModel));

        std::string setName = BMDControlSystem::getInstance()->getCamera()->getModelName();
    }
    else
    {
        BMDControlSystem::getInstance()->getCamera()->onModelNameReceived("UNKNOWN CAMERA MODEL");
    }
}

void CCUDecodingFunctions::DecodeMediaStatus(std::vector<byte> inData)
{
    std::vector<sbyte> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<sbyte>(inData, inData.size()); // We convert all the bytes to sbyte
    int slotCount = data.size();

    std::vector<CCUPacketTypes::MediaStatus> mediaStatuses;

    for(int index = 0; index < slotCount; index++)
    {
        CCUPacketTypes::MediaStatus slotMediaStatus = static_cast<CCUPacketTypes::MediaStatus>(data[index]);

        mediaStatuses.push_back(slotMediaStatus);
    }

    BMDControlSystem::getInstance()->getCamera()->onMediaStatusReceived(mediaStatuses);
}

// For DecodeRemainingRecordTime function
CCUDecodingFunctions::SecondsWithOverflow CCUDecodingFunctions::simplifyTime(int16_t time) {
    int16_t seconds = time; // positive value -> unit: second
    bool over = false;

    if (time < 0) { // negative value -> unit: minute
        uint16_t minutes = 0;
        if (time == INT16_MIN) { // more time remaining than fits in CCU message
            minutes = UINT16_MAX;
            over = true;
        } else {
            minutes = -time;
        }
        seconds = minutes * 60;
    }

    SecondsWithOverflow returnValue;
    returnValue.seconds = static_cast<uint16_t>(seconds);
    returnValue.over = over;

    return returnValue;
}

// For DecodeRemainingRecordTime function
std::string CCUDecodingFunctions::makeTimeLabel(SecondsWithOverflow time) {
    if (time.seconds == 0) { return "Transport Full"; }

    std::string label = "";
    uint16_t hours = time.seconds / 60 / 60;
    if (hours > 0) {
        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << hours; // Leading zeros, format to 00
        label += ss.str().c_str();
        label += ":";
    }

    uint16_t minutes = time.seconds / 60 % 60;

    std::stringstream ssmin;
    ssmin << std::setw(2) << std::setfill('0') << minutes; // Leading zeros, format to 00
    label += ssmin.str().c_str();
    label += ":";

    uint16_t seconds = time.seconds % 60;

    std::stringstream sssec;
    sssec << std::setw(2) << std::setfill('0') << seconds; // Leading zeros, format to 00
    label += sssec.str().c_str();

    if (time.over) { label += "+"; }

    return label;
}

void CCUDecodingFunctions::DecodeRemainingRecordTime(std::vector<byte> inData)
{
    std::vector<ccu_fixed_t> payload = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<ccu_fixed_t>(inData, inData.size() / 2);

    int slotCount = payload.size();
    std::vector<std::string> labels(slotCount, "");
    std::vector<ccu_fixed_t> minutes(slotCount, 0);
    for (int slotIndex = 0; slotIndex < slotCount; ++slotIndex) {
        SecondsWithOverflow remainingTime = simplifyTime(payload[slotIndex]);
        minutes[slotIndex] = remainingTime.seconds / 60;
        labels[slotIndex] = makeTimeLabel(remainingTime);

        // Serial.print("Decoded Remaining Record Time Slot #"); Serial.print(slotIndex); Serial.print(" is "); Serial.print(minutes[slotIndex]); Serial.print(" minutes, formatted as "); Serial.println(labels[slotIndex]);
    }

    BMDControlSystem::getInstance()->getCamera()->onRemainingRecordTimeMinsReceived(minutes);
    BMDControlSystem::getInstance()->getCamera()->onRemainingRecordTimeStringReceived(labels);
}


void CCUDecodingFunctions::DecodeMediaCategory(byte parameter, std::vector<byte> payloadData)
{
    if(CCUUtility::byteValueExistsInArray(CCUPacketTypes::MediaParameterValues, sizeof(CCUPacketTypes::MediaParameterValues) / sizeof(CCUPacketTypes::MediaParameterValues[0]), parameter))
    {
        CCUPacketTypes::MediaParameter parameterType = static_cast<CCUPacketTypes::MediaParameter>(parameter);

        switch (parameterType)
        {
            case CCUPacketTypes::MediaParameter::Codec:
                DecodeCodec(payloadData);
                break;
            case CCUPacketTypes::MediaParameter::TransportMode:
                DecodeTransportMode(payloadData);
                break;
        default:
            break;
        }
    }
    else
        throw "Invalid value for Media Category Parameter.";
}

void CCUDecodingFunctions::DecodeCodec(std::vector<byte> inData)
{
    std::vector<byte> data = ConvertPayloadDataWithExpectedCount<byte>(inData, 2);

    CodecInfo codecInfo(static_cast<CCUPacketTypes::BasicCodec>(data[0]), data[1]);

    BMDControlSystem::getInstance()->getCamera()->onCodecReceived(codecInfo);
}

void CCUDecodingFunctions::DecodeTransportMode(std::vector<byte> inData)
{
    std::vector<sbyte> data = ConvertPayloadDataWithExpectedCount<sbyte>(inData, inData.size());

    TransportInfo transportInfo;

    transportInfo.mode = static_cast<CCUPacketTypes::MediaTransportMode>(data[0]);
    transportInfo.speed = static_cast<sbyte>(data[1]);

    /*
    switch(transportInfo.mode)
    {
        case CCUPacketTypes::MediaTransportMode::Play:
            Serial.println("Playing.");
            break;
        case CCUPacketTypes::MediaTransportMode::Preview:
            Serial.println("Preview.");
            break;
        case CCUPacketTypes::MediaTransportMode::Record:
            Serial.println("Recording.");
            break;
    }
    */
   
    byte flags = static_cast<byte>(data[2]);

    transportInfo.loop = static_cast<bool>((static_cast<int>(flags) & static_cast<int>(CCUPacketTypes::MediaTransportFlag::Loop)) > 0);
    transportInfo.playAll = static_cast<bool>((static_cast<int>(flags) & static_cast<int>(CCUPacketTypes::MediaTransportFlag::PlayAll)) > 0);
    transportInfo.timelapseRecording = static_cast<bool>((static_cast<int>(flags) & static_cast<int>(CCUPacketTypes::MediaTransportFlag::TimelapseRecording)) > 0);

    // The remaining data is for storage slots
    int slotCount = data.size() - 3;
    transportInfo.slots = std::vector<TransportInfo::TransportInfoSlot>(slotCount, TransportInfo::TransportInfoSlot());
    
    for (int i = 0; i < transportInfo.slots.size(); i++)
    {
        transportInfo.slots[i].active = flags & CCUPacketTypes::slotActiveMasks[i];
        transportInfo.slots[i].medium = static_cast<CCUPacketTypes::ActiveStorageMedium>(data[i + 3]);
    }

    BMDControlSystem::getInstance()->getCamera()->onTransportModeReceived(transportInfo);
}

void CCUDecodingFunctions::DecodeMetadataCategory(byte parameter, std::vector<byte> payloadData)
{
    if(CCUUtility::byteValueExistsInArray(CCUPacketTypes::MetadataParameterValues, sizeof(CCUPacketTypes::MetadataParameterValues) / sizeof(CCUPacketTypes::MetadataParameterValues[0]), parameter))
    {
        CCUPacketTypes::MetadataParameter parameterType = static_cast<CCUPacketTypes::MetadataParameter>(parameter);

        switch (parameterType)
        {
            case CCUPacketTypes::MetadataParameter::Reel:
                DecodeReel(payloadData);
                break;
            case CCUPacketTypes::MetadataParameter::SceneTags:
                DecodeSceneTags(payloadData);
                break;
            case CCUPacketTypes::MetadataParameter::Scene:
                DecodeScene(payloadData);
                break;
            case CCUPacketTypes::MetadataParameter::Take:
                DecodeTake(payloadData);
                break;
            case CCUPacketTypes::MetadataParameter::GoodTake:
                DecodeGoodTake(payloadData);
                break;
            case CCUPacketTypes::MetadataParameter::CameraId:
                DecodeCameraId(payloadData);
                break;
            case CCUPacketTypes::MetadataParameter::CameraOperator:
                DecodeCameraOperator(payloadData);
                break;
            case CCUPacketTypes::MetadataParameter::Director:
                DecodeDirector(payloadData);
                break;
            case CCUPacketTypes::MetadataParameter::ProjectName:
                DecodeProjectName(payloadData);
                break;
            case CCUPacketTypes::MetadataParameter::SlateForType:
                DecodeSlateForType(payloadData);
                break;
            case CCUPacketTypes::MetadataParameter::SlateForName:
                DecodeSlateForName(payloadData);
                break;
            case CCUPacketTypes::MetadataParameter::LensFocalLength:
                DecodeLensFocalLength(payloadData);
                break;
            case CCUPacketTypes::MetadataParameter::LensDistance:
                DecodeLensDistance(payloadData);
                break;
            case CCUPacketTypes::MetadataParameter::LensType:
                DecodeLensType(payloadData);
                break;
            case CCUPacketTypes::MetadataParameter::LensIris:
                DecodeLensIris(payloadData);
                break;
            default:
                DEBUG_VERBOSE("DecodeMetadataCategory, ParameterType not known: %i", static_cast<byte>(parameterType));
                break;
        }
    }
    else
        throw "Invalid value for Metadata Parameter.";
}

void CCUDecodingFunctions::DecodeReel(std::vector<byte> inData)
{
    int typeCount = GetCount<short>(inData);

    if(typeCount == 1)
    {
        // Firmware 7.9.1 and older
        std::vector<short> data = ConvertPayloadDataWithExpectedCount<short>(inData, 1);
        short reelNumber = data[0];

        BMDControlSystem::getInstance()->getCamera()->onReelNumberReceived(reelNumber, true);
    }
    else if(typeCount == 2)
    {
        // Firmware 8.1
        std::vector<short> data = ConvertPayloadDataWithExpectedCount<short>(inData, 2);
        short reelNumber = data[0];
        bool editable = data[1] != 0;

        BMDControlSystem::getInstance()->getCamera()->onReelNumberReceived(reelNumber, editable);
    }
}

void CCUDecodingFunctions::DecodeScene(std::vector<byte> inData)
{
    std::string sceneString = CCUDecodingFunctions::ConvertPayloadDataToString(inData);

    BMDControlSystem::getInstance()->getCamera()->onSceneNameReceived(sceneString);
}

void CCUDecodingFunctions::DecodeSceneTags(std::vector<byte> inData)
{
    std::vector<sbyte> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<sbyte>(inData, 3);
    CCUPacketTypes::MetadataSceneTag sceneTag = static_cast<CCUPacketTypes::MetadataSceneTag>(data[0]);
    CCUPacketTypes::MetadataLocationTypeTag locationType = static_cast<CCUPacketTypes::MetadataLocationTypeTag>(data[1]);
    CCUPacketTypes::MetadataDayNightTag dayOrNight = static_cast<CCUPacketTypes::MetadataDayNightTag>(data[2]);

    BMDControlSystem::getInstance()->getCamera()->onSceneTagReceived(sceneTag);
    BMDControlSystem::getInstance()->getCamera()->onLocationTypeReceived(locationType);
    BMDControlSystem::getInstance()->getCamera()->onDayOrNightReceived(dayOrNight);
}

void CCUDecodingFunctions::DecodeTake(std::vector<byte> inData)
{
    std::vector<sbyte> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<sbyte>(inData, 2);
    sbyte takeNumber = data[0];
    CCUPacketTypes::MetadataTakeTag takeTag = static_cast<CCUPacketTypes::MetadataTakeTag>(data[1]);

    BMDControlSystem::getInstance()->getCamera()->onTakeTagReceived(takeTag);
    BMDControlSystem::getInstance()->getCamera()->onTakeNumberReceived(takeNumber);
}

void CCUDecodingFunctions::DecodeGoodTake(std::vector<byte> inData)
{
    std::vector<sbyte> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<sbyte>(inData, 1);
    sbyte goodTake = data[0];

    BMDControlSystem::getInstance()->getCamera()->onGoodTakeReceived(goodTake);
}

void CCUDecodingFunctions::DecodeCameraId(std::vector<byte> inData)
{
    std::string cameraId = CCUDecodingFunctions::ConvertPayloadDataToString(inData);

    BMDControlSystem::getInstance()->getCamera()->onCameraIdReceived(cameraId);
}

void CCUDecodingFunctions::DecodeCameraOperator(std::vector<byte> inData)
{
    std::string cameraOperator = CCUDecodingFunctions::ConvertPayloadDataToString(inData);

    BMDControlSystem::getInstance()->getCamera()->onCameraOperatorReceived(cameraOperator);
}

void CCUDecodingFunctions::DecodeDirector(std::vector<byte> inData)
{
    std::string director = CCUDecodingFunctions::ConvertPayloadDataToString(inData);

    BMDControlSystem::getInstance()->getCamera()->onDirectorReceived(director);
}

void CCUDecodingFunctions::DecodeProjectName(std::vector<byte> inData)
{
    std::string projectName = CCUDecodingFunctions::ConvertPayloadDataToString(inData);

    BMDControlSystem::getInstance()->getCamera()->onProjectNameReceived(projectName);
}


void CCUDecodingFunctions::DecodeSlateForType(std::vector<byte> inData)
{
    std::vector<sbyte> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<sbyte>(inData, 1);
    CCUPacketTypes::MetadataSlateForType slateForType = static_cast<CCUPacketTypes::MetadataSlateForType>(data[0]);

    BMDControlSystem::getInstance()->getCamera()->onSlateTypeReceived(slateForType);
}

void CCUDecodingFunctions::DecodeSlateForName(std::vector<byte> inData)
{
    std::string name = CCUDecodingFunctions::ConvertPayloadDataToString(inData);

    // NOTE: Camera software versions between 7.5 and 7.7.x send the full path of
    // the clip file name. When this is detected, strip the system folders, first
    // folder (volume), and final file extension.
    std::regex re("^/mnt/s[0-9]+/");
    std::smatch match;
    if (std::regex_search(name, match, re)) {
        name = name.substr(match.position() + match.length());
    }

    BMDControlSystem::getInstance()->getCamera()->onSlateNameReceived(name);
}

void CCUDecodingFunctions::DecodeLensFocalLength(std::vector<byte> inData)
{
    std::string lensFocalLength = CCUDecodingFunctions::ConvertPayloadDataToString(inData);

    // DecodeLensFocalLength Lens Focal Length: 65mm
    // DEBUG_DEBUG("DecodeLensFocalLength %s", lensFocalLength.c_str());

    BMDControlSystem::getInstance()->getCamera()->onLensFocalLengthReceived(lensFocalLength);
}

void CCUDecodingFunctions::DecodeLensDistance(std::vector<byte> inData)
{
    std::string lensDistance = ConvertPayloadDataToString(inData);

    // DecodeLensFocalDistance Lens Distance: Inf
    // DecodeLensFocalDistance Lens Distance: 26100mm to 41310mm
    // DEBUG_DEBUG("DecodeLensDistance %s", lensDistance.c_str());

    BMDControlSystem::getInstance()->getCamera()->onLensDistanceReceived(lensDistance);
}

void CCUDecodingFunctions::DecodeLensType(std::vector<byte> inData)
{
    std::string lensType = ConvertPayloadDataToString(inData);

    // DecodeLensType Lens Type: Canon EF-S 55-250mm f/4-5.6 IS
    // "DecodeLensType Lens Type:" <-- this shows when there's no info between camera and lens.

    // DEBUG_DEBUG("DecodeLensType %s", lensType.c_str());

    BMDControlSystem::getInstance()->getCamera()->onLensTypeReceived(lensType);
}

void CCUDecodingFunctions::DecodeLensIris(std::vector<byte> inData)
{
    std::string lensIris = ConvertPayloadDataToString(inData);

    // DEBUG_DEBUG("DecodeLensIris %s", lensIris.c_str());

    BMDControlSystem::getInstance()->getCamera()->onLensIrisReceived(lensIris);
}

// DISPLAY CATEGORY

void CCUDecodingFunctions::DecodeDisplayCategory(byte parameter, std::vector<byte> payloadData)
{
    if(CCUUtility::byteValueExistsInArray(CCUPacketTypes::DisplayParameterValues, sizeof(CCUPacketTypes::DisplayParameterValues) / sizeof(CCUPacketTypes::MetadataParameterValues[0]), parameter))
    {
        CCUPacketTypes::DisplayParameter parameterType = static_cast<CCUPacketTypes::DisplayParameter>(parameter);

        switch (parameterType)
        {
            case CCUPacketTypes::DisplayParameter::TimecodeSource:
                DecodeTimecodeSource(payloadData);
                break;
            case CCUPacketTypes::DisplayParameter::Overlays:
                // Not handled.
                break;
            case CCUPacketTypes::DisplayParameter::ZebraLevel:
                // Not handled.
                break;
            case CCUPacketTypes::DisplayParameter::PeakingLevel:
                // Not handled.
                break;
            case CCUPacketTypes::DisplayParameter::ColourBars:
                // Not handled.
                break;
            case CCUPacketTypes::DisplayParameter::FocusAssist:
                // Not handled.
                break;
            default:
                DEBUG_VERBOSE("DecodeDisplayCategory, ParameterType not known: %i", static_cast<byte>(parameterType));
                break;
        }
    }
    else
        throw "Invalid value for Display Parameter.";
}

void CCUDecodingFunctions::DecodeTimecodeSource(std::vector<byte> inData)
{
    std::vector<sbyte> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<sbyte>(inData, 1);
    CCUPacketTypes::DisplayTimecodeSource timecodeSource = static_cast<CCUPacketTypes::DisplayTimecodeSource>(data[0]);

    BMDControlSystem::getInstance()->getCamera()->onTimecodeSourceReceived(timecodeSource);
}


// Timecode decoding
const char* kTimecodeDigits[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
const int kTimecodeSize = sizeof(uint32_t);
const uint32_t kTimecodeDropFrameMask = 0x80000000;
void CCUDecodingFunctions::TimecodeToString(std::vector<byte> timecodeBytes)
{
    uint32_t timecode = ((uint32_t)timecodeBytes[3] << 24) |
                      ((uint32_t)timecodeBytes[2] << 16) |
                      ((uint32_t)timecodeBytes[1] << 8) |
                      (uint32_t)timecodeBytes[0];

    std::string str = "";
    int digitCount = kTimecodeSize * 2;
    int shift = kTimecodeSize * 8 - 4;
    bool dropFrame = (timecode & kTimecodeDropFrameMask) > 0;

    // Convert BCD timecode to string
    for (int i = 0; i < digitCount; i++) {
        int mask = (i == 0) ? 0x3 : 0xF;
        int digit = (int)(timecode >> shift) & mask;

        if (digit < 10) {
            str += kTimecodeDigits[digit];
        } else {
            str += "-";
        }

        if ((i % 2 == 1) && (i < digitCount - 1)) {
            str += (dropFrame && i >= 5) ? ";" : ":";
        }

        shift -= 4;
    }

    BMDControlSystem::getInstance()->getCamera()->onTimecodeReceived(str);
}