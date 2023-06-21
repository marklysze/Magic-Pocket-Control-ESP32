#ifndef CCUDECODINGFUNCTIONS_H
#define CCUDECODINGFUNCTIONS_H

//#pragma once
#include <sstream>
#include <iomanip>
#include <regex>
#include "BMDControlSystem.h"
#include "CCUPacketTypes.h"
#include "CCUValidationFunctions.h"
#include "CCUUtility.h"
#include "Camera/BMDCamera.h"
#include "Config/LensConfig.h"
#include "Config/VideoConfig.h"
#include "Camera/CodecInfo.h"
#include "Camera/CameraModels.h"
#include "Camera/TransportInfo.h"

class CCUDecodingFunctions
{
public:
    static void DecodeCCUPacket(std::vector<byte> byteArray);

    static void DecodePayloadData(CCUPacketTypes::Category category, byte parameter, std::vector<byte> payloadData);

    template<typename T>
    static int GetCount(std::vector<byte> data);

    template<typename T>
    static std::vector<T> ConvertPayloadDataWithExpectedCount(std::vector<byte> data, int expectedCount);
    static std::string ConvertPayloadDataToString(std::vector<byte> data);


    static void DecodeLensCategory(byte parameter, std::vector<byte> payloadData);
    static float ConvertCCUApertureToFstop(int16_t ccuAperture);
    static float CCUFloatFromFixed(ccu_fixed_t f);
    static void DecodeApertureFStop(std::vector<byte> inData);
    static void DecodeApertureNormalised(std::vector<byte> inData);
    static void DecodeAutoFocus(std::vector<byte> inData);
    static void DecodeZoom(std::vector<byte> inData);
    static void DecodeImageStabilisation(std::vector<byte> inData);

    static void DecodeVideoCategory(byte parameter, std::vector<byte> payloadData);
    static void DecodeSensorGainISO(std::vector<byte> inData);
    static void DecodeManualWB(std::vector<byte> inData);
    static void DecodeExposure(std::vector<byte> inData);
    static void DecodeRecordingFormat(std::vector<byte> inData);
    static void DecodeAutoExposureMode(std::vector<byte> inData);
    static void DecodeShutterAngle(std::vector<byte> inData);
    static void DecodeShutterSpeed(std::vector<byte> inData);
    static void DecodeGain(std::vector<byte> inData);
    static void DecodeISO(std::vector<byte> inData);
    static void DecodeDisplayLUT(std::vector<byte> inData);

    static void DecodeStatusCategory(byte parameter, std::vector<byte> payloadData);
    static void DecodeBattery(std::vector<byte> inData);
    static void DecodeCameraSpec(std::vector<byte> inData);
    static void DecodeMediaStatus(std::vector<byte> inData);
    static void DecodeRemainingRecordTime(std::vector<byte> inData);

    static void DecodeMediaCategory(byte parameter, std::vector<byte> payloadData);
    static void DecodeCodec(std::vector<byte> inData);
    static void DecodeTransportMode(std::vector<byte> inData);

    static void DecodeMetadataCategory(byte parameter, std::vector<byte> payloadData);
    static void DecodeReel(std::vector<byte> inData);
    static void DecodeScene(std::vector<byte> inData);
    static void DecodeSceneTags(std::vector<byte> inData);
    static void DecodeTake(std::vector<byte> inData);
    static void DecodeGoodTake(std::vector<byte> inData);
    static void DecodeCameraId(std::vector<byte> inData);
    static void DecodeCameraOperator(std::vector<byte> inData);
    static void DecodeDirector(std::vector<byte> inData);
    static void DecodeProjectName(std::vector<byte> inData);
    static void DecodeSlateForType(std::vector<byte> inData);
    static void DecodeSlateForName(std::vector<byte> inData);
    static void DecodeLensFocalLength(std::vector<byte> inData);
    static void DecodeLensDistance(std::vector<byte> inData);
    static void DecodeLensType(std::vector<byte> inData);
    static void DecodeLensIris(std::vector<byte> inData);

    static void DecodeDisplayCategory(byte parameter, std::vector<byte> payloadData);
    static void DecodeTimecodeSource(std::vector<byte> inData);

    // For DecodeRemainingRecordTime function
    struct SecondsWithOverflow {
        uint16_t seconds;
        bool over;
    };

    static void TimecodeToString(std::vector<byte> timecodeBytes);

    static SecondsWithOverflow simplifyTime(int16_t time);
    static std::string makeTimeLabel(SecondsWithOverflow time);

};

#endif