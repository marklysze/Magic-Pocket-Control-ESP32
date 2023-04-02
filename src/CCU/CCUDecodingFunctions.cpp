#include "CCUDecodingFunctions.h"
#include <stdexcept>

void CCUDecodingFunctions::DecodeCCUPacket(std::vector<byte> byteArray) // const byte* byteArray, int length)
{
    bool isValid = CCUValidationFunctions::ValidateCCUPacket(byteArray);

    if (isValid)
    {
        /*
        byte commandLength = byteArray[PacketFormatIndex::CommandLength];
        CCUPacketTypes::Category category = static_cast<CCUPacketTypes::Category>(byteArray[PacketFormatIndex::Category]);
        byte parameter = byteArray[PacketFormatIndex::Parameter];

        byte dataLength = static_cast<byte>(commandLength - CCUPacketTypes::kCCUCommandHeaderSize);
        byte payloadOffset = CCUPacketTypes::kCUUPayloadOffset;
        byte* payloadData = new byte[dataLength];
        memcpy(payloadData, byteArray + payloadOffset, dataLength);
        */

        byte commandLength = byteArray[PacketFormatIndex::CommandLength];
        CCUPacketTypes::Category category = static_cast<CCUPacketTypes::Category>(byteArray[PacketFormatIndex::Category]);
        byte parameter = byteArray[PacketFormatIndex::Parameter];

        byte dataLength = static_cast<byte>(commandLength - CCUPacketTypes::kCCUCommandHeaderSize);
        byte payloadOffset = CCUPacketTypes::kCUUPayloadOffset;
        std::vector<byte> payloadData(byteArray.begin() + payloadOffset, byteArray.begin() + payloadOffset + dataLength);

        try
        {
            DecodePayloadData(category, parameter, payloadData); //, dataLength);
        }
        catch (const std::exception& ex)
        {
            Serial.print("Exception in DecodeCCUPacket: ");Serial.println(ex.what());
            // delete[] payloadData;
            throw ex;
        }

        // delete[] payloadData;
    }
}

// implementation of DecodePayloadData function
void CCUDecodingFunctions::DecodePayloadData(CCUPacketTypes::Category category, byte parameter, std::vector<byte> payloadData) //, byte* payloadData, int payloadDataLength)
{
    if(category != CCUPacketTypes::Category::Status)
    {
        // Serial.print("DecodePayloadData, Category: "); Serial.println(static_cast<byte>(category));
    }

    switch (category) {
        case CCUPacketTypes::Category::Lens:
            DecodeLensCategory(parameter, payloadData.data(), payloadData.size()); // payloadDataLength);
            break;
        case CCUPacketTypes::Category::Video:
            DecodeVideoCategory(parameter, payloadData.data(), payloadData.size());
            break;
        case CCUPacketTypes::Category::Status:
            DecodeStatusCategory(parameter, payloadData.data(), payloadData.size());
            break;
        case CCUPacketTypes::Category::Media:
            DecodeMediaCategory(parameter, payloadData.data(), payloadData.size());
            break;
        case CCUPacketTypes::Category::Metadata:
            DecodeMetadataCategory(parameter, payloadData.data(), payloadData.size());
            break;
        case CCUPacketTypes::Category::Display:
            DecodeDisplayCategory(parameter, payloadData.data(), payloadData.size());
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
        // Serial.print("Decoded fStopIndex Aperture is "); Serial.println(LensConfig::GetFStopString(ConvertCCUApertureToFstop(apertureNumber), apertureUnits).c_str());

        BMDControlSystem::getInstance()->getCamera()->onHasLens(true); // A lens is attached

        BMDControlSystem::getInstance()->getCamera()->onApertureUnitsReceived(apertureUnits);
        BMDControlSystem::getInstance()->getCamera()->onApertureFStopStringReceived(LensConfig::GetFStopString(ConvertCCUApertureToFstop(apertureNumber), apertureUnits));
    }
    else
    {
        // Serial.println("Decoded fStopIndex: No Lens Information");

        BMDControlSystem::getInstance()->getCamera()->onHasLens(false); // No Lens
    }
}

void CCUDecodingFunctions::DecodeApertureNormalised(byte* inData, int inDataLength)
{
    std::vector<short> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, inDataLength, 1); // Receiving a fixed16 number, as a short it will be 0-2,048 representing 0.0-1.0
    short apertureNormalisedNumber = data[0];

    if(apertureNormalisedNumber != CCUPacketTypes::kLensAperture_NoLens)
    {
        float apertureValue = static_cast<float>(apertureNormalisedNumber) / 2048.0f; // Convert to float and perform division
        // std::string apertureString = "Decode Aperture Normalised: " + std::to_string(static_cast<int>(apertureValue * 100.0f)) + "%"; // Multiply by 100 to get percentage and convert to string

        // Serial.println(apertureString.c_str());

        BMDControlSystem::getInstance()->getCamera()->onHasLens(true); // A lens is attached

        BMDControlSystem::getInstance()->getCamera()->onApertureNormalisedReceived(static_cast<int>(apertureValue * 100.0f));
    }
    else
    {
        // Serial.println("Decoded Aperture Normalized: No Lens Information");

        BMDControlSystem::getInstance()->getCamera()->onHasLens(false); // No Lens
    }
}

void CCUDecodingFunctions::DecodeAutoFocus(byte* inData, int inDataLength)
{
    // Serial.print("DecodeAutoFocus: "); Serial.println(inDataLength);
    bool instantaneousAutoFocusPressed = true;

    BMDControlSystem::getInstance()->getCamera()->onAutoFocusPressed();
}

void CCUDecodingFunctions::DecodeZoom(byte* inData, int inDataLength)
{
    // ccu_fixed_t
    std::vector<ccu_fixed_t> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, inDataLength, 1);
    ccu_fixed_t focalLengthMM = data[0];

    if(focalLengthMM != 0)
    {
        // Serial.print("Decode Zoom Focal Length: "); Serial.println(focalLengthMM);

        BMDControlSystem::getInstance()->getCamera()->onHasLens(true); // A lens is attached

        BMDControlSystem::getInstance()->getCamera()->onFocalLengthMMReceived(focalLengthMM);
    }
    else
    {
        // Serial.println("Decode Zoom: No Lens Information");
    }
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
                DecodeSensorGainISO(payloadData, payloadLength);
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

void CCUDecodingFunctions::DecodeSensorGainISO(byte* inData, int inDataLength)
{
    std::vector<short> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, inDataLength, 1);
    short sensorGainValue = data[0];
    int sensorGain = sensorGainValue * static_cast<int>(VideoConfig::kReceivedSensorGainBase);
    
    // Serial.print("Decoded Sensor Gain is "); Serial.println(sensorGain);

    BMDControlSystem::getInstance()->getCamera()->onSensorGainISOReceived(sensorGain);
}

void CCUDecodingFunctions::DecodeManualWB(byte* inData, int inDataLength)
{
    std::vector<short> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<short>(inData, inDataLength, 2);
    short whiteBalance = data[0];
    short tint = data[1];

    // Serial.print("Decoded White Balance is "); Serial.print(whiteBalance); Serial.print(" and Tint is "); Serial.println(tint);

    BMDControlSystem::getInstance()->getCamera()->onWhiteBalanceReceived(whiteBalance);
    BMDControlSystem::getInstance()->getCamera()->onTintReceived(tint);
}

void CCUDecodingFunctions::DecodeExposure(byte* inData, int inDataLength)
{
    std::vector<int32_t> data = ConvertPayloadDataWithExpectedCount<int32_t>(inData, inDataLength, 1);
    int32_t shutterSpeedMS = data[0]; // Time in microseconds: us

    // Serial.print("Decoded Exposure (Shutter Speed): "); Serial.println(shutterSpeed);

    BMDControlSystem::getInstance()->getCamera()->onShutterSpeedMSReceived(shutterSpeedMS);
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

    /*
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
    */

   BMDControlSystem::getInstance()->getCamera()->onRecordingFormatReceived(recordingFormatData);
}

void CCUDecodingFunctions::DecodeAutoExposureMode(byte* inData, int inDataLength)
{
    std::vector<sbyte> data = ConvertPayloadDataWithExpectedCount<sbyte>(inData, inDataLength, 1);
    CCUPacketTypes::AutoExposureMode autoExposureMode = static_cast<CCUPacketTypes::AutoExposureMode>(data[0]);

    /*
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
    */
   
   BMDControlSystem::getInstance()->getCamera()->onAutoExposureModeReceived(autoExposureMode);
}

void CCUDecodingFunctions::DecodeShutterAngle(byte* inData, int inDataLength)
{
    std::vector<int32_t> data = ConvertPayloadDataWithExpectedCount<int32_t>(inData, inDataLength, 1);
    int32_t shutterAngleX100 = data[0];

    // Serial.print("Decoded Exposure (Shutter Angle x 100): "); Serial.println(shutterAngleX100);

    BMDControlSystem::getInstance()->getCamera()->onShutterAngleReceived(shutterAngleX100);
}

void CCUDecodingFunctions::DecodeShutterSpeed(byte* inData, int inDataLength)
{
    std::vector<int32_t> data = ConvertPayloadDataWithExpectedCount<int32_t>(inData, inDataLength, 1);
    int32_t shutterSpeed = data[0]; // Result is the denominator in 1/X, e.g. shutterSpeed = 24 is a shutter speed of 1/24

    // Serial.print("Decoded Exposure (Shutter Speed): "); Serial.println(shutterSpeed);

    BMDControlSystem::getInstance()->getCamera()->onShutterSpeedReceived(shutterSpeed);
}

void CCUDecodingFunctions::DecodeGain(byte* inData, int inDataLength)
{
    std::vector<byte> data = ConvertPayloadDataWithExpectedCount<byte>(inData, inDataLength, 1);
    byte gain = data[0];

    // Serial.print("Decoded Gain): "); Serial.println(gain);

    BMDControlSystem::getInstance()->getCamera()->onSensorGainDBReceived(gain);
}

void CCUDecodingFunctions::DecodeISO(byte* inData, int inDataLength)
{
    std::vector<int32_t> data = ConvertPayloadDataWithExpectedCount<int32_t>(inData, inDataLength, 1);
    int32_t iso = data[0];

    // Serial.print("Decoded ISO: "); Serial.println(iso);

    BMDControlSystem::getInstance()->getCamera()->onSensorGainISOValueReceived(iso);
}

void CCUDecodingFunctions::DecodeDisplayLUT(byte* inData, int inDataLength)
{
    std::vector<byte> data = ConvertPayloadDataWithExpectedCount<byte>(inData, inDataLength, 2);
    CCUPacketTypes::SelectedLUT selectedLut = static_cast<CCUPacketTypes::SelectedLUT>(data[0]);
    bool enabled = data[1] == 1;

    /*
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
    */

   BMDControlSystem::getInstance()->getCamera()->onSelectedLUTReceived(selectedLut);
   BMDControlSystem::getInstance()->getCamera()->onSelectedLUTEnabledReceived(enabled);
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

   BMDControlSystem::getInstance()->getCamera()->onBatteryReceived(batteryStatusData);
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

        // Serial.print("Camera Name: "); Serial.println(cameraName.c_str());

        // if(CameraModels::isPocket(cameraModel)) Serial.println("Classified as a Pocket Camera");
            
        // Update Camera object
        BMDControlSystem::getInstance()->getCamera()->onModelNameReceived(cameraName);
        BMDControlSystem::getInstance()->getCamera()->onIsPocketReceived(CameraModels::isPocket(cameraModel));

        std::string setName = BMDControlSystem::getInstance()->getCamera()->getModelName();
        // Serial.print("Retrieved Camera Name: "); Serial.println(setName.c_str());
    }
    else
    {
        // Serial.print("Unknown Camera with a value of: "); Serial.println(data[1]);

        BMDControlSystem::getInstance()->getCamera()->onModelNameReceived("UNKNOWN CAMERA MODEL");
    }
}

void CCUDecodingFunctions::DecodeMediaStatus(byte* inData, int inDataLength)
{
    std::vector<sbyte> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<sbyte>(inData, inDataLength, inDataLength); // We convert all the bytes to sbyte
    int slotCount = data.size();

    std::vector<CCUPacketTypes::MediaStatus> mediaStatuses;

    for(int index = 0; index < slotCount; index++)
    {
        // Serial.print("Slot #"); Serial.print(index);

        CCUPacketTypes::MediaStatus slotMediaStatus = static_cast<CCUPacketTypes::MediaStatus>(data[index]);

        mediaStatuses.push_back(slotMediaStatus);

        /*
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
        */
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

void CCUDecodingFunctions::DecodeRemainingRecordTime(byte* inData, int inDataLength)
{
    std::vector<ccu_fixed_t> payload = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<ccu_fixed_t>(inData, inDataLength, inDataLength / 2);

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

    /*

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

    */

    BMDControlSystem::getInstance()->getCamera()->onCodecReceived(codecInfo);
}

void CCUDecodingFunctions::DecodeTransportMode(byte* inData, int inDataLength)
{
    std::vector<sbyte> data = ConvertPayloadDataWithExpectedCount<sbyte>(inData, inDataLength, inDataLength);

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

    // Serial.print("DecodeTransportMode, Loop is "); Serial.print(transportInfo.loop); Serial.print(", playAll is "); Serial.print(transportInfo.playAll); Serial.print(", TimelapseRecording is "); Serial.println(transportInfo.timelapseRecording);

    // The remaining data is for storage slots
    int slotCount = data.size() - 3;
    transportInfo.slots = std::vector<TransportInfo::TransportInfoSlot>(slotCount, TransportInfo::TransportInfoSlot());
    
    for (int i = 0; i < transportInfo.slots.size(); i++)
    {
        transportInfo.slots[i].active = flags & CCUPacketTypes::slotActiveMasks[i];
        transportInfo.slots[i].medium = static_cast<CCUPacketTypes::ActiveStorageMedium>(data[i + 3]);

        // Serial.print("DecodeTransportMode Slot #"); Serial.print(i); Serial.print(", Active is "); Serial.print(transportInfo.slots[i].active); Serial.print(", Storage Medium: "); Serial.println(data[i + 3]);
    }

    BMDControlSystem::getInstance()->getCamera()->onTransportModeReceived(transportInfo);
}

void CCUDecodingFunctions::DecodeMetadataCategory(byte parameter, byte* payloadData, int payloadLength)
{
    if(CCUUtility::byteValueExistsInArray(CCUPacketTypes::MetadataParameterValues, sizeof(CCUPacketTypes::MetadataParameterValues) / sizeof(CCUPacketTypes::MetadataParameterValues[0]), parameter))
    {
        CCUPacketTypes::MetadataParameter parameterType = static_cast<CCUPacketTypes::MetadataParameter>(parameter);

        switch (parameterType)
        {
            case CCUPacketTypes::MetadataParameter::Reel:
                DecodeReel(payloadData, payloadLength);
                break;
            case CCUPacketTypes::MetadataParameter::SceneTags:
                DecodeSceneTags(payloadData, payloadLength);
                break;
            case CCUPacketTypes::MetadataParameter::Scene:
                DecodeScene(payloadData, payloadLength);
                break;
            case CCUPacketTypes::MetadataParameter::Take:
                DecodeTake(payloadData, payloadLength);
                break;
            case CCUPacketTypes::MetadataParameter::GoodTake:
                DecodeGoodTake(payloadData, payloadLength);
                break;
            case CCUPacketTypes::MetadataParameter::CameraId:
                DecodeCameraId(payloadData, payloadLength);
                break;
            case CCUPacketTypes::MetadataParameter::CameraOperator:
                DecodeCameraOperator(payloadData, payloadLength);
                break;
            case CCUPacketTypes::MetadataParameter::Director:
                DecodeDirector(payloadData, payloadLength);
                break;
            case CCUPacketTypes::MetadataParameter::ProjectName:
                DecodeProjectName(payloadData, payloadLength);
                break;
            case CCUPacketTypes::MetadataParameter::SlateForType:
                DecodeSlateForType(payloadData, payloadLength);
                break;
            case CCUPacketTypes::MetadataParameter::SlateForName:
                DecodeSlateForName(payloadData, payloadLength);
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
                DecodeLensIris(payloadData, payloadLength);
                break;
        default:
            Serial.print("DecodeMetadataCategory, ParameterType not known: "); Serial.println(static_cast<byte>(parameterType));
            break;
        }
    }
    else
        throw "Invalid value for Metadata Parameter.";
}

void CCUDecodingFunctions::DecodeReel(byte* inData, int inDataLength)
{
    std::vector<short> data = ConvertPayloadDataWithExpectedCount<short>(inData, inDataLength, 1);
    short reelNumber = data[0];

    // Serial.print("DecodeReel, Reel Number: "); Serial.println(reelNumber);

    BMDControlSystem::getInstance()->getCamera()->onReelNumberReceived(reelNumber);
}

void CCUDecodingFunctions::DecodeScene(byte* inData, int inDataLength)
{
    std::string sceneString = CCUDecodingFunctions::ConvertPayloadDataToString(inData, inDataLength);

    // Serial.print("DecodeScene Scene: "); Serial.println(sceneString.c_str());

    BMDControlSystem::getInstance()->getCamera()->onSceneNameReceived(sceneString);
}

void CCUDecodingFunctions::DecodeSceneTags(byte* inData, int inDataLength)
{
    std::vector<sbyte> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<sbyte>(inData, inDataLength, 3);
    CCUPacketTypes::MetadataSceneTag sceneTag = static_cast<CCUPacketTypes::MetadataSceneTag>(data[0]);
    CCUPacketTypes::MetadataLocationTypeTag locationType = static_cast<CCUPacketTypes::MetadataLocationTypeTag>(data[1]);
    CCUPacketTypes::MetadataDayNightTag dayOrNight = static_cast<CCUPacketTypes::MetadataDayNightTag>(data[2]);

    // Serial.print("DecodeSceneTags Scene Tag is "); Serial.print((sbyte)sceneTag); Serial.print(", Location Type is "); Serial.print((byte)locationType); Serial.print(", DayOrNight is ");Serial.println((byte)dayOrNight);

    BMDControlSystem::getInstance()->getCamera()->onSceneTagReceived(sceneTag);
    BMDControlSystem::getInstance()->getCamera()->onLocationTypeReceived(locationType);
    BMDControlSystem::getInstance()->getCamera()->onDayOrNightReceived(dayOrNight);
}

void CCUDecodingFunctions::DecodeTake(byte* inData, int inDataLength)
{
    std::vector<sbyte> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<sbyte>(inData, inDataLength, 2);
    sbyte takeNumber = data[0];
    CCUPacketTypes::MetadataTakeTag takeTag = static_cast<CCUPacketTypes::MetadataTakeTag>(data[1]);

    // Serial.print("DecodeTake Take Number is "); Serial.print((sbyte)takeNumber); Serial.print(", Take Tag is "); Serial.println((sbyte)takeTag);

    BMDControlSystem::getInstance()->getCamera()->onTakeTagReceived(takeTag);
    BMDControlSystem::getInstance()->getCamera()->onTakeNumberReceived(takeNumber);
}

void CCUDecodingFunctions::DecodeGoodTake(byte* inData, int inDataLength)
{
    std::vector<sbyte> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<sbyte>(inData, inDataLength, 1);
    sbyte goodTake = data[0];

    // Serial.print("DecodeGoodTake Good Take is "); Serial.println((sbyte)goodTake);

    BMDControlSystem::getInstance()->getCamera()->onGoodTakeReceived(goodTake);
}

void CCUDecodingFunctions::DecodeCameraId(byte* inData, int inDataLength)
{
    std::string cameraId = CCUDecodingFunctions::ConvertPayloadDataToString(inData, inDataLength);

    // Serial.print("DecodeCameraId CameraId is "); Serial.println(cameraId.c_str());

    BMDControlSystem::getInstance()->getCamera()->onCameraIdReceived(cameraId);
}

void CCUDecodingFunctions::DecodeCameraOperator(byte* inData, int inDataLength)
{
    std::string cameraOperator = CCUDecodingFunctions::ConvertPayloadDataToString(inData, inDataLength);

    // Serial.print("DecodeCameraOperator Camera Operator is "); Serial.println(cameraOperator.c_str());

    BMDControlSystem::getInstance()->getCamera()->onCameraOperatorReceived(cameraOperator);
}

void CCUDecodingFunctions::DecodeDirector(byte* inData, int inDataLength)
{
    std::string director = CCUDecodingFunctions::ConvertPayloadDataToString(inData, inDataLength);

    // Serial.print("DecodeDirector Director is "); Serial.println(director.c_str());

    BMDControlSystem::getInstance()->getCamera()->onDirectorReceived(director);
}

void CCUDecodingFunctions::DecodeProjectName(byte* inData, int inDataLength)
{
    std::string projectName = CCUDecodingFunctions::ConvertPayloadDataToString(inData, inDataLength);

    // Serial.print("DecodeProjectName Project Name is "); Serial.println(projectName.c_str());

    BMDControlSystem::getInstance()->getCamera()->onProjectNameReceived(projectName);
}


void CCUDecodingFunctions::DecodeSlateForType(byte* inData, int inDataLength)
{
    std::vector<sbyte> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<sbyte>(inData, inDataLength, 1);
    CCUPacketTypes::MetadataSlateForType slateForType = static_cast<CCUPacketTypes::MetadataSlateForType>(data[0]);

    // Serial.print("DecodeSlateForType Slate For Type is "); Serial.println((sbyte)slateForType);

    BMDControlSystem::getInstance()->getCamera()->onSlateTypeReceived(slateForType);
}

void CCUDecodingFunctions::DecodeSlateForName(byte* inData, int inDataLength)
{
    std::string name = CCUDecodingFunctions::ConvertPayloadDataToString(inData, inDataLength);

    // NOTE: Camera software versions between 7.5 and 7.7.x send the full path of
    // the clip file name. When this is detected, strip the system folders, first
    // folder (volume), and final file extension.
    std::regex re("^/mnt/s[0-9]+/");
    std::smatch match;
    if (std::regex_search(name, match, re)) {
        name = name.substr(match.position() + match.length());
    }

    // Serial.print("DecodeSlateForName Name is "); Serial.println(name.c_str());

    BMDControlSystem::getInstance()->getCamera()->onSlateNameReceived(name);
}

void CCUDecodingFunctions::DecodeLensFocalLength(byte* inData, int inDataLength)
{
    std::string lensFocalLength = CCUDecodingFunctions::ConvertPayloadDataToString(inData, inDataLength);

    // Serial.print("DecodeLensFocalLength Lens Focal Length: "); Serial.println(lensFocalLength.c_str());
    // DecodeLensFocalLength Lens Focal Length: 65mm

    BMDControlSystem::getInstance()->getCamera()->onLensFocalLengthReceived(lensFocalLength);
}

void CCUDecodingFunctions::DecodeLensDistance(byte* inData, int inDataLength)
{
    std::string lensDistance = ConvertPayloadDataToString(inData, inDataLength);

    // Serial.print("DecodeLensFocalDistance Lens Distance: "); Serial.println(lensDistance.c_str());
    // DecodeLensFocalDistance Lens Distance: Inf
    // DecodeLensFocalDistance Lens Distance: 26100mm to 41310mm

    BMDControlSystem::getInstance()->getCamera()->onLensDistanceReceived(lensDistance);
}

void CCUDecodingFunctions::DecodeLensType(byte* inData, int inDataLength)
{
    std::string lensType = ConvertPayloadDataToString(inData, inDataLength);

    // Serial.print("DecodeLensType Lens Type: "); Serial.println(lensType.c_str());
    // DecodeLensType Lens Type: Canon EF-S 55-250mm f/4-5.6 IS
    // "DecodeLensType Lens Type:" <-- this shows when there's no info between camera and lens.

    BMDControlSystem::getInstance()->getCamera()->onLensTypeReceived(lensType);
}

void CCUDecodingFunctions::DecodeLensIris(byte* inData, int inDataLength)
{
    std::string lensIris = ConvertPayloadDataToString(inData, inDataLength);

    // Serial.print("DecodeLensType Lens Iris: "); Serial.println(lensIris.c_str());
    // DecodeLensType Lens Type: Canon EF-S 55-250mm f/4-5.6 IS
    // "DecodeLensType Lens Type:" <-- this shows when there's no info between camera and lens.

    BMDControlSystem::getInstance()->getCamera()->onLensIrisReceived(lensIris);
}

// DISPLAY CATEGORY

void CCUDecodingFunctions::DecodeDisplayCategory(byte parameter, byte* payloadData, int payloadLength)
{
    if(CCUUtility::byteValueExistsInArray(CCUPacketTypes::DisplayParameterValues, sizeof(CCUPacketTypes::DisplayParameterValues) / sizeof(CCUPacketTypes::MetadataParameterValues[0]), parameter))
    {
        CCUPacketTypes::DisplayParameter parameterType = static_cast<CCUPacketTypes::DisplayParameter>(parameter);

        switch (parameterType)
        {
            case CCUPacketTypes::DisplayParameter::TimecodeSource:
                DecodeTimecodeSource(payloadData, payloadLength);
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
            Serial.print("DecodeDisplayCategory, ParameterType not known: "); Serial.println(static_cast<byte>(parameterType));
            break;
        }
    }
    else
        throw "Invalid value for Display Parameter.";
}

void CCUDecodingFunctions::DecodeTimecodeSource(byte* inData, int inDataLength)
{
    std::vector<sbyte> data = CCUDecodingFunctions::ConvertPayloadDataWithExpectedCount<sbyte>(inData, inDataLength, 1);
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