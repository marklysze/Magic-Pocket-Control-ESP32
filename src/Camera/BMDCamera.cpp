#include "BMDCamera.h"

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
// VIDEO Attributes
//

void BMDCamera::onSensorGainISOReceived(int inSensorGainISO)
{
    if (sensorGainISO)
        *sensorGainISO = inSensorGainISO;
    else
        sensorGainISO = std::make_shared<int>(inSensorGainISO);
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
    if(shutterAngle)
        *shutterAngle = inShutterAngle;
    else
        shutterAngle = std::make_shared<int32_t>(inShutterAngle);
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
    if(shutterSpeed)
        *shutterSpeed = inShutterSpeed;
    else
        shutterSpeed = std::make_shared<int32_t>(inShutterSpeed);
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
    if (selectedLUTEnabled) {
        *selectedLUTEnabled = inSelectedLUTEnabled;
    } else {
        selectedLUTEnabled = std::make_shared<bool>(inSelectedLUTEnabled);
    }
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

void BMDCamera::onModelNameReceived(std::string inModelName)
{
    if(modelName)
        modelName->assign(inModelName);
    else
        modelName = std::make_shared<std::string>(inModelName);
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

//
// METADATA Attributes
//

void BMDCamera::onReelNumberReceived(short inReelNumber)
{
    if(reelNumber)
        *reelNumber = inReelNumber;
    else
        reelNumber = std::make_shared<short>(inReelNumber);
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

void BMDCamera::onSceneNameReceived(std::string inSceneName)
{
    if(sceneName)
        sceneName->assign(inSceneName);
    else
        sceneName = std::make_shared<std::string>(inSceneName);
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