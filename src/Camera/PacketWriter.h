#ifndef PACKETWRITER_H
#define PACKETWRITER_H

#include "Arduino_DebugUtils.h"
#include "CCU/CCUEncodingFunctions.h"
#include "CCU/CCUPacketTypes.h"
#include "CCU/CCUValidationFunctions.h"
#include "BMDCamera.h"
#include "BMDCameraConnection.h"

class PacketWriter
{
    public:
        static void validateAndSendCCUCommand(CCUPacketTypes::Command command, BMDCameraConnection* connection, bool response);
        static void writeWhiteBalance(short whiteBalance, short tint, BMDCameraConnection* connection);
        static void writeAutoWhiteBalance(BMDCameraConnection* connection);
        static void writeRecordingFormatStatus(BMDCameraConnection* connection);
        static void writeRecordingFormat(CCUPacketTypes::RecordingFormatData recordingFormatData, BMDCameraConnection* connection);
        static void writeApertureNormalised(short normalisedApertureValue, BMDCameraConnection* connection);
        static void writeIris(short apertureValue, BMDCameraConnection* connection);
        static void writeShutterSpeed(int shutter, BMDCameraConnection* connection);
        static void writeShutterAngle(int shutterAngleX100, BMDCameraConnection* connection);
        static void writeSensorGain(int sensorGain, BMDCameraConnection* connection);
        static void writeISO(int iso, BMDCameraConnection* connection);
        static void writeTransportInfo(TransportInfo transportInfo, BMDCameraConnection* connection);
        static void writeCodec(CodecInfo codecInfo, BMDCameraConnection* connection);
        static void writeAutoFocus(BMDCameraConnection* connection);
        static void writeFocusPositionWithOffset(int32_t focusPosition, BMDCameraConnection* connection);
        static void writeFocusPositionWithActual(int32_t focusPosition, BMDCameraConnection* connection);
        static void writeFocusNormalised(float focusPosition, BMDCameraConnection* connection);
        static void writeZoomMM(short zoomPositionMM, BMDCameraConnection* connection);
        static void writeZoomNormalised(float zoomPosition, BMDCameraConnection* connection);
};

#endif