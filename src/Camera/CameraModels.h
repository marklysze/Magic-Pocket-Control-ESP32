#ifndef CAMERAMODELS_H
#define CAMERAMODELS_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include "ConstantsTypes.h"

enum class CameraModel : byte {
    Unknown = 0,
    CinemaCamera = 1,
    PocketCinemaCamera = 2,
    ProductionCamera4K = 3,
	StudioCamera = 4,
	StudioCamera4K = 5,
	URSA = 6,
	MicroCinemaCamera = 7,
	MicroStudioCamera = 8,
	URSAMini = 9,
	URSAMiniPro = 10,
	URSABroadcast = 11,
	URSAMiniProG2 = 12,
	PocketCinemaCamera4K = 13,
	PocketCinemaCamera6K = 14,
	PocketCinemaCamera6KPro = 15,
	URSAMiniPro12K = 16,
	URSABroadcastG2 = 17,
	StudioCamera4KPlus = 18,
	StudioCamera4KPro = 19,
	PocketCinemaCamera6KG2 = 20,
	StudioCamera4KExtreme = 21
};

class CameraModels {
public:
    static CameraModel fromValue(const byte value);
    // static CameraModel fromName(const std::string& name);
    static bool isPocket(const CameraModel model);
    static const std::unordered_map<std::string, CameraModel> nameToModel;
    static const std::unordered_map<CameraModel, std::string> modelToName;

    static const short NumberOfCameraModels = 21;
};

#endif // CAMERAMODELS_H
