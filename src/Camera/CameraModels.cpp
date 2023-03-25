#include "CameraModels.h"

CameraModel CameraModels::fromValue(const byte value) {

    CameraModel cameraModel = static_cast<CameraModel>(value);

    return cameraModel;
}

/*
CameraModel CameraModels::fromName(const std::string& name) {
    if (auto it = nameToModel.find(name); it != nameToModel.end()) {
        return it->second;
    }

    return CameraModel::Unknown;
}
*/

bool CameraModels::isPocket(const CameraModel model) {
    switch (model) {
        case CameraModel::PocketCinemaCamera:
        case CameraModel::PocketCinemaCamera4K:
        case CameraModel::PocketCinemaCamera6K:
        case CameraModel::PocketCinemaCamera6KG2:
        case CameraModel::PocketCinemaCamera6KPro:
            return true;
        default:
            return false;
    }
}

const std::unordered_map<std::string, CameraModel> CameraModels::nameToModel {
    {"Cinema Camera", CameraModel::CinemaCamera},
    {"Pocket Cinema Camera", CameraModel::PocketCinemaCamera},
    {"Micro Cinema Camera", CameraModel::MicroCinemaCamera},
    {"Micro Studio Camera", CameraModel::MicroStudioCamera},
    {"Pocket Cinema Camera", CameraModel::PocketCinemaCamera},
    {"Pocket Cinema Camera 4K", CameraModel::PocketCinemaCamera4K},
    {"Pocket Cinema Camera 6K", CameraModel::PocketCinemaCamera6K},
    {"Pocket Cinema Camera 6K G2", CameraModel::PocketCinemaCamera6KG2},
    {"Pocket Cinema Camera 6K Pro", CameraModel::PocketCinemaCamera6KPro},
    {"Production Camera 4K", CameraModel::ProductionCamera4K},
    {"Studio Camera", CameraModel::StudioCamera},
    {"Studio Camera 4K", CameraModel::StudioCamera4K},
    {"Studio Camera 4K Extreme", CameraModel::StudioCamera4KExtreme},
    {"Studio Camera 4K Plus", CameraModel::StudioCamera4KPlus},
    {"Studio Camera 4K Pro", CameraModel::StudioCamera4KPro},
    {"URSA", CameraModel::URSA},
    {"URSA Broadcast", CameraModel::URSABroadcast},
    {"URSA Broadcast G2", CameraModel::URSABroadcastG2},
    {"URSA Mini", CameraModel::URSAMini},
    {"URSA Mini Pro", CameraModel::URSAMiniPro},
    {"URSA Mini Pro 12K", CameraModel::URSAMiniPro12K},
    {"URSA Mini Pro G2", CameraModel::URSAMiniProG2}
};

const std::unordered_map<CameraModel, std::string> CameraModels::modelToName {
    {CameraModel::CinemaCamera, "Cinema Camera"},
    {CameraModel::PocketCinemaCamera, "Pocket Cinema Camera"},
    {CameraModel::MicroCinemaCamera, "Micro Cinema Camera"},
    {CameraModel::MicroStudioCamera, "Micro Studio Camera"},
    {CameraModel::PocketCinemaCamera, "Pocket Cinema Camera"},
    {CameraModel::PocketCinemaCamera4K, "Pocket Cinema Camera 4K"},
    {CameraModel::PocketCinemaCamera6K, "Pocket Cinema Camera 6K"},
    {CameraModel::PocketCinemaCamera6KG2, "Pocket Cinema Camera 6K G2"},
    {CameraModel::PocketCinemaCamera6KPro, "Pocket Cinema Camera 6K Pro"},
    {CameraModel::ProductionCamera4K, "Production Camera 4K"},
    {CameraModel::StudioCamera, "Studio Camera"},
    {CameraModel::StudioCamera4K, "Studio Camera 4K"},
    {CameraModel::StudioCamera4KExtreme, "Studio Camera 4K Extreme"},   
    {CameraModel::StudioCamera4KPlus, "Studio Camera 4K Plus"},
    {CameraModel::StudioCamera4KPro, "Studio Camera 4K Pro"},
    {CameraModel::URSA, "URSA"},
    {CameraModel::URSABroadcast, "URSA Broadcast"},
    {CameraModel::URSABroadcastG2, "URSA Broadcast G2"},
    {CameraModel::URSAMini, "URSA Mini"},
    {CameraModel::URSAMiniPro, "URSA Mini Pro"},
    {CameraModel::URSAMiniPro12K, "URSA Mini Pro 12K"},
    {CameraModel::URSAMiniProG2, "URSA Mini Pro G2"}
};
