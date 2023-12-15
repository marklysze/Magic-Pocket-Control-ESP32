#include "PacketWriter.h"
#include "CCU/CCUPacketTypes.h"

void PacketWriter::validateAndSendCCUCommand(CCUPacketTypes::Command command, BMDCameraConnection* connection, bool response = true)
{
    bool packetIsValid = CCUValidationFunctions::ValidateCCUPacket(command.serialize());
    if(packetIsValid)
        connection->sendCommandToOutgoing(command, response);
    else
        DEBUG_ERROR("PacketWriter::validateAndSendCCUCommand: Invalid Packet");
}

void PacketWriter::writeWhiteBalance(short whiteBalance, short tint, BMDCameraConnection* connection)
{
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateVideoWhiteBalanceCommand(whiteBalance, tint);
    validateAndSendCCUCommand(command, connection);
}

void PacketWriter::writeAutoWhiteBalance(BMDCameraConnection* connection)
{
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateVideoSetAutoWBCommand();
    validateAndSendCCUCommand(command, connection);
}

void PacketWriter::writeRecordingFormat(CCUPacketTypes::RecordingFormatData recordingFormatData, BMDCameraConnection* connection)
{
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateRecordingFormatCommand(recordingFormatData);
    validateAndSendCCUCommand(command, connection);
}

void PacketWriter::writeApertureNormalised(short normalisedApertureValue, BMDCameraConnection* connection)
{
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateFixed16Command(normalisedApertureValue, CCUPacketTypes::Category::Lens, (byte)CCUPacketTypes::LensParameter::ApertureNormalised);
    validateAndSendCCUCommand(command, connection);
}

void PacketWriter::writeIris(short apertureValue, BMDCameraConnection* connection)
{
    DEBUG_DEBUG("writeIris, AV:");
    DEBUG_DEBUG(std::to_string(apertureValue).c_str());
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateFixed16Command(apertureValue, CCUPacketTypes::Category::Lens, (byte)CCUPacketTypes::LensParameter::ApertureFstop);
    validateAndSendCCUCommand(command, connection);
}

void PacketWriter::writeShutterSpeed(int shutter, BMDCameraConnection* connection)
{
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateCommand(shutter, CCUPacketTypes::Category::Video, (byte)CCUPacketTypes::VideoParameter::ShutterSpeed);
    validateAndSendCCUCommand(command, connection);
}

void PacketWriter::writeShutterAngle(int shutterAngleX100, BMDCameraConnection* connection)
{
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateShutterAngleCommand(shutterAngleX100);
    validateAndSendCCUCommand(command, connection);
}

void PacketWriter::writeSensorGain(int sensorGain, BMDCameraConnection* connection)
{
    byte sensorGainValue = (byte)(sensorGain / VideoConfig::kSentSensorGainBase);
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateVideoSensorGainCommand(sensorGainValue);
    validateAndSendCCUCommand(command, connection);
}

void PacketWriter::writeISO(int iso, BMDCameraConnection* connection)
{
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateVideoISOCommand(iso);
    validateAndSendCCUCommand(command, connection);
}

void PacketWriter::writeTransportInfo(TransportInfo transportInfo, BMDCameraConnection* connection)
{
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateTransportInfoCommand(transportInfo);
    validateAndSendCCUCommand(command, connection);
}

void PacketWriter::writeCodec(CodecInfo codecInfo, BMDCameraConnection* connection)
{
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateCodecCommand(codecInfo);
    validateAndSendCCUCommand(command, connection);
}

void PacketWriter::writeAutoFocus(BMDCameraConnection* connection)
{
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateVoidCommand(CCUPacketTypes::Category::Lens, (byte)CCUPacketTypes::LensParameter::AutoFocus);
    validateAndSendCCUCommand(command, connection);
}

// Focus position is 0 to 65435
// NOTE - this uses an offset of the previous value (which works on the URSA Mini G2, whereas a set value does not)
void PacketWriter::writeFocusPositionWithOffset(int32_t focusPositionOffset, BMDCameraConnection* connection)
{
    float focusFloatPosition = static_cast<float>(focusPositionOffset) / static_cast<float>(65435);

    ccu_fixed_t fixedFromFloat = CCUPacketTypes::CCUFixedFromFloat(static_cast<float>(focusPositionOffset) / static_cast<float>(65435));

    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateFixed16Command(fixedFromFloat, CCUPacketTypes::Category::Lens, (byte)CCUPacketTypes::LensParameter::Focus, CCUPacketTypes::OperationType::OffsetValue);

    validateAndSendCCUCommand(command, connection);
}

// Focus position is 0 to 65435
// NOTE - this uses the specific focus position to move to
void PacketWriter::writeFocusPositionWithActual(int32_t focusPosition, BMDCameraConnection* connection)
{
    float focusFloatPosition = static_cast<float>(focusPosition) / static_cast<float>(65435);

    ccu_fixed_t fixedFromFloat = CCUPacketTypes::CCUFixedFromFloat(static_cast<float>(focusPosition) / static_cast<float>(65435));

    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateFixed16Command(fixedFromFloat, CCUPacketTypes::Category::Lens, (byte)CCUPacketTypes::LensParameter::Focus, CCUPacketTypes::OperationType::AssignValue);

    validateAndSendCCUCommand(command, connection);
}

// Focus position is 0.0 (near) to 1.0 (far)
void PacketWriter::writeFocusNormalised(float focusPosition, BMDCameraConnection* connection)
{
    int16_t shortFocusPosition = CCUPacketTypes::toFixed16(focusPosition);

    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateFixed16Command(shortFocusPosition, CCUPacketTypes::Category::Lens, (byte)CCUPacketTypes::LensParameter::Focus);

    validateAndSendCCUCommand(command, connection);
}

// Zoom position (can't confirm this works as I don't have the lens, let me know if it doesn't)
void PacketWriter::writeZoomMM(short zoomPositionMM, BMDCameraConnection* connection)
{
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateInt16Command(zoomPositionMM, CCUPacketTypes::Category::Lens, (byte)CCUPacketTypes::LensParameter::Zoom);
    validateAndSendCCUCommand(command, connection);
}

// Focus position is 0.0 (widest) to 1.0 (telephoto)
void PacketWriter::writeZoomNormalised(float zoomPosition, BMDCameraConnection* connection)
{
    int16_t shortZoomPosition = CCUPacketTypes::toFixed16(zoomPosition);

    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateFixed16Command(shortZoomPosition, CCUPacketTypes::Category::Lens, (byte)CCUPacketTypes::LensParameter::ZoomNormalised);

    validateAndSendCCUCommand(command, connection);
}