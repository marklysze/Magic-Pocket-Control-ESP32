#include "CCUEncodingFunctions.h"

int16_t CCUEncodingFunctions::ConvertFStopToCCUAperture(float inputFStop)
{
    int16_t ccuAperture = static_cast<int16_t>(log2(inputFStop) * 2048 * 2);

    return ccuAperture;
}

CCUPacketTypes::Command CCUEncodingFunctions::CreateVideoWhiteBalanceCommand(short whiteBalance, short tint)
{
    short dataArray[] = { whiteBalance, tint };
    std::vector<byte> payloadData = CCUUtility::ToByteArrayFromArray(dataArray, sizeof(dataArray));

    CCUPacketTypes::Command command(
        CCUPacketTypes::kBroadcastTarget,
        CCUPacketTypes::CommandID::ChangeConfiguration,
        CCUPacketTypes::Category::Video,
        static_cast<byte>(CCUPacketTypes::VideoParameter::ManualWB),
        CCUPacketTypes::OperationType::AssignValue,
        static_cast<byte>(CCUPacketTypes::DataTypes::kInt16),
        payloadData);

    return command;
}

CCUPacketTypes::Command CCUEncodingFunctions::CreateVoidCommand(CCUPacketTypes::Category category, byte parameter) {
    std::vector<byte> data;
    CCUPacketTypes::Command command(
        CCUPacketTypes::kBroadcastTarget,
        CCUPacketTypes::CommandID::ChangeConfiguration,
        category,
        parameter,
        CCUPacketTypes::OperationType::AssignValue,
        static_cast<byte>(CCUPacketTypes::DataTypes::kVoid),
        data);

    return command;
}

CCUPacketTypes::Command CCUEncodingFunctions::CreateVideoSetAutoWBCommand() {
    return CCUEncodingFunctions::CreateVoidCommand(CCUPacketTypes::Category::Video, static_cast<byte>(CCUPacketTypes::VideoParameter::SetAutoWB));
}

CCUPacketTypes::Command CCUEncodingFunctions::CreateRecordingFormatCommand(CCUPacketTypes::RecordingFormatData recordingFormatData) {
    short flags = 0;

    if (recordingFormatData.mRateEnabled) {
        flags |= static_cast<int16_t>(CCUPacketTypes::VideoRecordingFormat::FileMRate);
    }

    // MS this doesn't seem to be part of the command, though it's in the manual
    /*
    if (recordingFormatData.sensorMRateEnabled) {
        flags |= static_cast<short>(CCUPacketTypes::VideoRecordingFormat::SensorMRate);
    }
    */

    if (recordingFormatData.offSpeedEnabled) {
        flags |= static_cast<int16_t>(CCUPacketTypes::VideoRecordingFormat::SensorOffSpeed);
    }

    if (recordingFormatData.interlacedEnabled) {
        flags |= static_cast<int16_t>(CCUPacketTypes::VideoRecordingFormat::Interlaced);
    }

    if (recordingFormatData.windowedModeEnabled) {
        flags |= static_cast<int16_t>(CCUPacketTypes::VideoRecordingFormat::WindowedMode);
    }

    DEBUG_DEBUG("CreateRecordingFormatCommand - flags: %i", flags);

    short data[] = { recordingFormatData.frameRate, recordingFormatData.offSpeedFrameRate, recordingFormatData.width, recordingFormatData.height, flags };

    std::vector<byte> payloadData = CCUUtility::ToByteArrayFromArray(data, sizeof(data));

    CCUPacketTypes::Command command(
        CCUPacketTypes::kBroadcastTarget,
        CCUPacketTypes::CommandID::ChangeConfiguration,
        CCUPacketTypes::Category::Video,
        static_cast<byte>(CCUPacketTypes::VideoParameter::RecordingFormat),
        CCUPacketTypes::OperationType::AssignValue,
        static_cast<byte>(CCUPacketTypes::DataTypes::kInt16),
        payloadData);

    return command;
}

template<typename T>
CCUPacketTypes::DataTypes getCCUDataType(T value)
{
    if (std::is_same<T, int>::value)
    {
        return CCUPacketTypes::DataTypes::kInt32;
    }
    else if (std::is_same<T, short>::value)
    {
        return CCUPacketTypes::DataTypes::kInt16;
    }
    else if (std::is_same<T, long>::value)
    {
        return CCUPacketTypes::DataTypes::kInt64;
    }
    else if (std::is_same<T, byte>::value)
    {
        return CCUPacketTypes::DataTypes::kInt8;
    }

    throw std::runtime_error("Have not handled type in CCUEncodingFunction::getCCUDataType");
}

// Enables the OperationType to be passed in (Actual Value vs Offset Value), defaults to the original Actual Value
CCUPacketTypes::Command CCUEncodingFunctions::CreateFixed16Command(short value, CCUPacketTypes::Category category, byte parameter, CCUPacketTypes::OperationType operationType /*= CCUPacketTypes::OperationType::AssignValue */) {

    std::vector<byte> data = CCUUtility::ToByteArray(value);
    CCUPacketTypes::Command command(
        CCUPacketTypes::kBroadcastTarget,
        CCUPacketTypes::CommandID::ChangeConfiguration,
        category,
        parameter,
        operationType,
        static_cast<byte>(CCUPacketTypes::DataTypes::kFixed16),
        data);

    return command;
}

template <typename T>
CCUPacketTypes::Command CCUEncodingFunctions::CreateCommand(T value, CCUPacketTypes::Category category, byte parameter) {

    std::vector<byte> data = CCUUtility::ToByteArray(value);
    CCUPacketTypes::Command command(
        CCUPacketTypes::kBroadcastTarget,
        CCUPacketTypes::CommandID::ChangeConfiguration,
        category,
        parameter,
        CCUPacketTypes::OperationType::AssignValue,
        static_cast<byte>(getCCUDataType(value)),
        data);

    return command;
}

CCUPacketTypes::Command CCUEncodingFunctions::CreateVideoSensorGainCommand(byte value)
{
    return CreateCommand(value, CCUPacketTypes::Category::Video, static_cast<byte>(CCUPacketTypes::VideoParameter::SensorGain));
}

CCUPacketTypes::Command CCUEncodingFunctions::CreateVideoISOCommand(int value)
{
    return CreateCommand(value, CCUPacketTypes::Category::Video, static_cast<byte>(CCUPacketTypes::VideoParameter::ISO));
}

CCUPacketTypes::Command CCUEncodingFunctions::CreateShutterAngleCommand(int value)
{
    return CreateCommand(value, CCUPacketTypes::Category::Video, static_cast<byte>(CCUPacketTypes::VideoParameter::ShutterAngle));
}


CCUPacketTypes::Command CCUEncodingFunctions::CreateTransportInfoCommand(TransportInfo transportInfo)
{
    std::vector<byte> data = transportInfo.toArray();
    CCUPacketTypes::Command command(
        CCUPacketTypes::kBroadcastTarget,
        CCUPacketTypes::CommandID::ChangeConfiguration,
        CCUPacketTypes::Category::Media,
        static_cast<byte>(CCUPacketTypes::MediaParameter::TransportMode),
        CCUPacketTypes::OperationType::AssignValue,
        static_cast<byte>(CCUPacketTypes::DataTypes::kInt8),
        data);

    return command;
}

CCUPacketTypes::Command CCUEncodingFunctions::CreateCodecCommand(CodecInfo codecInfo)
{
    std::vector<byte> data = { static_cast<byte>(codecInfo.basicCodec), codecInfo.codecVariant };

    // DEBUG_DEBUG("CreateCodecCommand byte 1: %i, byte 2: %i", codecInfo.basicCodec, codecInfo.codecVariant);

    CCUPacketTypes::Command command(
        CCUPacketTypes::kBroadcastTarget,
        CCUPacketTypes::CommandID::ChangeConfiguration,
        CCUPacketTypes::Category::Media,
        static_cast<byte>(CCUPacketTypes::MediaParameter::Codec),
        CCUPacketTypes::OperationType::AssignValue,
        static_cast<byte>(CCUPacketTypes::DataTypes::kInt8),
        data);

    return command;
}

CCUPacketTypes::Command CCUEncodingFunctions::CreateInt16Command(short value, CCUPacketTypes::Category category, byte parameter) {

    short dataArray[] = { 0, value };
    std::vector<byte> payloadData = CCUUtility::ToByteArrayFromArray(dataArray, sizeof(dataArray));

    // std::vector<byte> data = CCUUtility::ToByteArray(value);
    CCUPacketTypes::Command command(
        CCUPacketTypes::kBroadcastTarget,
        CCUPacketTypes::CommandID::ChangeConfiguration,
        category,
        parameter,
        CCUPacketTypes::OperationType::AssignValue,
        static_cast<byte>(CCUPacketTypes::DataTypes::kInt16),
        payloadData);

    return command;
}

/* FUNCTIONS NOT USED AND CONVERTED FROM C# VERSON.

CCUPacketTypes::Command CCUEncodingFunctions::CreateRecordingFormatStatusCommand()
{
    byte* data = CCUUtility::ToByteArray(400);
    CCUPacketTypes::Command command(
        CCUPacketTypes::kBroadcastTarget,
        CCUPacketTypes::CommandID::ChangeConfiguration,
        CCUPacketTypes::Category::Video,
        static_cast<byte>(CCUPacketTypes::VideoParameter::ISO),
        CCUPacketTypes::OperationType::OffsetValue,
        static_cast<byte>(getCCUDataType(static_cast<int>(0))),
        data);

    delete[] data; // CHECK IF THIS SHOULD BE HERE

    return command;
}

public static func CreateVideoExposureCommand(value: Int32) -> (CCUPacketTypes.Command?) {
    return CreateCommand(value, CCUPacketTypes.Category.Video, CCUPacketTypes.VideoParameter.Exposure.rawValue)
}


// Metadata encoding functions
// Important: All metadata CCU commands are undocumented and are subject to change in a future release

public static func CreateMetadataStringCommand(_ string: String, _ category: CCUPacketTypes.Category, _ parameter: CCUPacketTypes.MetadataParameter) -> (CCUPacketTypes.Command?) {
    let maxLength: Int8? = CCUPacketTypes.MetadataParameterToStringLengthMap[parameter]
    if maxLength != nil {
        return CreateStringCommand(string, category, parameter.rawValue, maxLength: maxLength!)
    } else {
        Logger.LogError("No max string length info for metadata parameter \(parameter). Cannot create CCU command.")
    }

    return nil
}

public static func CreateMetadataSceneTagsCommand(_ sceneTag: CCUPacketTypes.MetadataSceneTag, _ locationType: CCUPacketTypes.MetadataLocationTypeTag, _ dayOrNight: CCUPacketTypes.MetadataDayNightTag) -> (CCUPacketTypes.Command?) {
    var data = [UInt8](repeating: 0, count: 3)
    data[0] = UInt8(bitPattern: sceneTag.rawValue)
    data[1] = locationType.rawValue
    data[2] = dayOrNight.rawValue

    let command = CCUPacketTypes.InitCommand(
        target: CCUPacketTypes.kBroadcastTarget,
        commandId: CCUPacketTypes.CommandID.ChangeConfiguration,
        category: CCUPacketTypes.Category.Metadata,
        parameter: CCUPacketTypes.MetadataParameter.SceneTags.rawValue,
        operationType: CCUPacketTypes.OperationType.AssignValue,
        dataType: CCUPacketTypes.DataTypes.kInt8,
        data: data)

    return command
}

public static func CreateMetadataTakeCommand(_ takeNumber: UInt8, _ takeTag: CCUPacketTypes.MetadataTakeTag) -> (CCUPacketTypes.Command?) {
    var data = [UInt8](repeating: 0, count: 2)
    data[0] = UInt8(takeNumber)
    data[1] = UInt8(bitPattern: takeTag.rawValue)

    let command = CCUPacketTypes.InitCommand(
        target: CCUPacketTypes.kBroadcastTarget,
        commandId: CCUPacketTypes.CommandID.ChangeConfiguration,
        category: CCUPacketTypes.Category.Metadata,
        parameter: CCUPacketTypes.MetadataParameter.Take.rawValue,
        operationType: CCUPacketTypes.OperationType.AssignValue,
        dataType: CCUPacketTypes.DataTypes.kInt8,
        data: data)

    return command
}

// Generic encoding functions
public static func CreateCommandFromArray<T: CCUDataType>(_ value: [T], _ category: CCUPacketTypes.Category, _ parameter: UInt8) -> (CCUPacketTypes.Command?) {

    let data: [UInt8] = UtilityFunctions.ToByteArrayFromArray(value)
    let command = CCUPacketTypes.InitCommand(
        target: CCUPacketTypes.kBroadcastTarget,
        commandId: CCUPacketTypes.CommandID.ChangeConfiguration,
        category: category,
        parameter: parameter,
        operationType: CCUPacketTypes.OperationType.AssignValue,
        dataType: T.getCCUDataType(),
        data: data)

    return command
}

public static func CreateStringCommand(_ string: String, _ category: CCUPacketTypes.Category, _ parameter: UInt8, maxLength: Int8 = -1) -> (CCUPacketTypes.Command?) {
    let data: [UInt8] = Array(string.utf8)

    if maxLength >= 0 && data.count > Int(maxLength) {
        Logger.LogError("String length (\(data.count)) exceeds maximum (\(maxLength)). Cannot create CCU command.")
        return nil
    }

    let command = CCUPacketTypes.InitCommand(
        target: CCUPacketTypes.kBroadcastTarget,
        commandId: CCUPacketTypes.CommandID.ChangeConfiguration,
        category: category,
        parameter: parameter,
        operationType: CCUPacketTypes.OperationType.AssignValue,
        dataType: CCUPacketTypes.DataTypes.kString,
        data: data)

    return command
}

    */
// }