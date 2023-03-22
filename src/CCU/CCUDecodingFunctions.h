#ifndef CCUDECODINGFUNCTIONS_H
#define CCUDECODINGFUNCTIONS_H

//#pragma once
#include "CCUPacketTypes.h"
#include "CCUValidationFunctions.h"
#include "CCUUtility.h"
#include "Camera\BMDCamera.h"
#include "Config\LensConfig.h"
#include "Config\VideoConfig.h"
// #include "Camera\BMDCameraConnection.h"

//class BMDCameraConnection; // forward declaration as both header files include each other.

class CCUDecodingFunctions
{
public:
    static void DecodeCCUPacket(const byte* byteArray, int length);

    static void DecodePayloadData(CCUPacketTypes::Category category, byte parameter, byte* payloadData, int payloadDataLength);

/*

    static void DecodeLensCategory(byte parameter, byte* payloadData, int payloadLength);
*/
    // template<typename T>
    // static T* ConvertPayloadDataWithExpectedCount(byte* data, int byteCount, int expectedCount);

    template<typename T>
    static std::vector<T> ConvertPayloadDataWithExpectedCount(byte* data, int byteCount, int expectedCount);
/*
    static void DecodeApertureFStop(byte* inData, int inDataLength);

*/
    static void DecodeVideoCategory(byte parameter, byte* payloadData, int payloadDataLength);
    static void DecodeSensorGain(byte* inData, int inDataLength);
    static void DecodeManualWB(byte* inData, int inDataLength);
    static void DecodeExposure(byte* inData, int inDataLength);
    static void DecodeRecordingFormat(byte* inData, int inDataLength);
    static void DecodeISO(byte* inData, int inDataLength);

/*    
    static void DecodeApertureNormalised(byte* inData, int inDataLength);


    static void DecodeStatusCategory(byte parameter, byte* payloadData, int payloadDataLength);
    static void DecodeMediaCategory(byte parameter, byte* payloadData, int payloadDataLength);
    static void DecodeMetadataCategory(byte parameter, byte* payloadData, int payloadDataLength);

*/
};

#endif