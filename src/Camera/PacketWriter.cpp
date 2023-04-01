#include "PacketWriter.h"

void PacketWriter::validateAndSendCCUCommand(CCUPacketTypes::Command command, BMDCameraConnection* connection)
{
    bool packetIsValid = CCUValidationFunctions::ValidateCCUPacket(command.serialize());
    if(packetIsValid) {
        Serial.println("PacketWriter::validateAndSendCCUCommand: Send Command");
        connection->sendCommandToOutgoing(command);
    }
    else
        Serial.println("PacketWriter::validateAndSendCCUCommand: Invalid Packet");
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

void PacketWriter::writeRecordingFormatStatus(BMDCameraConnection* connection)
{
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateRecordingFormatStatusCommand();
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
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateCommand(shutterAngleX100, CCUPacketTypes::Category::Video, (byte)CCUPacketTypes::VideoParameter::ShutterAngle);
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

void PacketWriter::writeTransportPacket(TransportInfo transportInfo, BMDCameraConnection* connection)
{
    CCUPacketTypes::Command command = CCUEncodingFunctions::CreateTransportInfoCommand(transportInfo);
    validateAndSendCCUCommand(command, connection);
}