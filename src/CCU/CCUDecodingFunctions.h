#ifndef CCUDECODINGFUNCTIONS_H
#define CCUDECODINGFUNCTIONS_H

//#pragma once
#include <sstream>
#include <iomanip>
#include <regex>
#include "CCUPacketTypes.h"
#include "CCUValidationFunctions.h"
#include "CCUUtility.h"
#include "Camera\BMDCamera.h"
#include "Config\LensConfig.h"
#include "Config\VideoConfig.h"
#include "Camera\CodecInfo.h"
#include "Camera\CameraModels.h"
#include "Camera\TransportInfo.h"
// #include "Camera\BMDCameraConnection.h"

//class BMDCameraConnection; // forward declaration as both header files include each other.

class CCUDecodingFunctions
{
public:
    static void DecodeCCUPacket(const byte* byteArray, int length);

    static void DecodePayloadData(CCUPacketTypes::Category category, byte parameter, byte* payloadData, int payloadDataLength);

    template<typename T>
    static std::vector<T> ConvertPayloadDataWithExpectedCount(byte* data, int byteCount, int expectedCount);
    // template <typename T>
    // static std::vector<T> ConvertPayloadData(const std::vector<byte>& data);
    static std::string ConvertPayloadDataToString(byte* data, int byteCount);


    static void DecodeLensCategory(byte parameter, byte* payloadData, int payloadLength);
    static float ConvertCCUApertureToFstop(int16_t ccuAperture);
    static float CCUFloatFromFixed(ccu_fixed_t f);
    static void DecodeApertureFStop(byte* inData, int inDataLength);
    static void DecodeApertureNormalised(byte* inData, int inDataLength);
    static void DecodeAutoFocus(byte* inData, int inDataLength);
    static void DecodeZoom(byte* inData, int inDataLength);

    static void DecodeVideoCategory(byte parameter, byte* payloadData, int payloadDataLength);
    static void DecodeSensorGain(byte* inData, int inDataLength);
    static void DecodeManualWB(byte* inData, int inDataLength);
    static void DecodeExposure(byte* inData, int inDataLength);
    static void DecodeRecordingFormat(byte* inData, int inDataLength);
    static void DecodeAutoExposureMode(byte* inData, int inDataLength);
    static void DecodeShutterAngle(byte* inData, int inDataLength);
    static void DecodeShutterSpeed(byte* inData, int inDataLength);
    static void DecodeGain(byte* inData, int inDataLength);
    static void DecodeISO(byte* inData, int inDataLength);
    static void DecodeDisplayLUT(byte* inData, int inDataLength);

    static void DecodeStatusCategory(byte parameter, byte* payloadData, int payloadDataLength);
    static void DecodeBattery(byte* inData, int inDataLength);
    static void DecodeCameraSpec(byte* inData, int inDataLength);
    static void DecodeMediaStatus(byte* inData, int inDataLength);
    static void DecodeRemainingRecordTime(byte* inData, int inDataLength);

    static void DecodeMediaCategory(byte parameter, byte* payloadData, int payloadDataLength);
    static void DecodeCodec(byte* inData, int inDataLength);
    static void DecodeTransportMode(byte* inData, int inDataLength);

    static void DecodeMetadataCategory(byte parameter, byte* payloadData, int payloadDataLength);
    static void DecodeReel(byte* inData, int inDataLength);
    static void DecodeScene(byte* inData, int inDataLength);
    static void DecodeSceneTags(byte* inData, int inDataLength);
    static void DecodeTake(byte* inData, int inDataLength);
    static void DecodeGoodTake(byte* inData, int inDataLength);
    static void DecodeCameraId(byte* inData, int inDataLength);
    static void DecodeCameraOperator(byte* inData, int inDataLength);
    static void DecodeDirector(byte* inData, int inDataLength);
    static void DecodeProjectName(byte* inData, int inDataLength);
    static void DecodeSlateForType(byte* inData, int inDataLength);
    static void DecodeSlateForName(byte* inData, int inDataLength);
    static void DecodeLensFocalLength(byte* inData, int inDataLength);
    static void DecodeLensDistance(byte* inData, int inDataLength);
    static void DecodeLensType(byte* inData, int inDataLength);
    static void DecodeLensIris(byte* inData, int inDataLength);

    // For DecodeRemainingRecordTime function
    struct SecondsWithOverflow {
        uint16_t seconds;
        bool over;
    };

    static SecondsWithOverflow simplifyTime(int16_t time);
    static String makeTimeLabel(SecondsWithOverflow time);

};

#endif