#ifndef BMDCAMERA_H
#define BMDCAMERA_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include "Camera\ConstantsTypes.h"
#include "CCU\CCUPacketTypes.h"

class BMDCamera
{
public:
    BMDCamera();
    ~BMDCamera();

    void onIrisReceived(short apertureNumber, short infStopIndex);
    void onNormalisedApertureReceived(short normalisedAperture);

    void setAsConnected();
    void setAsDisconnected();

    // Video Attributes

    void onSensorGainISOReceived(int inSensorGainISO);
    bool hasSensorGainISO();
    int getSensorGainISO();

    void onWhiteBalanceReceived(short inWhiteBalance);
    bool hasWhiteBalance();
    short getWhiteBalance();

    void onTintReceived(short inTint);
    bool hasTint();
    short getTint();

    void onShutterSpeedMSReceived(int32_t inShutterSpeedMS);
    bool hasShutterSpeedMS();
    int32_t getShutterSpeedMS();

    void onRecordingFormatReceived(CCUPacketTypes::RecordingFormatData inModelName);
    bool hasRecordingFormat();
    CCUPacketTypes::RecordingFormatData getRecordingFormat();

    void onAutoExposureModeReceived(CCUPacketTypes::AutoExposureMode inAutoExposureMode);
    bool hasAutoExposureMode();
    CCUPacketTypes::AutoExposureMode getAutoExposureMode();

    void onShutterAngleReceived(int32_t inShutterAngle);
    bool hasShutterAngle();
    int32_t getShutterAngle();

    void onShutterSpeedReceived(int32_t inShutterSpeed);
    bool hasShutterSpeed();
    int32_t getShutterSpeed();

    void onSensorGainDBReceived(byte inSensorGainDB);
    bool hasSensorGainDB();
    byte getSensorGainDB();

    void onSensorGainISOValueReceived(int32_t inSensorGainISOValue);
    bool hasSensorGainISOValue();
    int32_t getSensorGainISOValue();

    void onSelectedLUTReceived(CCUPacketTypes::SelectedLUT inSelectedLUT);
    bool hasSelectedLUT();
    CCUPacketTypes::SelectedLUT getSelectedLUT();

    void onSelectedLUTEnabledReceived(bool inSelectedLUTEnabled);
    bool hasSelectedLUTEnabled();
    bool getSelectedLUTEnabled();


    // Status Attributes
    void onModelNameReceived(std::string inModelName);
    bool hasModelName();
    std::string getModelName();

    void onIsPocketReceived(bool inIsPocket);
    bool hasIsPocket();
    bool getIsPocket();


    // Metadata Attributes
    void onReelNumberReceived(short inReelNumber);
    bool hasReelNumber();
    short getReelNumber();

    void onSceneNameReceived(std::string inSceneName);
    bool hasSceneName();
    std::string getSceneName();

    void onSceneTagReceived(CCUPacketTypes::MetadataSceneTag inSceneTag);
    bool hasSceneTag();
    CCUPacketTypes::MetadataSceneTag getSceneTag();

    void onLocationTypeReceived(CCUPacketTypes::MetadataLocationTypeTag inLocationType);
    bool hasLocationType();
    CCUPacketTypes::MetadataLocationTypeTag getLocationType();

    void onDayOrNightReceived(CCUPacketTypes::MetadataDayNightTag inDayOrNight);
    bool hasDayOrNight();
    CCUPacketTypes::MetadataDayNightTag getDayOrNight();

    void onTakeTagReceived(CCUPacketTypes::MetadataTakeTag inTakeTag);
    bool hasTakeTag();
    CCUPacketTypes::MetadataTakeTag getTakeTag();

    void onTakeNumberReceived(sbyte inTakeNumber);
    bool hasTakeNumber();
    sbyte getTakeNumber();

    void onGoodTakeReceived(sbyte inGoodTake);
    bool hasGoodTake();
    sbyte getGoodTake();

    void onCameraIdReceived(std::string incameraId);
    bool hasCameraId();
    std::string getCameraId();

    void onCameraOperatorReceived(std::string incameraOperator);
    bool hasCameraOperator();
    std::string getCameraOperator();

    void onDirectorReceived(std::string inDirector);
    bool hasDirector();
    std::string getDirector();

    void onProjectNameReceived(std::string inProjectName);
    bool hasProjectName();
    std::string getProjectName();

    void onSlateTypeReceived(CCUPacketTypes::MetadataSlateForType inSlateForType);
    bool hasSlateType();
    CCUPacketTypes::MetadataSlateForType getSlateType();

    void onSlateNameReceived(std::string inSlateName);
    bool hasSlateName();
    std::string getSlateName();

    void onLensFocalLengthReceived(std::string inLensFocalLength);
    bool hasLensFocalLength();
    std::string getLensFocalLength();

    void onLensDistanceReceived(std::string inLensDistance);
    bool hasLensDistance();
    std::string getLensDistance();

    void onLensTypeReceived(std::string inLensType);
    bool hasLensType();
    std::string getLensType();

    void onLensIrisReceived(std::string inLensIris);
    bool hasLensIris();
    std::string getLensIris();


    // Structures for vectors/arrays
    struct MediaSlot {
        bool active = false;
        CCUPacketTypes::ActiveStorageMedium medium = CCUPacketTypes::ActiveStorageMedium::CFastCard;
        CCUPacketTypes::MediaStatus status;
        ccu_fixed_t remainingRecordTimeMinutes;
        std::string remainingRecordTimeString;
    };

private:
    bool connected = false;

    // Attributes are shared pointers so we can tell if they have been set or not (e.g. the pointer is null means it hasn't been set yet.)

    // Video Attributes
    std::shared_ptr<int> sensorGainISO; // Gain as ISO
    std::shared_ptr<short> whiteBalance;
    std::shared_ptr<short> tint;
    std::shared_ptr<int32_t> shutterSpeedMS; // Shutter speed in us
    std::shared_ptr<CCUPacketTypes::RecordingFormatData> recordingFormat;
    std::shared_ptr<CCUPacketTypes::AutoExposureMode> autoExposureMode;
    std::shared_ptr<int32_t> shutterAngle;
    std::shared_ptr<int32_t> shutterSpeed; // Shutter speed as a fraction of 1
    std::shared_ptr<byte> sensorGainDB; // Gain in decibels
    std::shared_ptr<int32_t> sensorGainISOValue; // ISO Value
    std::shared_ptr<CCUPacketTypes::SelectedLUT> selectedLUT;
    std::shared_ptr<bool> selectedLUTEnabled;


    // Status Attributes
    CCUPacketTypes::BatteryStatusData batteryStatus;
    std::shared_ptr<std::string> modelName;
    std::shared_ptr<bool> isPocketCamera;
    std::vector<MediaSlot> mediaSlots;


    // Metadata Attributes
    std::shared_ptr<short> reelNumber;
    std::shared_ptr<std::string> sceneName;
    std::shared_ptr<CCUPacketTypes::MetadataSceneTag> sceneTag;
    std::shared_ptr<CCUPacketTypes::MetadataLocationTypeTag> locationType;
    std::shared_ptr<CCUPacketTypes::MetadataDayNightTag> dayOrNight;
    std::shared_ptr<CCUPacketTypes::MetadataTakeTag> takeTag;
    std::shared_ptr<sbyte> takeNumber;
    std::shared_ptr<sbyte> goodTake;
    std::shared_ptr<std::string> cameraId;
    std::shared_ptr<std::string> cameraOperator;
    std::shared_ptr<std::string> director;
    std::shared_ptr<std::string> projectName;
    std::shared_ptr<CCUPacketTypes::MetadataSlateForType> slateType;
    std::shared_ptr<std::string> slateName;
    std::shared_ptr<std::string> lensFocalLength;
    std::shared_ptr<std::string> lensDistance;
    std::shared_ptr<std::string> lensType;
    std::shared_ptr<std::string> lensIris;
};

#endif