#ifndef BMDCAMERA_H
#define BMDCAMERA_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include "Arduino_DebugUtils.h"
#include "Camera/BMDCamera.h"
#include "CCU/CCUPacketTypes.h"
#include "CCU/CCUPacketTypesString.h"
#include "Config/LensConfig.h"
#include "Camera/CodecInfo.h"
#include "Camera/TransportInfo.h"
#include "Camera/CameraModels.h"

class BMDCamera
{
public:
    BMDCamera();
    ~BMDCamera();

    void setAsConnected();
    void setAsDisconnected();

    // Structures for vectors/arrays
    struct MediaSlot {
        bool active = false;
        CCUPacketTypes::ActiveStorageMedium medium = CCUPacketTypes::ActiveStorageMedium::CFastCard;
        CCUPacketTypes::MediaStatus status;
        ccu_fixed_t remainingRecordTimeMinutes;
        std::string remainingRecordTimeString;

        std::string GetMediumString()
        {
            return CCUPacketTypesString::GetEnumString(medium);
        }

        std::string GetStatusString(bool toUpper = true)
        {
            switch(status)
            {
                case CCUPacketTypes::MediaStatus::None:
                    return toUpper ? "NONE" : "None";
                case CCUPacketTypes::MediaStatus::Ready:
                    return toUpper ? "READY" : "Ready";
                case CCUPacketTypes::MediaStatus::MountError:
                    return toUpper ? "MOUNT ERROR" : "Mount Error";
                case CCUPacketTypes::MediaStatus::RecordError:
                    return toUpper ? "RECORD ERROR" : "Record Error";
                default:
                    return "UNKNOWN STATUS";
            }   
        }

        // Is/has the media had an error?
        bool StatusIsError()
        {
            return status == CCUPacketTypes::MediaStatus::MountError || status == CCUPacketTypes::MediaStatus::RecordError;
        }
    };

    // Quick access attributes
    bool isRecording = 0;
    bool shutterValueIsAngle = true;

    // Quick access functions
    bool hasRecordError();
    bool isPocket4K6K();
    bool isPocket4K();
    bool isPocket6K();
    bool isURSAMiniProG2();
    bool isURSAMiniPro12K();

    // Lens Attributes

    void onHasLens(bool inHasLens);
    bool hasHasLens();
    bool getHasLens();

    void onApertureUnitsReceived(LensConfig::ApertureUnits inApertureUnits);
    bool hasApertureUnits();
    LensConfig::ApertureUnits getApertureUnits();
    std::string getApertureUnitsString();

    void onApertureFStopStringReceived(std::string inApertureFStopString);
    bool hasApertureFStopString();
    std::string getApertureFStopString();

    void onApertureNormalisedReceived(int inApertureNormalised);
    bool hasApertureNormalised();
    int getApertureNormalised();

    void onFocalLengthMMReceived(ccu_fixed_t inFocalLengthMM);
    bool hasFocalLengthMM();
    ccu_fixed_t getFocalLengthMM();

    void OnImageStabilisationReceived(bool inImageStabilisation);
    bool hasImageStabilisation();
    bool getImageStabilisation();

    void onAutoFocusPressed(); // When auto focus button is pressed

    const std::vector<BMDCamera::MediaSlot> getMediaSlots();
    std::string getSlotActiveStorageMediumString(int slotIndex);
    std::string getSlotMediumStatusString(int slotIndex);
    bool hasActiveMediaSlot();
    MediaSlot getActiveMediaSlot();


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

    void onRecordingFormatReceived(CCUPacketTypes::RecordingFormatData inRecordingFormat);
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
    void onBatteryReceived(CCUPacketTypes::BatteryStatusData inBattery);
    bool hasBattery();
    CCUPacketTypes::BatteryStatusData getBattery();

    void onModelNameReceived(std::string inModelName);
    bool hasModelName();
    std::string getModelName();

    void onIsPocketReceived(bool inIsPocket);
    bool hasIsPocket();
    bool getIsPocket();

    void onMediaStatusReceived(std::vector<CCUPacketTypes::MediaStatus> inMediaStatuses);
    void onRemainingRecordTimeMinsReceived(std::vector<ccu_fixed_t> inRecordTimeMins);
    void onRemainingRecordTimeStringReceived(std::vector<std::string> inRecordTimeStrings);


    // Media Attributes

    void onCodecReceived(CodecInfo inCodec);
    bool hasCodec();
    CodecInfo getCodec();

    void onTransportModeReceived(TransportInfo inTransportMode);
    bool hasTransportMode();
    TransportInfo getTransportMode();


    // Metadata Attributes

    void onReelNumberReceived(short inReelNumber, bool inIsEditable);
    bool hasReelNumber();
    short getReelNumber();
    bool hasReelEditable();
    bool getReelEditable();

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

    // Display Attributes

    void onTimecodeSourceReceived(CCUPacketTypes::DisplayTimecodeSource inTimecodeSource);
    bool hasTimecodeSource();
    CCUPacketTypes::DisplayTimecodeSource getTimecodeSource();

    void onTimecodeReceived(std::string inTimecode);
    std::string getTimecodeString();

    // Last Modified
    unsigned long getLastModified() const { return lastUpdated; }
    void setLastModified() { modified(); }

    // Last known BRAW Bitrate, BRAW Quality, and ProRes settings (for when we switch between options we know what to change it to)
    // Default options for now, until we get settings from the camera coming through
    bool lastKnownBRAWIsBitrate = false;
    CodecInfo lastKnownBRAWBitrate = CodecInfo(CCUPacketTypes::BasicCodec::BRAW, static_cast<byte>(CCUPacketTypes::CodecVariants::kBRAW5_1));
    CodecInfo lastKnownBRAWQuality = CodecInfo(CCUPacketTypes::BasicCodec::BRAW, static_cast<byte>(CCUPacketTypes::CodecVariants::kBRAWQ5));
    CodecInfo lastKnownProRes = CodecInfo(CCUPacketTypes::BasicCodec::ProRes, static_cast<byte>(CCUPacketTypes::CodecVariants::kProResHQ));


private:
    bool connected = false;
    int activeMediaSlotIndex = -1; // The index of the active media slot, used for quick access to info on it.
    unsigned long lastUpdated = millis(); // Keeps track of when it was last changed

    // Last Modified
    void modified() { lastUpdated = millis(); }

    // Custom Attributes
    std::vector<MediaSlot> mediaSlots;

    // Attributes are shared pointers so we can tell if they have been set or not (e.g. the pointer is null means it hasn't been set yet.)

    // Lens Attributes
    std::shared_ptr<bool> hasLens;
    std::shared_ptr<LensConfig::ApertureUnits> apertureUnits;
    std::shared_ptr<std::string> aperturefStopString;
    std::shared_ptr<int> apertureNormalised;
    std::shared_ptr<ccu_fixed_t> focalLengthMM;
    std::shared_ptr<bool> imageStabilisation;


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
    std::shared_ptr<CCUPacketTypes::BatteryStatusData> batteryStatus;
    std::shared_ptr<std::string> modelName;
    std::shared_ptr<bool> isPocketCamera;

    
    // Media Attributes
    std::shared_ptr<CodecInfo> codec;
    std::shared_ptr<TransportInfo> transportMode;

    // Metadata Attributes
    std::shared_ptr<short> reelNumber;
    std::shared_ptr<bool> reelEditable;
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

    // Display Attributes
    std::shared_ptr<CCUPacketTypes::DisplayTimecodeSource> timecodeSource;
    std::shared_ptr<std::string> timecode;
};

#endif