#include "BMDCamera.h"

// By default let's output settings to serial
#ifndef OUTPUT_CAMERA_SETTINGS
    #define OUTPUT_CAMERA_SETTINGS 1
#endif

BMDCamera::BMDCamera() {
    setAsDisconnected();
}

BMDCamera::~BMDCamera() {}

void BMDCamera::setAsConnected()
{
    connected = true;
}

void BMDCamera::setAsDisconnected()
{
    connected = false;
}

//
// Quick Access Functions
//
bool BMDCamera::hasRecordError()
{
    for(int i = 0; i < mediaSlots.size(); i++)
    {
      if(mediaSlots[i].status == CCUPacketTypes::MediaStatus::RecordError)
        return true;
    }

    return false;
}

bool BMDCamera::isPocket4K6K()
{
    if(hasModelName())
    {
        // Loop up the map, passing in the model name
        auto cameraModel = CameraModels::nameToModel.find(getModelName());

        if (cameraModel != CameraModels::nameToModel.end()) {
            CameraModel model = cameraModel->second; // Enum Value part of map

            switch(model)
            {
                case CameraModel::PocketCinemaCamera4K:
                case CameraModel::PocketCinemaCamera6K:
                case CameraModel::PocketCinemaCamera6KG2:
                case CameraModel::PocketCinemaCamera6KPro:
                    return true;
                default:
                    return false;
            }
        }
        else {
            // the camera was not found in the map
            DEBUG_ERROR("BMDCamera::isPocket4K6K - Could not map name to model");
            return false;
        }
    }
    else
    {
        DEBUG_ERROR("BMDCamera::isPocket4K6K - Called but do not have model name");
        return false;
    }
}

bool BMDCamera::isPocket4K()
{
    if(hasModelName())
    {
        // Loop up the map, passing in the model name
        auto cameraModel = CameraModels::nameToModel.find(getModelName());

        if (cameraModel != CameraModels::nameToModel.end()) {
            CameraModel model = cameraModel->second; // Enum Value part of map

            return model == CameraModel::PocketCinemaCamera4K;
        }
        else {
            // the camera was not found in the map
            DEBUG_ERROR("BMDCamera::isPocket4K - Could not map name to model");
            return false;
        }
    }
    else
    {
        DEBUG_ERROR("BMDCamera::isPocket4K - Called but do not have model name");
        return false;
    }
}

bool BMDCamera::isPocket6K()
{
    if(hasModelName())
    {
        // Look up the map, passing in the model name
        auto cameraModel = CameraModels::nameToModel.find(getModelName());

        if (cameraModel != CameraModels::nameToModel.end()) {
            CameraModel model = cameraModel->second; // Enum Value part of map

            switch(model)
            {
                case CameraModel::PocketCinemaCamera6K:
                case CameraModel::PocketCinemaCamera6KG2:
                case CameraModel::PocketCinemaCamera6KPro:
                    return true;
                default:
                    return false;
            }
        }
        else {
            // the camera was not found in the map
            DEBUG_ERROR("BMDCamera::isPocket6K - Could not map name to model");
            return false;
        }
    }
    else
    {
        DEBUG_ERROR("BMDCamera::isPocket6K - Called but do not have model name");
        return false;
    }
}


bool BMDCamera::isURSAMiniProG2()
{
    if(hasModelName())
    {
        // Loop up the map, passing in the model name
        auto cameraModel = CameraModels::nameToModel.find(getModelName());

        if (cameraModel != CameraModels::nameToModel.end()) {
            CameraModel model = cameraModel->second; // Enum Value part of map

            switch(model)
            {
                case CameraModel::URSAMiniProG2:
                    return true;
                default:
                    return false;
            }
        }
        else {
            // the camera was not found in the map
            DEBUG_ERROR("BMDCamera::isURSAMiniProG2 - Could not map name to model");
            return false;
        }
    }
    else
    {
        DEBUG_ERROR("BMDCamera::isURSAMiniProG2 - Called but do not have model name");
        return false;
    }
}

bool BMDCamera::isURSAMiniPro12K()
{
    if(hasModelName())
    {
        // Loop up the map, passing in the model name
        auto cameraModel = CameraModels::nameToModel.find(getModelName());

        if (cameraModel != CameraModels::nameToModel.end()) {
            CameraModel model = cameraModel->second; // Enum Value part of map

            switch(model)
            {
                case CameraModel::URSAMiniPro12K:
                    return true;
                default:
                    return false;
            }
        }
        else {
            // the camera was not found in the map
            DEBUG_ERROR("BMDCamera::URSAMiniPro12K - Could not map name to model");
            return false;
        }
    }
    else
    {
        DEBUG_ERROR("BMDCamera::URSAMiniPro12K - Called but do not have model name");
        return false;
    }
}

//
// LENS Attributes
//

void BMDCamera::onHasLens(bool inHasLens)
{
    if(hasLens)
        *hasLens = inHasLens;
    else
        hasLens = std::make_shared<bool>(inHasLens);

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>HasLens:%s", (hasLens ? "Yes" : "No"));
    #endif
}
bool BMDCamera::hasHasLens()
{
    return static_cast<bool>(hasLens);
}
bool BMDCamera::getHasLens()
{
    if(hasLens)
        return *hasLens;
    else
        throw std::runtime_error("Has Lens not assigned to.");
}

void BMDCamera::onApertureUnitsReceived(LensConfig::ApertureUnits inApertureUnits)
{
    if(apertureUnits)
        *apertureUnits = inApertureUnits;
    else
        apertureUnits = std::make_shared<LensConfig::ApertureUnits>(inApertureUnits);

    modified();
}
bool BMDCamera::hasApertureUnits()
{
    return static_cast<bool>(apertureUnits);
}
LensConfig::ApertureUnits BMDCamera::getApertureUnits()
{
    if(apertureUnits)
        return *apertureUnits;
    else
        throw std::runtime_error("Aperture Units not assigned to.");
}
std::string BMDCamera::getApertureUnitsString()
{
    if(apertureUnits)
        return CCUPacketTypesString::GetEnumString(*apertureUnits);
    else
        throw std::runtime_error("Aperture Units not assigned to.");
}

void BMDCamera::onApertureFStopStringReceived(std::string inApertureFStopString)
{
    if(aperturefStopString)
        *aperturefStopString = inApertureFStopString;
    else
        aperturefStopString = std::make_shared<std::string>(inApertureFStopString);

    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>Aperture:%s", aperturefStopString->c_str());
    #endif
}
bool BMDCamera::hasApertureFStopString()
{
    return static_cast<bool>(aperturefStopString);
}
std::string BMDCamera::getApertureFStopString()
{
    if(aperturefStopString)
        return *aperturefStopString;
    else
        throw std::runtime_error("Aperture F-Stop String not assigned to.");
}

void BMDCamera::onApertureNormalisedReceived(int inApertureNormalised)
{
    if(apertureNormalised)
        *apertureNormalised = inApertureNormalised;
    else
        apertureNormalised = std::make_shared<int>(inApertureNormalised);

    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>ApertureNormalised:%i", *apertureNormalised);
    #endif
}
bool BMDCamera::hasApertureNormalised()
{
    return static_cast<bool>(apertureNormalised);
}
int BMDCamera::getApertureNormalised()
{
    if(apertureNormalised)
        return *apertureNormalised;
    else
        throw std::runtime_error("Aperture Normalised not assigned to.");
}

void BMDCamera::onFocalLengthMMReceived(ccu_fixed_t inFocalLengthMM)
{
    if(focalLengthMM)
        *focalLengthMM = inFocalLengthMM;
    else
        focalLengthMM = std::make_shared<ccu_fixed_t>(inFocalLengthMM);

    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>FocalLengthMM:%i", *focalLengthMM);
    #endif
}
bool BMDCamera::hasFocalLengthMM()
{
    return static_cast<bool>(focalLengthMM);
}
ccu_fixed_t BMDCamera::getFocalLengthMM()
{
    if(focalLengthMM)
        return *focalLengthMM;
    else
        throw std::runtime_error("Focal Length MM not assigned to.");
}

void BMDCamera::OnImageStabilisationReceived(bool inImageStabilisation)
{
    if(imageStabilisation)
        *imageStabilisation = inImageStabilisation;
    else
        imageStabilisation = std::make_shared<bool>(inImageStabilisation);

    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>ImageStabilisation:%s", (imageStabilisation ? "Yes" : "No"));
    #endif
}
bool BMDCamera::hasImageStabilisation()
{
    return static_cast<bool>(imageStabilisation);
}
bool BMDCamera::getImageStabilisation()
{
    if(imageStabilisation)
        return *imageStabilisation;
    else
        throw std::runtime_error("Image Stabilisation not assigned to.");
}

// When auto focus button is pressed
void BMDCamera::onAutoFocusPressed()
{
    DEBUG_VERBOSE("Auto Focus button pressed.");
}

const std::vector<BMDCamera::MediaSlot> BMDCamera::getMediaSlots()
{
    return mediaSlots;
}

std::string BMDCamera::getSlotActiveStorageMediumString(int slotIndex)
{
    if(mediaSlots.size() > slotIndex)
    {
        return CCUPacketTypesString::GetEnumString(mediaSlots[slotIndex].medium);
    }
    else
        throw std::runtime_error("Invalid Media Slot index.");
}

std::string BMDCamera::getSlotMediumStatusString(int slotIndex)
{
    if(mediaSlots.size() > slotIndex)
    {
        return CCUPacketTypesString::GetEnumString(mediaSlots[slotIndex].status);
    }
    else
        throw std::runtime_error("Invalid Media Slot index.");
}

// Is there a known active media slot
bool BMDCamera::hasActiveMediaSlot()
{
    return activeMediaSlotIndex != -1;
}

BMDCamera::MediaSlot BMDCamera::getActiveMediaSlot()
{
    if(mediaSlots.size() > activeMediaSlotIndex)
    {
        return mediaSlots[activeMediaSlotIndex];
    }
    else
        throw std::runtime_error("Active Media Slot index not set or media slots not populated.");
}


//
// VIDEO Attributes
//

void BMDCamera::onSensorGainISOReceived(int inSensorGainISO)
{
    if (sensorGainISO)
        *sensorGainISO = inSensorGainISO;
    else
        sensorGainISO = std::make_shared<int>(inSensorGainISO);

    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>ISO:%i", *sensorGainISO);
    #endif
}

bool BMDCamera::hasSensorGainISO()
{
    return static_cast<bool>(sensorGainISO);
}

int BMDCamera::getSensorGainISO()
{
    if (sensorGainISO)
        return *sensorGainISO;
    else
        throw std::runtime_error("Sensor Gain ISO not assigned to.");
}

void BMDCamera::onWhiteBalanceReceived(short inWhiteBalance)
{
    if (whiteBalance)
        *whiteBalance = inWhiteBalance;
    else
        whiteBalance = std::make_shared<short>(inWhiteBalance);

    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>WhiteBalance:%d", *whiteBalance);
    #endif
}

bool BMDCamera::hasWhiteBalance()
{
    return static_cast<bool>(whiteBalance);
}

short BMDCamera::getWhiteBalance()
{
    if (whiteBalance)
        return *whiteBalance;
    else
        throw std::runtime_error("White Balance not assigned to.");
}

void BMDCamera::onTintReceived(short inTint)
{
    if (tint)
        *tint = inTint;
    else
        tint = std::make_shared<short>(inTint);

    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>Tint:%d", *tint);
    #endif
}

bool BMDCamera::hasTint()
{
    return static_cast<bool>(tint);
}

short BMDCamera::getTint()
{
    if (tint)
        return *tint;
    else
        throw std::runtime_error("Tint not assigned to.");
}

void BMDCamera::onShutterSpeedMSReceived(int32_t inShutterSpeedMS)
{
    if (shutterSpeedMS)
        *shutterSpeedMS = inShutterSpeedMS;
    else
        shutterSpeedMS = std::make_shared<int32_t>(inShutterSpeedMS);

    modified();
}

bool BMDCamera::hasShutterSpeedMS()
{
    return static_cast<bool>(shutterSpeedMS);
}

int32_t BMDCamera::getShutterSpeedMS()
{
    if (shutterSpeedMS)
        return *shutterSpeedMS;
    else
        throw std::runtime_error("Shutter Speed (ms) not assigned to.");
}

void BMDCamera::onRecordingFormatReceived(CCUPacketTypes::RecordingFormatData inModelName)
{
    if (recordingFormat)
        *recordingFormat = inModelName;
    else
        recordingFormat = std::make_shared<CCUPacketTypes::RecordingFormatData>(inModelName);

    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>FrameRate:%s", recordingFormat->frameRate_string().c_str());
        DEBUG_INFO(">>FrameDims:%s", recordingFormat->frameWidthHeight_string().c_str());
        DEBUG_INFO(">>FrameSize:%s", recordingFormat->frameDimensionsShort_string().c_str());
        DEBUG_INFO(">>mRateEnabled:%s", recordingFormat->mRateEnabled ? "Yes" : "No");
        DEBUG_INFO(">>offSpeedEnabled:%s", recordingFormat->offSpeedEnabled ? "Yes" : "No");
        DEBUG_INFO(">>interlacedEnabled:%s", recordingFormat->interlacedEnabled ? "Yes" : "No");
        DEBUG_INFO(">>windowedModeEnabled:%s", recordingFormat->windowedModeEnabled ? "Yes" : "No");
        DEBUG_INFO(">>sensorMRateEnabled:%s", recordingFormat->sensorMRateEnabled ? "Yes" : "No");
    #endif
}

bool BMDCamera::hasRecordingFormat()
{
    return static_cast<bool>(recordingFormat);
}

CCUPacketTypes::RecordingFormatData BMDCamera::getRecordingFormat()
{
    if (recordingFormat)
        return *recordingFormat;
    else
        throw std::runtime_error("Recording Format not assigned to.");
}

void BMDCamera::onAutoExposureModeReceived(CCUPacketTypes::AutoExposureMode inAutoExposureMode)
{
    if (autoExposureMode)
        *autoExposureMode = inAutoExposureMode;
    else
        autoExposureMode = std::make_shared<CCUPacketTypes::AutoExposureMode>(inAutoExposureMode);
    
    modified();
}

bool BMDCamera::hasAutoExposureMode()
{
    return static_cast<bool>(autoExposureMode);
}

CCUPacketTypes::AutoExposureMode BMDCamera::getAutoExposureMode()
{
    if (autoExposureMode)
        return *autoExposureMode;
    else
        throw std::runtime_error("Auto Exposure Mode not assigned to.");
}

void BMDCamera::onShutterAngleReceived(int32_t inShutterAngle)
{
    shutterValueIsAngle = true;

    if(shutterAngle)
        *shutterAngle = inShutterAngle;
    else
        shutterAngle = std::make_shared<int32_t>(inShutterAngle);

    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>ShutterAngle:%i", *shutterAngle);
    #endif
}

bool BMDCamera::hasShutterAngle()
{
    return static_cast<bool>(shutterAngle);
}

int32_t BMDCamera::getShutterAngle()
{
    if(shutterAngle)
        return *shutterAngle;
    else
        throw std::runtime_error("Shutter Angle not assigned to.");
}

void BMDCamera::onShutterSpeedReceived(int32_t inShutterSpeed)
{
    shutterValueIsAngle = false;
    
    if(shutterSpeed)
        *shutterSpeed = inShutterSpeed;
    else
        shutterSpeed = std::make_shared<int32_t>(inShutterSpeed);

    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>ShutterSpeed:%i", *shutterSpeed);
    #endif
}

bool BMDCamera::hasShutterSpeed()
{
    return static_cast<bool>(shutterSpeed);
}

int32_t BMDCamera::getShutterSpeed()
{
    if(shutterSpeed)
        return *shutterSpeed;
    else
        throw std::runtime_error("Shutter Speed not assigned to.");
}

void BMDCamera::onSensorGainDBReceived(byte inSensorGainDB)
{
    if(sensorGainDB)
        *sensorGainDB = inSensorGainDB;
    else
        sensorGainDB = std::make_shared<byte>(inSensorGainDB);
    
    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>SensorGainDB:%u", *sensorGainDB);
    #endif
}

bool BMDCamera::hasSensorGainDB()
{
    return static_cast<bool>(sensorGainDB);
}

byte BMDCamera::getSensorGainDB()
{
    if(sensorGainDB)
        return *sensorGainDB;
    else
        throw std::runtime_error("Sensor Gain DB not assigned to.");
}

void BMDCamera::onSensorGainISOValueReceived(int32_t inSensorGainISOValue)
{
    if(sensorGainISOValue)
        *sensorGainISOValue = inSensorGainISOValue;
    else
        sensorGainISOValue = std::make_shared<int32_t>(inSensorGainISOValue);
    
    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>ISO:%i", *sensorGainISOValue);
    #endif
}

bool BMDCamera::hasSensorGainISOValue()
{
    return static_cast<bool>(sensorGainISOValue);
}

int32_t BMDCamera::getSensorGainISOValue()
{
    if(sensorGainISOValue)
        return *sensorGainISOValue;
    else
        throw std::runtime_error("Sensor Gain ISO Value not assigned to.");
}

void BMDCamera::onSelectedLUTReceived(CCUPacketTypes::SelectedLUT inSelectedLUT)
{
    if(selectedLUT)
        *selectedLUT = inSelectedLUT;
    else
        selectedLUT = std::make_shared<CCUPacketTypes::SelectedLUT>(inSelectedLUT);
    
    modified();
}

bool BMDCamera::hasSelectedLUT()
{
    return static_cast<bool>(selectedLUT);
}

CCUPacketTypes::SelectedLUT BMDCamera::getSelectedLUT()
{
    if(selectedLUT)
        return *selectedLUT;
    else
        throw std::runtime_error("Selected LUT not assigned to.");
}

void BMDCamera::onSelectedLUTEnabledReceived(bool inSelectedLUTEnabled) {
    if (selectedLUTEnabled)
        *selectedLUTEnabled = inSelectedLUTEnabled;
    else
        selectedLUTEnabled = std::make_shared<bool>(inSelectedLUTEnabled);

    modified();
}
bool BMDCamera::hasSelectedLUTEnabled() {
    return static_cast<bool>(selectedLUTEnabled);
}
bool BMDCamera::getSelectedLUTEnabled() {
    if (selectedLUTEnabled) {
        return *selectedLUTEnabled;
    } else {
        throw std::runtime_error("Selected LUT Enabled not assigned to.");
    }
}

//
// STATUS Attributes
//

void BMDCamera::onBatteryReceived(CCUPacketTypes::BatteryStatusData inBattery)
{
    if (batteryStatus)
        *batteryStatus = inBattery;
    else
        batteryStatus = std::make_shared<CCUPacketTypes::BatteryStatusData>(inBattery);
    
    // Not updated modified at this stage for this as we're not using it and the battery gets updated often.
    // If you are using the battery value, reinstate this next line.
    // modified();
}
bool BMDCamera::hasBattery()
{
    return static_cast<bool>(batteryStatus);
}
CCUPacketTypes::BatteryStatusData BMDCamera::getBattery()
{
    if (batteryStatus)
        return *batteryStatus;
    else
        throw std::runtime_error("Battery status not assigned to.");
}


void BMDCamera::onModelNameReceived(std::string inModelName)
{
    if(modelName)
        modelName->assign(inModelName);
    else
        modelName = std::make_shared<std::string>(inModelName);

    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>ModelName:%s", modelName->c_str());
    #endif
}
bool BMDCamera::hasModelName()
{
    return static_cast<bool>(modelName);
}
std::string BMDCamera::getModelName()
{
    if(modelName)
        return *modelName;
    else
        throw std::runtime_error("Model Name not assigned to.");
}

void BMDCamera::onIsPocketReceived(bool inIsPocket)
{
    if(isPocketCamera)
        *isPocketCamera = inIsPocket;
    else
        isPocketCamera = std::make_shared<bool>(inIsPocket);
    
    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>IsPocketCamera:%s", isPocketCamera ? "Yes" : "No");
    #endif
}
bool BMDCamera::hasIsPocket()
{
    return static_cast<bool>(isPocketCamera);
}
bool BMDCamera::getIsPocket()
{
    if(isPocketCamera)
        return *isPocketCamera;
    else
        throw std::runtime_error("Is Pocket not assigned to.");
}

void BMDCamera::onMediaStatusReceived(std::vector<CCUPacketTypes::MediaStatus> inMediaStatuses)
{
    // Update Slots
    for(int i = 0; i < inMediaStatuses.size(); i++)
    {
        // Add a new slot if we don't have one created yet
        if(mediaSlots.size() < (i + 1))
        {
            MediaSlot newSlot;
            newSlot.status = inMediaStatuses[i];
            mediaSlots.push_back(newSlot);
        }
        else
        {
            // Update existing slots
            mediaSlots[i].status = inMediaStatuses[i];
        }
    }

    modified();
}

void BMDCamera::onRemainingRecordTimeMinsReceived(std::vector<ccu_fixed_t> inRecordTimeMins)
{
    // Update Slots
    for(int i = 0; i < inRecordTimeMins.size(); i++)
    {
        // Add a new slot if we don't have one created yet
        if(mediaSlots.size() < (i + 1))
        {
            MediaSlot newSlot;
            newSlot.remainingRecordTimeMinutes = inRecordTimeMins[i];
            mediaSlots.push_back(newSlot);
        }
        else
        {
            // Update existing slots
            mediaSlots[i].remainingRecordTimeMinutes = inRecordTimeMins[i];
        }
    }

    modified();
}

void BMDCamera::onRemainingRecordTimeStringReceived(std::vector<std::string> inRecordTimeStrings)
{
    // Update Slots
    for(int i = 0; i < inRecordTimeStrings.size(); i++)
    {
        // Add a new slot if we don't have one created yet
        if(mediaSlots.size() < (i + 1))
        {
            MediaSlot newSlot;
            newSlot.remainingRecordTimeString = inRecordTimeStrings[i];
            mediaSlots.push_back(newSlot);
        }
        else
        {
            // Update existing slots
            mediaSlots[i].remainingRecordTimeString = inRecordTimeStrings[i];
        }
    }

    modified();
}



// 
// MEDIA Attributes
//
void BMDCamera::onCodecReceived(CodecInfo inCodec)
{
    if(codec)
        *codec = inCodec;
    else
        codec = std::make_shared<CodecInfo>(inCodec);

    // Update the last known Codec value
    switch(inCodec.basicCodec)
    {
        case CCUPacketTypes::BasicCodec::BRAW:
            if(inCodec.codecVariant == CCUPacketTypes::CodecVariants::kBRAW3_1 || inCodec.codecVariant == CCUPacketTypes::CodecVariants::kBRAW5_1 || inCodec.codecVariant == CCUPacketTypes::CodecVariants::kBRAW8_1 ||
                inCodec.codecVariant == CCUPacketTypes::CodecVariants::kBRAW12_1 || inCodec.codecVariant == CCUPacketTypes::CodecVariants::kBRAW18_1)
            {
                lastKnownBRAWBitrate = inCodec;
                lastKnownBRAWIsBitrate = true;
            }
            else
            {
                lastKnownBRAWQuality = inCodec;
                lastKnownBRAWIsBitrate = false;
            }
            break;
        case CCUPacketTypes::BasicCodec::ProRes:
            lastKnownProRes = inCodec;
            break;
    }

    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>Codec:%s", inCodec.to_string().c_str());
    #endif
}
bool BMDCamera::hasCodec()
{
    return static_cast<bool>(codec);
}
CodecInfo BMDCamera::getCodec()
{
    if(codec)
        return *codec;
    else
        throw std::runtime_error("Codec not assigned to.");
}

void BMDCamera::onTransportModeReceived(TransportInfo inTransportMode)
{
    if(transportMode)
        *transportMode = inTransportMode;
    else
        transportMode = std::make_shared<TransportInfo>(inTransportMode);
    
    // Update Slots
    for(int i = 0; i < transportMode->slots.size(); i++)
    {
        // Add a new slot if we don't have one created yet
        if(mediaSlots.size() < (i + 1))
        {
            MediaSlot newSlot;
            newSlot.active = transportMode->slots[i].active;
            newSlot.medium = transportMode->slots[i].medium;
            mediaSlots.push_back(newSlot);
        }
        else
        {
            // Update existing slots
            mediaSlots[i].active = transportMode->slots[i].active;
            mediaSlots[i].medium = transportMode->slots[i].medium;
        }

        // Keep track of the active media slot
        if(mediaSlots[i].active)
            activeMediaSlotIndex = i;
    }

    bool changedRecordingState = isRecording != (transportMode->mode == CCUPacketTypes::MediaTransportMode::Record);
    isRecording = transportMode->mode == CCUPacketTypes::MediaTransportMode::Record;
    if(changedRecordingState) DEBUG_VERBOSE("isRecording: %s", (isRecording ? "Yes" : "No"));

    modified();

    if(changedRecordingState)
    {
        #if OUTPUT_CAMERA_SETTINGS == 1
            DEBUG_INFO(">>Recording:%s", isRecording ? "Yes" : "No");
        #endif
    }
}
bool BMDCamera::hasTransportMode()
{
    return static_cast<bool>(transportMode);
}
TransportInfo BMDCamera::getTransportMode()
{
    if(transportMode)
        return *transportMode;
    else
        throw std::runtime_error("Transport mode not assigned to.");
}




//
// METADATA Attributes
//

void BMDCamera::onReelNumberReceived(short inReelNumber, bool inIsEditable)
{
    if(reelNumber)
        *reelNumber = inReelNumber;
    else
        reelNumber = std::make_shared<short>(inReelNumber);

    if(reelEditable)
        *reelEditable = inIsEditable;
    else
        reelEditable = std::make_shared<bool>(inIsEditable);
    
    modified();
}
bool BMDCamera::hasReelNumber()
{
    return static_cast<bool>(reelNumber);
}
short BMDCamera::getReelNumber()
{
    if(reelNumber)
        return *reelNumber;
    else
        throw std::runtime_error("Reel Number not assigned to.");
}
bool BMDCamera::hasReelEditable()
{
    return static_cast<bool>(reelEditable);
}
bool BMDCamera::getReelEditable()
{
    if(reelEditable)
        return *reelEditable;
    else
        throw std::runtime_error("Reel Editable not assigned to.");
}

void BMDCamera::onSceneNameReceived(std::string inSceneName)
{
    if(sceneName)
        sceneName->assign(inSceneName);
    else
        sceneName = std::make_shared<std::string>(inSceneName);
    
    modified();
}
bool BMDCamera::hasSceneName()
{
    return static_cast<bool>(sceneName);
}
std::string BMDCamera::getSceneName()
{
    if(sceneName)
        return *sceneName;
    else
        throw std::runtime_error("Scene Name not assigned to.");
}

void BMDCamera::onSceneTagReceived(CCUPacketTypes::MetadataSceneTag inSceneTag)
{
    if(sceneTag)
        *sceneTag = inSceneTag;
    else
        sceneTag = std::make_shared<CCUPacketTypes::MetadataSceneTag>(inSceneTag);
    
    modified();
}
bool BMDCamera::hasSceneTag()
{
    return static_cast<bool>(sceneTag);
}
CCUPacketTypes::MetadataSceneTag BMDCamera::getSceneTag()
{
    if(sceneTag)
        return *sceneTag;
    else
        throw std::runtime_error("Scene Tag not assigned to.");
}

void BMDCamera::onLocationTypeReceived(CCUPacketTypes::MetadataLocationTypeTag inLocationType)
{
    if(locationType)
        *locationType = inLocationType;
    else
        locationType = std::make_shared<CCUPacketTypes::MetadataLocationTypeTag>(inLocationType);
    
    modified();
}
bool BMDCamera::hasLocationType()
{
    return static_cast<bool>(locationType);
}
CCUPacketTypes::MetadataLocationTypeTag BMDCamera::getLocationType()
{
    if(locationType)
        return *locationType;
    else
        throw std::runtime_error("Location Type not assigned to.");
}

void BMDCamera::onDayOrNightReceived(CCUPacketTypes::MetadataDayNightTag inDayOrNight)
{
    if(dayOrNight)
        *dayOrNight = inDayOrNight;
    else
        dayOrNight = std::make_shared<CCUPacketTypes::MetadataDayNightTag>(inDayOrNight);
    
    modified();
}
bool BMDCamera::hasDayOrNight()
{
    return static_cast<bool>(dayOrNight);
}
CCUPacketTypes::MetadataDayNightTag BMDCamera::getDayOrNight()
{
    if(dayOrNight)
        return *dayOrNight;
    else
        throw std::runtime_error("Day or Night not assigned to.");
}

void BMDCamera::onTakeTagReceived(CCUPacketTypes::MetadataTakeTag inTakeTag)
{
    if(takeTag)
        *takeTag = inTakeTag;
    else
        takeTag = std::make_shared<CCUPacketTypes::MetadataTakeTag>(inTakeTag);
    
    modified();
}
bool BMDCamera::hasTakeTag()
{
    return static_cast<bool>(takeTag);
}
CCUPacketTypes::MetadataTakeTag BMDCamera::getTakeTag()
{
    if(takeTag)
        return *takeTag;
    else
        throw std::runtime_error("Take Tag not assigned to.");
}

void BMDCamera::onTakeNumberReceived(sbyte inTakeNumber)
{
    if(takeNumber)
        *takeNumber = inTakeNumber;
    else
        takeNumber = std::make_shared<sbyte>(inTakeNumber);
    
    modified();
}
bool BMDCamera::hasTakeNumber()
{
    return static_cast<bool>(takeNumber);
}
sbyte BMDCamera::getTakeNumber()
{
    if(takeNumber)
        return *takeNumber;
    else
        throw std::runtime_error("Take Number not assigned to.");
}

void BMDCamera::onGoodTakeReceived(sbyte inGoodTake)
{
    if(goodTake)
        *goodTake = inGoodTake;
    else
        goodTake = std::make_shared<sbyte>(inGoodTake);
    
    modified();
}
bool BMDCamera::hasGoodTake()
{
    return static_cast<bool>(goodTake);
}
sbyte BMDCamera::getGoodTake()
{
    if(goodTake)
        return *goodTake;
    else
        throw std::runtime_error("Good Take not assigned to.");
}


void BMDCamera::onCameraIdReceived(std::string incameraId)
{
    if(cameraId)
        cameraId->assign(incameraId);
    else
        cameraId = std::make_shared<std::string>(incameraId);
    
    modified();
}
bool BMDCamera::hasCameraId()
{
    return static_cast<bool>(cameraId);
}
std::string BMDCamera::getCameraId()
{
    if(cameraId)
        return *cameraId;
    else
        throw std::runtime_error("Camera Id not assigned to.");
}


void BMDCamera::onCameraOperatorReceived(std::string incameraOperator)
{
    if(cameraOperator)
        cameraOperator->assign(incameraOperator);
    else
        cameraOperator = std::make_shared<std::string>(incameraOperator);
    
    modified();
}
bool BMDCamera::hasCameraOperator()
{
    return static_cast<bool>(cameraOperator);
}
std::string BMDCamera::getCameraOperator()
{
    if(cameraOperator)
        return *cameraOperator;
    else
        throw std::runtime_error("Camera Operator not assigned to.");
}


void BMDCamera::onDirectorReceived(std::string inDirector)
{
    if(director)
        director->assign(inDirector);
    else
        director = std::make_shared<std::string>(inDirector);
    
    modified();
}
bool BMDCamera::hasDirector()
{
    return static_cast<bool>(director);
}
std::string BMDCamera::getDirector()
{
    if(director)
        return *director;
    else
        throw std::runtime_error("Director not assigned to.");
}


void BMDCamera::onProjectNameReceived(std::string inProjectName)
{
    if(projectName)
        projectName->assign(inProjectName);
    else
        projectName = std::make_shared<std::string>(inProjectName);
    
    modified();
}
bool BMDCamera::hasProjectName()
{
    return static_cast<bool>(projectName);
}
std::string BMDCamera::getProjectName()
{
    if(projectName)
        return *projectName;
    else
        throw std::runtime_error("Project Name not assigned to.");
}


void BMDCamera::onSlateTypeReceived(CCUPacketTypes::MetadataSlateForType inslateType)
{
    if(slateType)
        *slateType = inslateType;
    else
        slateType = std::make_shared<CCUPacketTypes::MetadataSlateForType>(inslateType);
    
    modified();
}
bool BMDCamera::hasSlateType()
{
    return static_cast<bool>(slateType);
}
CCUPacketTypes::MetadataSlateForType BMDCamera::getSlateType()
{
    if(slateType)
        return *slateType;
    else
        throw std::runtime_error("Slate Type not assigned to.");
}


void BMDCamera::onSlateNameReceived(std::string inSlateName)
{
    if(slateName)
        slateName->assign(inSlateName);
    else
        slateName = std::make_shared<std::string>(inSlateName);
    
    modified();
}
bool BMDCamera::hasSlateName()
{
    return static_cast<bool>(slateName);
}
std::string BMDCamera::getSlateName()
{
    if(slateName)
        return *slateName;
    else
        throw std::runtime_error("Slate Name not assigned to.");
}


void BMDCamera::onLensFocalLengthReceived(std::string inLensFocalLength)
{
    if(lensFocalLength)
        lensFocalLength->assign(inLensFocalLength);
    else
        lensFocalLength = std::make_shared<std::string>(inLensFocalLength);
    
    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>FocalLength:%s", lensFocalLength->c_str());
    #endif
}
bool BMDCamera::hasLensFocalLength()
{
    return static_cast<bool>(lensFocalLength);
}
std::string BMDCamera::getLensFocalLength()
{
    if(lensFocalLength)
        return *lensFocalLength;
    else
        throw std::runtime_error("Lens Focal Length not assigned to.");
}


void BMDCamera::onLensDistanceReceived(std::string inLensDistance)
{
    if(lensDistance)
        lensDistance->assign(inLensDistance);
    else
        lensDistance = std::make_shared<std::string>(inLensDistance);
    
    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>LensDistance:%s", lensDistance->c_str());
    #endif
}
bool BMDCamera::hasLensDistance()
{
    return static_cast<bool>(lensDistance);
}
std::string BMDCamera::getLensDistance()
{
    if(lensDistance)
        return *lensDistance;
    else
        throw std::runtime_error("Lens Distance not assigned to.");
}


void BMDCamera::onLensTypeReceived(std::string inLensType)
{
    if(lensType)
        lensType->assign(inLensType);
    else
        lensType = std::make_shared<std::string>(inLensType);
    
    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>LensType:%s", lensType->c_str());
    #endif
}
bool BMDCamera::hasLensType()
{
    return static_cast<bool>(lensType);
}
std::string BMDCamera::getLensType()
{
    if(lensType)
        return *lensType;
    else
        throw std::runtime_error("Lens Type not assigned to.");
}

void BMDCamera::onLensIrisReceived(std::string inLensIris)
{
    if(lensIris)
        lensIris->assign(inLensIris);
    else
        lensIris = std::make_shared<std::string>(inLensIris);
    
    modified();

    #if OUTPUT_CAMERA_SETTINGS == 1
        DEBUG_INFO(">>LensIris:%s", lensIris->c_str());
    #endif
}
bool BMDCamera::hasLensIris()
{
    return static_cast<bool>(lensIris);
}
std::string BMDCamera::getLensIris()
{
    if(lensIris)
        return *lensIris;
    else
        return "";
}

// Timecode

void BMDCamera::onTimecodeSourceReceived(CCUPacketTypes::DisplayTimecodeSource inTimecodeSource)
{
    if(timecodeSource)
        *timecodeSource = inTimecodeSource;
    else
        timecodeSource = std::make_shared<CCUPacketTypes::DisplayTimecodeSource>(inTimecodeSource);
    
    modified();
}
bool BMDCamera::hasTimecodeSource()
{
    return static_cast<bool>(timecodeSource);
}
CCUPacketTypes::DisplayTimecodeSource BMDCamera::getTimecodeSource()
{
    if(timecodeSource)
        return *timecodeSource;
    else
        throw std::runtime_error("Timecode Source not assigned to.");
}

void BMDCamera::onTimecodeReceived(std::string inTimecode)
{
    if(timecode)
        *timecode = inTimecode;
    else
        timecode = std::make_shared<std::string>(inTimecode);
    
    modified();
}
std::string BMDCamera::getTimecodeString()
{
    if(timecode)
        return *timecode;
    else
        return "00:00:00:00";
}