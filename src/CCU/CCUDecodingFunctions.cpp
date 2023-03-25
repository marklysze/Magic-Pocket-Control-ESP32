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
        // Serial.print("DecodePayloadData, Category: "); Serial.println(static_cast<byte>(category));
    }

    switch (category) {
        case CCUPacketTypes::Category::Lens:
            DecodeLensCategory(parameter, payloadData, payloadDataLength);
            break;
        case CCUPacketTypes::Category::Video:
            DecodeVideoCategory(parameter, payloadData, payloadDataLength);
            break;
        case CCUPacketTypes::Category::Status:
            DecodeStatusCategory(parameter, payloadData, payloadDataLength);
            break;
        case CCUPacketTypes::Category::Media:
            DecodeMediaCategory(parameter, payloadData, payloadDataLength);
            break;
        case CCUPacketTypes::Category::Metadata:
            DecodeMetadataCategory(parameter, payloadData, payloadDataLength);
            break;
        default:
            break;
    }
}

// implementation of member functions
void CCUDecodingFunctions::DecodeLensCategory(byte parameter, byte* payloadData, int payloadLength)
{
    if(CCUUtility::byteValueExistsInArray(CCUPacketTypes::LensParameterValues, sizeof(CCUPacketTypes::LensParameterValues) / sizeof(CCUPacketTypes::LensParameterValues[0]), parameter))
    {
        CCUPacketTypes::LensParameter parameterType = static_cast<CCUPacketTypes::LensParameter>(parameter);

        // Serial.print("DecodeLensCategory, ParameterType: "); Serial.println(static_cast<byte>(parameterType));

        switch (parameterType)
        {
        case CCUPacketTypes::LensParameter::ApertureFstop:
            DecodeApertureFStop(payloadData, payloadLength);
            break;
        case CCUPacketTypes::LensParameter::ApertureNormalised:
            DecodeApertureNormalised(payloadData, payloadLength);
            break;
        case CCUPacketTypes::LensParameter::ApertureOrdinal:
            // Not catered for
            Serial.print("DecodeLensCategory, ParameterType ApertureOrdinal not catered for: "); Serial.println(static_cast<byte>(parameterType));
            break;
        case CCUPacketTypes::LensParameter::AutoAperture:
            // Not catered for
            Serial.print("DecodeLensCategory, ParameterType AutoAperture not catered for: "); Serial.println(static_cast<byte>(parameterType));
            break;
        case CCUPacketTypes::LensParameter::AutoFocus:
            DecodeAutoFocus(payloadData, payloadLength);
            break;
        case CCUPacketTypes::LensParameter::ContinuousZoom:
            // Not catered for
            Serial.print("DecodeLensCategory, ParameterType ContinuousZoom not catered for: "); Serial.println(static_cast<byte>(parameterType));
            break;
        case CCUPacketTypes::LensParameter::Focus:
            // Not catered for
            Serial.print("DecodeLensCategory, ParameterType Focus not catered for: "); Serial.println(static_cast<byte>(parameterType));
            break;
        case CCUPacketTypes::LensParameter::ImageStabilisation:
            // Not catered for
            Serial.print("DecodeLensCategory, ParameterType ImageStabilisation not catered for: "); Serial.println(static_cast<byte>(parameterType));
            break;
        case CCUPacketTypes::LensParameter::Zoom:
            DecodeZoom(payloadData, payloadLength);
            break;
        case CCUPacketTypes::LensParameter::ZoomNormalised:
            // Not catered for
            Serial.print("DecodeLensCategory, ParameterType ZoomNormalised not catered for: "); Serial.println(static_cast<byte>(parameterType));
            break;
        default:
            break;
        }
    }
    else
        throw "Invalid value for Lens Parameter.";
}

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

std::string CCUDecodingFunctions::ConvertPayloadDataToString(byte* data, int byteCount)
{
    return std::string(reinterpret_cast<const char*>(data), byteCount);
}

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

void CCUDecodingFunctions::DecodeApertureFStop(byte* inData, int inDataLength)
{
    std::vector<ccu_fixed_t> data;
    LensConfig::ApertureUnits apertureUnits = LensConfig::ApertureUnits::Fstops;
    
    if(inDataLength == 4)
    {
        data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<ccu_fixed_t>(inData, inDataLength, 2); // First two bytes are the aperture value and the last two are the aperture units (FStop = 0, TStop = 1)
        apertureUnits = static_cast<LensConfig::ApertureUnits>(data[1]);
    }
    else
    {
        // We may only get the fstops not the F stops vs T stops component.
        data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<ccu_fixed_t>(inData, inDataLength, 1); // Two bytes are the aperture value
    }

    ccu_fixed_t apertureNumber = data[0];

    if(apertureNumber != CCUPacketTypes::kLensAperture_NoLens)
    {
        Serial.print("Decoded fStopIndex Aperture is "); Serial.println(LensConfig::GetFStopString(ConvertCCUApertureToFstop(apertureNumber), apertureUnits).c_str());
    }
    else
        Serial.println("Decoded fStopIndex: No Lens Information");
}

void CCUDecodingFunctions::DecodeApertureNormalised(byte* inData, int inDataLength)
{
    std::vector<short> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, inDataLength, 1); // Receiving a fixed16 number, as a short it will be 0-2,048 representing 0.0-1.0
    short apertureNormalisedNumber = data[0];

    if(apertureNormalisedNumber != CCUPacketTypes::kLensAperture_NoLens)
    {
        float apertureValue = static_cast<float>(apertureNormalisedNumber) / 2048.0f; // Convert to float and perform division
        std::string apertureString = "Decode Aperture Normalised: " + std::to_string(static_cast<int>(apertureValue * 100.0f)) + "%"; // Multiply by 100 to get percentage and convert to string

        Serial.println(apertureString.c_str());
    }
    else
        Serial.println("Decoded Aperture Normalized: No Lens Information");
}

void CCUDecodingFunctions::DecodeAutoFocus(byte* inData, int inDataLength)
{
    bool instantaneousAutoFocusPressed = true;
}

void CCUDecodingFunctions::DecodeZoom(byte* inData, int inDataLength)
{
    // ccu_fixed_t
    std::vector<ccu_fixed_t> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, inDataLength, 1);
    ccu_fixed_t focalLengthMM = data[0];

    if(focalLengthMM != 0)
    {
        Serial.print("Decode Zoom Focal Length: "); Serial.println(focalLengthMM);
    }
    else
        Serial.println("Decode Zoom: No Lens Information");
}


void CCUDecodingFunctions::DecodeVideoCategory(byte parameter, byte* payloadData, int payloadLength)
{
    if(CCUUtility::byteValueExistsInArray(CCUPacketTypes::VideoParameterValues, sizeof(CCUPacketTypes::VideoParameterValues) / sizeof(CCUPacketTypes::VideoParameterValues[0]), parameter))
    {
        CCUPacketTypes::VideoParameter parameterType = static_cast<CCUPacketTypes::VideoParameter>(parameter);

        // Serial.print("DecodeVideoCategory, ParameterType: "); Serial.println(static_cast<byte>(parameterType));

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
                DecodeAutoExposureMode(payloadData, payloadLength);
                break;
            case CCUPacketTypes::VideoParameter::ShutterAngle:
                DecodeShutterAngle(payloadData, payloadLength);
                break;
            case CCUPacketTypes::VideoParameter::ShutterSpeed:
                DecodeShutterSpeed(payloadData, payloadLength);
                break;
            case CCUPacketTypes::VideoParameter::Gain:
                DecodeGain(payloadData, payloadLength);
                break;
            case CCUPacketTypes::VideoParameter::ISO:
                DecodeISO(payloadData, payloadLength);
                break;
            case CCUPacketTypes::VideoParameter::DisplayLUT:
                DecodeDisplayLUT(payloadData, payloadLength);
                break;
        default:
            break;
        }
    }
    else
        throw "Invalid value for Video Parameter.";
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

void CCUDecodingFunctions::DecodeAutoExposureMode(byte* inData, int inDataLength)
{
    std::vector<sbyte> data = ConvertPayloadDataWithExpectedCount<sbyte>(inData, inDataLength, 1);
    CCUPacketTypes::AutoExposureMode autoExposureMode = static_cast<CCUPacketTypes::AutoExposureMode>(data[0]);

    Serial.print("Decoded Auto Exposure Mode: ");
    switch(autoExposureMode)
    {
        case CCUPacketTypes::AutoExposureMode::Manual:
            Serial.println("Manual");
            break;
        case CCUPacketTypes::AutoExposureMode::Iris:
            Serial.println("Iris");
            break;
        case CCUPacketTypes::AutoExposureMode::Shutter:
            Serial.println("Shutter");
            break;
        case CCUPacketTypes::AutoExposureMode::IrisAndShutter:
            Serial.println("IrisAndShutter");
            break;
        case CCUPacketTypes::AutoExposureMode::ShutterAndIris:
            Serial.println("ShutterAndIris");
            break;
    }
}

void CCUDecodingFunctions::DecodeShutterAngle(byte* inData, int inDataLength)
{
    std::vector<int32_t> data = ConvertPayloadDataWithExpectedCount<int32_t>(inData, inDataLength, 1);
    int32_t shutterAngleX100 = data[0];

    Serial.print("Decoded Exposure (Shutter Angle x 100): "); Serial.println(shutterAngleX100);
}

void CCUDecodingFunctions::DecodeShutterSpeed(byte* inData, int inDataLength)
{
    std::vector<int32_t> data = ConvertPayloadDataWithExpectedCount<int32_t>(inData, inDataLength, 1);
    int32_t shutterSpeed = data[0]; // Result is the denominator in 1/X, e.g. shutterSpeed = 24 is a shutter speed of 1/24

    Serial.print("Decoded Exposure (Shutter Speed): "); Serial.println(shutterSpeed);
}

void CCUDecodingFunctions::DecodeGain(byte* inData, int inDataLength)
{
    std::vector<byte> data = ConvertPayloadDataWithExpectedCount<byte>(inData, inDataLength, 1);
    byte gain = data[0];

    Serial.print("Decoded Gain): "); Serial.println(gain);
}

void CCUDecodingFunctions::DecodeISO(byte* inData, int inDataLength)
{
    std::vector<int32_t> data = ConvertPayloadDataWithExpectedCount<int32_t>(inData, inDataLength, 1);
    int32_t iso = data[0];

    Serial.print("Decoded ISO: "); Serial.println(iso);
}

void CCUDecodingFunctions::DecodeDisplayLUT(byte* inData, int inDataLength)
{
    std::vector<byte> data = ConvertPayloadDataWithExpectedCount<byte>(inData, inDataLength, 2);
    CCUPacketTypes::SelectedLUT selectedLut = static_cast<CCUPacketTypes::SelectedLUT>(data[0]);
    bool enabled = data[1] == 1;

    Serial.print("Decoded Display LUT: ");
    switch(selectedLut)
    {
        case CCUPacketTypes::SelectedLUT::None:
            Serial.print("None");
            break;
        case CCUPacketTypes::SelectedLUT::Custom:
            Serial.print("Custom");
            break;
        case CCUPacketTypes::SelectedLUT::FilmToVideo:
            Serial.print("FilmToVideo");
            break;
        case CCUPacketTypes::SelectedLUT::FilmToExtendedVideo:
            Serial.print("FilmToExtendedVideo");
            break;
    }

    if(enabled)
        Serial.println(", Enabled");
    else
        Serial.println(", Not Enabled");
}

void CCUDecodingFunctions::DecodeStatusCategory(byte parameter, byte* payloadData, int payloadDataLength)
{
    if(CCUUtility::byteValueExistsInArray(CCUPacketTypes::StatusParameterValues, sizeof(CCUPacketTypes::StatusParameterValues) / sizeof(CCUPacketTypes::StatusParameterValues[0]), parameter))
    {
        CCUPacketTypes::StatusParameter parameterType = static_cast<CCUPacketTypes::StatusParameter>(parameter);

        switch (parameterType)
        {
            case CCUPacketTypes::StatusParameter::Battery:
                // Not catered for
                // Attempting to cater for it
                DecodeBattery(payloadData, payloadDataLength);
                break;
            case CCUPacketTypes::StatusParameter::CameraSpec:
                // Not catered for
                DecodeCameraSpec(payloadData, payloadDataLength);
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
                Serial.print("DecodeStatusCategory, ParameterType DisplayTimecode not catered for: "); Serial.println(static_cast<byte>(parameterType));
                break;
            case CCUPacketTypes::StatusParameter::MediaStatus:
                DecodeMediaStatus(payloadData, payloadDataLength);
                break;
            case CCUPacketTypes::StatusParameter::RemainingRecordTime:
                DecodeRemainingRecordTime(payloadData, payloadDataLength);
                break;
            case CCUPacketTypes::StatusParameter::SwitcherStatus:
                // Not catered for
                // Serial.print("DecodeStatusCategory, ParameterType SwitcherStatus not catered for: "); Serial.println(static_cast<byte>(parameterType));
                break;
        default:
            break;
        }
    }
    else
        throw "Invalid value for Status Parameter.";
}

void CCUDecodingFunctions::DecodeBattery(byte* inData, int inDataLength)
{
    std::vector<short> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, inDataLength, 3);

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
}

void CCUDecodingFunctions::DecodeCameraSpec(byte* inData, int inDataLength)
{
    std::vector<byte> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<byte>(inData, inDataLength, 4);

    // Serial.print("DecodeCameraSpec #1: ");Serial.println(data[0]);
    // Serial.print("DecodeCameraSpec #2: ");Serial.println(data[1]); // This is the camera model, 14 = Pocket 6K.
    // Serial.print("DecodeCameraSpec #3: ");Serial.println(data[2]);
    // Serial.print("DecodeCameraSpec #4: ");Serial.println(data[3]);

    if(data[1] < CameraModels::NumberOfCameraModels)
    {
        CameraModel cameraModel = CameraModels::fromValue(data[1]);

        std::string cameraName = CameraModels::modelToName.at(cameraModel);

        Serial.print("Camera Name: "); Serial.println(cameraName.c_str());

        if(CameraModels::isPocket(cameraModel))
            Serial.println("Classified as a Pocket Camera");
    }
    else
    {
        Serial.print("Unknown Camera with a value of: "); Serial.println(data[1]);
    }
}

void CCUDecodingFunctions::DecodeMediaStatus(byte* inData, int inDataLength)
{
    std::vector<sbyte> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<sbyte>(inData, inDataLength, inDataLength); // We convert all the bytes to sbyte
    int slotCount = data.size();

    for(int index = 0; index < slotCount; index++)
    {
        Serial.print("Slot #"); Serial.print(index);

        CCUPacketTypes::MediaStatus slotMediaStatus = static_cast<CCUPacketTypes::MediaStatus>(data[index]);

        switch(slotMediaStatus)
        {
            case CCUPacketTypes::MediaStatus::None:
                Serial.println(": None");
                break;
            case CCUPacketTypes::MediaStatus::Ready:
                Serial.println(": Ready");
                break;
            case CCUPacketTypes::MediaStatus::MountError:
                Serial.println(": Mount Error");
                break;
            case CCUPacketTypes::MediaStatus::RecordError:
                Serial.println(": Record Error");
                break;
        }
    }
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
String CCUDecodingFunctions::makeTimeLabel(SecondsWithOverflow time) {
    if (time.seconds == 0) { return "Transport Full"; }

    String label = "";
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

void CCUDecodingFunctions::DecodeRemainingRecordTime(byte* inData, int inDataLength)
{
    std::vector<ccu_fixed_t> payload = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<ccu_fixed_t>(inData, inDataLength, inDataLength / 2);

    int slotCount = payload.size();
    std::vector<String> labels(slotCount, "");
    std::vector<int16_t> minutes(slotCount, 0);
    for (int slotIndex = 0; slotIndex < slotCount; ++slotIndex) {
        SecondsWithOverflow remainingTime = simplifyTime(payload[slotIndex]);
        minutes[slotIndex] = remainingTime.seconds / 60;
        labels[slotIndex] = makeTimeLabel(remainingTime);

        Serial.print("Decoded Remaining Record Time Slot #"); Serial.print(slotIndex); Serial.print(" is "); Serial.print(minutes[slotIndex]); Serial.print(" minutes, formatted as "); Serial.println(labels[slotIndex]);
    }
}


void CCUDecodingFunctions::DecodeMediaCategory(byte parameter, byte* payloadData, int payloadDataLength)
{
    if(CCUUtility::byteValueExistsInArray(CCUPacketTypes::MediaParameterValues, sizeof(CCUPacketTypes::MediaParameterValues) / sizeof(CCUPacketTypes::MediaParameterValues[0]), parameter))
    {
        CCUPacketTypes::MediaParameter parameterType = static_cast<CCUPacketTypes::MediaParameter>(parameter);

        switch (parameterType)
        {
            case CCUPacketTypes::MediaParameter::Codec:
                DecodeCodec(payloadData, payloadDataLength);
                break;
            case CCUPacketTypes::MediaParameter::TransportMode:
                DecodeTransportMode(payloadData, payloadDataLength);
                break;
        default:
            break;
        }
    }
    else
        throw "Invalid value for Media Category Parameter.";
}

void CCUDecodingFunctions::DecodeCodec(byte* inData, int inDataLength)
{
    std::vector<byte> data = ConvertPayloadDataWithExpectedCount<byte>(inData, inDataLength, 2);

    CodecInfo codecInfo;
    codecInfo.basicCodec = static_cast<CCUPacketTypes::BasicCodec>(data[0]);
    codecInfo.codecVariant = data[1];

    Serial.print("Decoded Codec Info Basic Codec is ");
    switch(codecInfo.basicCodec)
    {
        case CCUPacketTypes::BasicCodec::RAW:
            Serial.println("RAW");
            break;
        case CCUPacketTypes::BasicCodec::DNxHD:
            Serial.print("DNxHD, ");

            switch(codecInfo.codecVariant)
            {
                case 0:
                    Serial.println("Lossless Raw");
                    break;
                case 1:
                    Serial.println("Raw 3:1");
                    break;
                case 2:
                    Serial.println("Raw 4:1");
                    break;
            }

            break;
        case CCUPacketTypes::BasicCodec::ProRes:
            Serial.print("ProRes ");

            switch(codecInfo.codecVariant)
            {
                case 0:
                    Serial.println("HQ");
                    break;
                case 1:
                    Serial.println("422");
                    break;
                case 2:
                    Serial.println("LT");
                    break;
                case 3:
                    Serial.println("Proxy");
                    break;
                case 4:
                    Serial.println("444");
                    break;
                case 5:
                    Serial.println("444XQ");
                    break;
            }

            break;
        case CCUPacketTypes::BasicCodec::BRAW:
            Serial.print("BRAW ");

            switch(codecInfo.codecVariant)
            {
                case 0:
                    Serial.println("Q0");
                    break;
                case 1:
                    Serial.println("Q5");
                    break;
                case 2:
                    Serial.println("3:1");
                    break;
                case 3:
                    Serial.println("5:1");
                    break;
                case 4:
                    Serial.println("8:1");
                    break;
                case 5:
                    Serial.println("12:1");
                    break;
                case 6:
                    Serial.println("18:1"); // Guessing as the Ursa 12K has 18:1.
                    break;
                case 7:
                    Serial.println("Q1");
                    break;
                case 8:
                    Serial.println("Q3");
                    break;
            }

            break;
    }
}

void CCUDecodingFunctions::DecodeTransportMode(byte* inData, int inDataLength)
{
    std::vector<sbyte> data = ConvertPayloadDataWithExpectedCount<sbyte>(inData, inDataLength, inDataLength);

    TransportInfo transportInfo;

    transportInfo.mode = static_cast<CCUPacketTypes::MediaTransportMode>(data[0]);
    transportInfo.speed = static_cast<sbyte>(data[1]);

    byte flags = static_cast<byte>(data[2]);

    transportInfo.loop = static_cast<bool>((static_cast<int>(flags) & static_cast<int>(CCUPacketTypes::MediaTransportFlag::Loop)) > 0);
    transportInfo.playAll = static_cast<bool>((static_cast<int>(flags) & static_cast<int>(CCUPacketTypes::MediaTransportFlag::PlayAll)) > 0);
    transportInfo.timelapseRecording = static_cast<bool>((static_cast<int>(flags) & static_cast<int>(CCUPacketTypes::MediaTransportFlag::TimelapseRecording)) > 0);

    Serial.print("DecodeTransportMode, Loop is "); Serial.print(transportInfo.loop); Serial.print(", playAll is "); Serial.print(transportInfo.playAll); Serial.print("TimelapseRecording is "); Serial.println(transportInfo.timelapseRecording);

    // The remaining data is for storage slots
    int slotCount = data.size() - 3;
    std::vector<TransportInfo::Slot> slots = std::vector<TransportInfo::Slot>(slotCount, TransportInfo::Slot());
    
    for (int i = 0; i < slots.size(); i++)
    {
        slots[i].active = flags & CCUPacketTypes::slotActiveMasks[i];
        slots[i].medium = static_cast<CCUPacketTypes::ActiveStorageMedium>(data[i + 3]);
        Serial.print("DecodeTransportMode Slot #"); Serial.print(i); Serial.print(", Active is "); Serial.print(slots[i].active); Serial.print(", Storage Medium: "); Serial.println(data[i + 3]);
    }
}

void CCUDecodingFunctions::DecodeMetadataCategory(byte parameter, byte* payloadData, int payloadLength)
{
    if(CCUUtility::byteValueExistsInArray(CCUPacketTypes::MetadataParameterValues, sizeof(CCUPacketTypes::MetadataParameterValues) / sizeof(CCUPacketTypes::MetadataParameterValues[0]), parameter))
    {
        CCUPacketTypes::MetadataParameter parameterType = static_cast<CCUPacketTypes::MetadataParameter>(parameter);

        switch (parameterType)
        {
            case CCUPacketTypes::MetadataParameter::Reel:
                // CONTINUE HERE...
                break;
            case CCUPacketTypes::MetadataParameter::SceneTags:
                break;
            case CCUPacketTypes::MetadataParameter::Scene:
                break;
            case CCUPacketTypes::MetadataParameter::Take:
                break;
            case CCUPacketTypes::MetadataParameter::GoodTake:
                break;
            case CCUPacketTypes::MetadataParameter::SlateForType:
                break;
            case CCUPacketTypes::MetadataParameter::SlateForName:
                break;
            case CCUPacketTypes::MetadataParameter::LensFocalLength:
                DecodeLensFocalLength(payloadData, payloadLength);
                break;
            case CCUPacketTypes::MetadataParameter::LensDistance:
                DecodeLensDistance(payloadData, payloadLength);
                break;
            case CCUPacketTypes::MetadataParameter::LensType:
                DecodeLensType(payloadData, payloadLength);
                break;
            case CCUPacketTypes::MetadataParameter::LensIris:
                break;
        default:
            break;
        }
    }
    else
        throw "Invalid value for Metadata Parameter.";
}

void CCUDecodingFunctions::DecodeLensFocalLength(byte* inData, int inDataLength)
{
    std::string lensFocalLength = CCUDecodingFunctions::ConvertPayloadDataToString(inData, inDataLength);
    Serial.print("DecodeLensFocalLength Lens Focal Length: "); Serial.println(lensFocalLength.c_str());
    // DecodeLensFocalLength Lens Focal Length: 65mm

}
void CCUDecodingFunctions::DecodeLensDistance(byte* inData, int inDataLength)
{
    std::string lensDistance = ConvertPayloadDataToString(inData, inDataLength);
    Serial.print("DecodeLensFocalDistance Lens Distance: "); Serial.println(lensDistance.c_str());
    // DecodeLensFocalDistance Lens Distance: Inf
    // DecodeLensFocalDistance Lens Distance: 26100mm to 41310mm
}
void CCUDecodingFunctions::DecodeLensType(byte* inData, int inDataLength)
{
    std::string lensType = ConvertPayloadDataToString(inData, inDataLength);
    Serial.print("DecodeLensType Lens Type: "); Serial.println(lensType.c_str());
    // DecodeLensType Lens Type: Canon EF-S 55-250mm f/4-5.6 IS
    // "DecodeLensType Lens Type:" <-- this shows when there's no info between camera and lens.
}