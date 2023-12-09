#ifndef CCUPACKETTYPES_H
#define CCUPACKETTYPES_H

#include <stdint.h>
#include <Arduino.h>
#include <vector>
#include "Arduino_DebugUtils.h"
#include "Camera/ConstantsTypes.h"

// Packet Format Index
struct PacketFormatIndex {
    static int Destination;
    static int CommandLength;
    static int CommandId;
    static int Source;
    static int Category;
    static int Parameter;
    static int DataType;
    static int OperationType;
    static int PayloadStart;
};

class CCUPacketTypes
{
    public:
        static const byte kPacketSizeMin = 8;
        static const byte kPacketSizeMax = 64;
        static const byte kCCUPacketHeaderSize = 4;
        static const byte kCCUCommandHeaderSize = 4;
        static const byte kCUUPayloadOffset = 8;

        // Camera ID that is specified to broadcast a command to all cameras
        static const byte kBroadcastTarget = 255;

        static const int16_t kLensAperture_NoLens = -32768;

        static ccu_fixed_t CCUFixedFromFloat(double f);
        double CCUFloatFromFixed(ccu_fixed_t f);
        double CCUFloatFromFixed(uint16_t f);
        int32_t CCUPercentFromFixed(ccu_fixed_t f);
        static int16_t toFixed16(float value);

        enum class DataTypes : byte {
            kVoid = 0,
            kBool = 0,
            kInt8 = 1,
            kInt16 = 2,
            kInt32 = 3,
            kInt64 = 4,
            kString = 5,
            kFixed16 = 128
        };

        enum class OperationType : byte
        {
            AssignValue = 0,
            OffsetValue = 1,
            StatusUpdate = 2
        };

        enum class CommandID : byte
        {
            ChangeConfiguration = 0
        };

        // Command Category
        enum class Category : byte
        {
            Lens = 0,
            Video = 1,
            Audio = 2,
            Output = 3,
            Display = 4,
            Tally = 5,
            Reference = 6,
            Configuration = 7,
            ColorCorrection = 8,
            Status = 9,
            Media = 10,
            ExternalDeviceControl = 11,
            Metadata = 12
        };

        // *** Parameters for each Command Category *** //
        enum class LensParameter : byte
        {
            Focus = 0,
            AutoFocus = 1,
            ApertureFstop = 2,
            ApertureNormalised = 3,
            ApertureOrdinal = 4,
            AutoAperture = 5,
            ImageStabilisation = 6,
            Zoom = 7,
            ZoomNormalised = 8,
            ContinuousZoom = 9
        };
        const static byte LensParameterValues[10]; // Added to support checking a value is valid within the enum

        enum class VideoParameter : byte
        {
            Mode = 0,
            SensorGain = 1,
            ManualWB = 2,
            SetAutoWB = 3,
            RestoreAutoWB = 4,
            Exposure = 5,
            ExposureOrdinal = 6,
            DynamicRange = 7,
            SharpeningLevel = 8,
            RecordingFormat = 9,
            AutoExposureMode = 10,
            ShutterAngle = 11,
            ShutterSpeed = 12,
            Gain = 13,
            ISO = 14,
            DisplayLUT = 15 // NEW
        };
        const static byte VideoParameterValues[16]; // Added to support checking a value is valid within the enum

        enum class AudioParameter : byte
        {
            MicLevel = 0,
            HeadphoneLevel = 1,
            HeadphoneProgramMix = 2,
            SpeakerLevel = 3,
            InputType = 4,
            InputLevels = 5,
            PhantomPower = 6
        };
        const static byte AudioParameterValues[7]; // Added to support checking a value is valid within the enum

        enum class OutputParameter : byte
        {
            OverlayEnables = 0,
            FrameGuideStyle = 1, // legacy Cameras 3.x packet was never de-embedded by camera
            FrameGuideOpacity = 2, // legacy Cameras 3.x packet was never de-embedded by camera
            Overlays = 3
        };
        const static byte OutputParameterValues[4]; // Added to support checking a value is valid within the enum

        enum class DisplayParameter : byte
        {
            Brightness = 0,
            Overlays = 1,
            ZebraLevel = 2,
            PeakingLevel = 3,
            ColourBars = 4,
            FocusAssist = 5,
            TimecodeSource = 7
        };
        const static byte DisplayParameterValues[7]; // Added to support checking a value is valid within the enum

        enum class TallyParameter : byte
        {
            Brightness = 0,
            FrontBrightness = 1,
            RearBrightness = 2
        };
        const static byte TallyParameterValues[3]; // Added to support checking a value is valid within the enum

        enum class ReferenceParameter : byte
        {
            Source = 0,
            Offset = 1
        };
        const static byte ReferenceParameterValues[2]; // Added to support checking a value is valid within the enum

        enum class ConfigurationParameter : byte
        {
            SystemClock = 0,
            Language = 1,
            Timezone = 2,
            Location = 3
        };
        const static byte ConfigurationParameterValues[4]; // Added to support checking a value is valid within the enum

        enum class ColorCorrectionParameter : byte
        {
            LiftAdjust = 0,
            GammaAdjust = 1,
            GainAdjust = 2,
            OffsetAdjust = 3,
            ContrastAdjust = 4,
            LumaContribution = 5,
            ColourAdjust = 6,
            ResetDefault = 7
        };
        const static byte ColorCorrectionParameterValues[8]; // Added to support checking a value is valid within the enum

        enum class StatusParameter : byte
        {
            Battery = 0,
            MediaStatus = 1,
            RemainingRecordTime = 2,
            DisplayThresholds = 3,
            DisplayTimecode = 4,
            CameraSpec = 5,
            SwitcherStatus = 6,
            DisplayParameters = 7,
            NewParameterTBD = 8 // 2023-04-22 MS Camera Firmware 8.1 is sending this through on resolution changes (perhaps on other changes too)
        };
        const static byte StatusParameterValues[9]; // Added to support checking a value is valid within the enum

        enum class MediaParameter : byte
        {
            Codec = 0,
            TransportMode = 1
        };
        const static byte MediaParameterValues[2]; // Added to support checking a value is valid within the enum

        enum class ExDevControlParameter : byte
        {
            PanTiltVelocity = 0
        };
        const static byte ExDevControlParameterValues[1]; // Added to support checking a value is valid within the enum

        enum class MetadataParameter : byte
        {
            Reel = 0,
            SceneTags = 1,
            Scene = 2,
            Take = 3,
            GoodTake = 4,
            CameraId = 5,
            CameraOperator = 6,
            Director = 7,
            ProjectName = 8,
            LensType = 9,
            LensIris = 10,
            LensFocalLength = 11,
            LensDistance = 12,
            LensFilter = 13,
            SlateForType = 14,
            SlateForName = 15
        };
        const static byte MetadataParameterValues[16]; // Added to support checking a value is valid within the enum

        // *** Data Structure and Enums, orgranised by Command Category: Parameter *** //
        // Video: Video Mode: MRate
        enum class VideoModeMRate : byte
        {
            Regular = 0,
            MRate = 1
        };

        // Video: Video Mode: Dimensions
        enum class VideoModeDimensions : byte
        {
            NTSC = 0,
            PAL = 1,
            HD720 = 2,
            HD1080 = 3,
            _2K = 4,
            _2KDCI = 5,
            UHD4K = 6
        };

        // Video: Video Mode: Interlaced
        enum class VideoModeInterlaced : byte
        {
            Progressive = 0,
            Interlaced = 1
        };

        // Video: Video Mode: Colorspace
        enum class VideoModeColorspace : byte
        {
            YUV = 0
        };

        // Video: Dynamic Range
        enum class VideoDynamicRange : byte
        {
            Film = 0,
            Video = 1
        };

        // Video: Recording Format: Flags
        // public typealias VideoRecordingFormatFlags = UInt16
        enum class VideoRecordingFormat : ushort
        {
            FileMRate = 0x01,
            SensorMRate = 0x02,
            SensorOffSpeed = 0x04,
            Interlaced = 0x08,
            WindowedMode = 0x10
        };

        // Video: Auto Exposure Mode
        enum class AutoExposureMode : sbyte
        {
            Manual = 0,
            Iris = 1,
            Shutter = 2,
            IrisAndShutter = 3,
            ShutterAndIris = 4
        };

        // Video: Selected LUT (New)
        enum class SelectedLUT : byte
        {
            None = 0,
            Custom = 1,
            FilmToVideo = 2,
            FilmToExtendedVideo = 3
        };

        // Display: Timecode Source
        enum class DisplayTimecodeSource : byte
        {
            Clip = 0,
            Timecode = 1
        };

        // Status: Battery: Flags
        // typealias StatusBatteryFlags = UInt16
        enum class BatteryStatus : ushort
        {
            BatteryPresent = 0x01,
            ACPresent = 0x02,
            BatteryIsCharging = 0x04,
            ChargeRemainingPercentageIsEstimated = 0x08,
            PreferVoltageDisplay = 0x10
        };

        // Status: Media
        enum class MediaStatus : sbyte
        {
            None = 0,
            Ready = 1,
            MountError = -1,
            RecordError = -2
        };

        // Status: Video display settings
        enum class DisplayFocusAssistMode : int
        {
            ColouredLines = 0,
            Peak = 1
        };

        enum class DisplayFocusAssistColour : int
        {
            Red = 0,
            Green = 1,
            Blue = 2,
            White = 3,
            Black = 4
        };

        // Status: Camera Specifications: Manufacturer
        enum class CameraManufacturer : byte
        {
            Blackmagic = 0
        };

        // Status: Camera Specifications: Model
        enum class CameraModel : byte
        {
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
            URSAMiniPro = 10
        };

        // Status: Camera Specifications: Camera Variant
        // public typealias CameraVariant = UInt8
        struct CameraVariants
        {
            static const byte kDefault = 0;
            static const byte kURSAMini_46K = 0;
            static const byte kURSAMini_4K = 0;
            static const byte kURSA_4K_V1 = 0;
            static const byte kURSA_4K_V2 = 1;
            static const byte kURSA_46K = 2;
        };

        // Status: Camera Specifications: Color Science Generation
        enum class ColorScienceGeneration : byte
        {
            Unknown = 0,
            Generation3 = 3
        };

        // Output: Overlay Enables: Frame Guide Style
        enum class FrameGuideStyle : byte
        {
            Off = 0,
            _240_1 = 1,
            _239_1 = 2,
            _235_1 = 3,
            _185_1 = 4,
            _16_9 = 5,
            _14_9 = 6,
            _4_3 = 7
        };

        // Output: Overlay Enables: Grid Style
        // public typealias GridStyleFlags = UInt8
        enum class GridStyleFlag : byte
        {
            Thirds = 0x01,
            CrossHairs = 0x02,
            CenterDot = 0x04
        };

        // Status: Switcher Status: Flags
        // public typealias SwitcherStatusFlags = UInt8
        enum class SwitcherStatusFlag : byte
        {
            SwitcherConnected = 0x01,
            PreviewTally = 0x02,
            ProgramTally = 0x04
        };

        // Media: Codec: Basic Codec
        // MS: BRAW?
        enum class BasicCodec : byte
        {
            RAW = 0,
            DNxHD = 1,
            ProRes = 2,
            BRAW = 3
        };

        // Media: Codec: Codec Variant
        // public typealias CodecVariant = UInt8
        struct CodecVariants
        {
            static const byte kDefault = 0;

            // CinemaDNG
            static const byte kLosslessRaw = 0;
            static const byte kRaw3_1 = 1;
            static const byte kRaw4_1 = 2;

            // ProRes
            static const byte kProResHQ = 0;
            static const byte kProRes422 = 1;
            static const byte kProResLT = 2;
            static const byte kProResProxy = 3;
            static const byte kProRes444 = 4;
            static const byte kProRes444XQ = 5;

            // BRAW
            static const byte kBRAWQ0 = 0;
            static const byte kBRAWQ5 = 1;
            static const byte kBRAW3_1 = 2;
            static const byte kBRAW5_1 = 3;
            static const byte kBRAW8_1 = 4;
            static const byte kBRAW12_1 = 5;
            static const byte kBRAW18_1 = 6;
            static const byte kBRAWQ1 = 7;
            static const byte kBRAWQ3 = 8;

        };

        // Media: Transport Mode: Mode
        enum class MediaTransportMode : byte
        {
            Preview = 0,
            Play = 1,
            Record = 2
        };

        // Media: Transport Mode: Flags
        // public typealias MediaTransportFlags = UInt8
        enum class MediaTransportFlag : byte
        {
            Loop = 0x01,
            PlayAll = 0x02,
            Disk1Active = 0x20,
            Disk2Active = 0x40,
            Disk3Active = 0x10,
            TimelapseRecording = 0x80
        };

        static const std::array<int, 4> slotActiveMasks;

        enum class ActiveStorageMedium : byte
        {
            CFastCard = 0,
            SDCard = 1,
            SSDRecorder = 2,
            USB = 3
        };

        enum class MetadataSceneTag : sbyte
        {
            None = -1,
            WS = 0,
            CU = 1,
            MS = 2,
            BCU = 3,
            MCU = 4,
            ECU = 5
        };

        enum class MetadataLocationTypeTag : byte
        {
            Exterior = 0,
            Interior = 1
        };

        enum class MetadataDayNightTag : byte
        {
            Night = 0,
            Day = 1
        };

        enum class MetadataTakeTag : sbyte
        {
            None = -1,
            PU = 0,
            VFX = 1,
            SER = 2
        };

        enum class MetadataSlateForType : sbyte
        {
            NextClip = 0,
            PlaybackFile = 1
        };

        struct MetadataMaxStringLength
        {
            static const sbyte kScene = 5;
            static const sbyte kCameraId = 1;
            static const sbyte kCameraOperator = 29;
            static const sbyte kDirector = 29;
            static const sbyte kProjectName = 29;
            static const sbyte kLensType = 56;
            static const sbyte kLensIris = 20;
            static const sbyte kLensFocalLength = 30;
            static const sbyte kLensDistance = 50;
            static const sbyte kLensFilter = 30;
        };

        struct RecordingFormatData
        {
            short frameRate;
            short offSpeedFrameRate;
            short width;
            short height;
            bool mRateEnabled;
            bool offSpeedEnabled;
            bool interlacedEnabled;
            bool windowedModeEnabled;
            bool sensorMRateEnabled;

            std::string frameRate_string()
            {
                String nonOffspeed = std::to_string(frameRate).c_str();

                if(mRateEnabled && frameRate == 24) // && !offSpeedEnabled)
                    nonOffspeed = "23.98";
                else if(mRateEnabled && frameRate == 30) // && !offSpeedEnabled)
                    nonOffspeed = "29.97";
                else if(mRateEnabled && frameRate == 60) // && !offSpeedEnabled)
                    nonOffspeed = "59.94";
                
                if(!offSpeedEnabled)
                    return nonOffspeed.c_str();
                else
                {
                    String combinedSpeed = std::to_string(offSpeedFrameRate).c_str();
                    combinedSpeed.concat("/");
                    combinedSpeed.concat(nonOffspeed);
                    return combinedSpeed.c_str();
                }
            }

            std::string frameWidthHeight_string()
            {
                return std::to_string(width) + " x " + std::to_string(height);
            }

            std::string frameDimensionsShort_string()
            {
                // Pocket 4K
                if(width == 1920 && height == 1080)
                    return "HD";
                else if(width == 2688 && height == 1512)
                    return "2.6K 16:9";
                else if(width == 2880 && height == 2160)
                    return "2.8K Ana";
                else if(width == 3840 && height == 2160)
                    return "4K UHD";
                else if(width == 4096 && height == 1720)
                    return "4K 2.4:1";
                else if(width == 4096 && height == 2160)
                    return "4K DCI";

                // Pocket 6K / Pro / G2 additional
                else if(width == 2868 && height == 1512)
                    return "2.8K 17:9";
                else if(width == 2880 && height == 1512) // Version 8.1 has width at 2880
                    return "2.8K 17:9";
                else if(width == 3728 && height == 3104)
                    return "3.7K 6:5A";
                else if(width == 5744 && height == 3024)
                    return "5.7K 17:9";
                else if(width == 6144 && height == 2560)
                    return "6K 2.4:1";
                else if(width == 6144 && height == 3456)
                    return "6K";
                
                // Ursa 4.6K G2
                else if(width == 2048 && height == 1080)
                    return "2K DCI";
                else if(width == 2048 && height == 1152)
                    return "2K 16:9";
                else if(width == 3072 && height == 2560)
                    return "3K Ana";
                else if(width == 4096 && height == 2304)
                    return "4K 16:9";
                else if(width == 4608 && height == 2592)
                    return "4.6K";
                else if(width == 4608 && height == 1920)
                    return "4.6K 2.4:1";

                // Ursa 12K
                else if(width == 2560 && height == 2136)
                    return "4K Ana";
                else if(width == 4096 && height == 1704)
                    return "4K 2.4:1";
                else if(width == 6144 && height == 3240)
                    return "6K S16";
                else if(width == 5120 && height == 4272)
                    return "8K Ana";
                else if(width == 8192 && height == 3408)
                    return "8K 2.4:1";
                else if(width == 7680 && height == 4320)
                    return "8K 16:9";
                else if(width == 8192 && height == 4320)
                    return "8K DCI";
                else if(width == 7680 && height == 6408)
                    return "12K Ana";
                else if(width == 12288 && height == 5112)
                    return "12K 2.4:1";
                else if(width == 11520 && height == 6480)
                    return "12K 16:9";
                else if(width == 12288 && height == 6480)
                    return "12K DCI";

                else // We don't know the short name so output the full dimensions
                    return (String(width) + String(" x ") + String(height)).c_str();
            }
        };

        struct BatteryStatusData
        {
            short batteryLevelX1000;
            bool batteryPresent;
            bool ACPresent;
            bool batteryIsCharging;
            bool chargeRemainingPercentageIsEstimated;
            bool preferVoltageDisplay;
        };

        // *** Command Structure *** //
        struct Command
        {
            // Command Header:
            byte target; // Destination device
            byte length; // Length of this command (excluding the 4-byte Command Header)
            CommandID commandID;
            byte reserved;

            // Command Data:
            Category category;
            byte parameter;
            byte dataType;
            OperationType operationType;
            std::vector<byte> data; // Changed from pointer to vector for simpler memory management

            std::vector<byte> serialisedCommand;

            Command(byte target, CommandID commandID, Category category, byte parameter, OperationType operationType, byte dataType, std::vector<byte> inData)
            {
                byte commandLength = (byte)(CCUPacketTypes::kCCUCommandHeaderSize + inData.size());
                int packetSize = commandLength + CCUPacketTypes::kCCUPacketHeaderSize;
                if(packetSize > CCUPacketTypes::kPacketSizeMax)
                {
                    DEBUG_ERROR("Packet size (%i) exceeds kPacketSizeMax (%i). Aborting InitCommand.", packetSize, CCUPacketTypes::kPacketSizeMax);
                    throw std::runtime_error("Packet size exceeds kPacketSizeMax. Aborting InitCommand.");
                }

                this->target = target;
                this->length = commandLength;
                this->commandID = commandID;
                this->reserved = 0;
                this->category = category;
                this->parameter = parameter;
                this->operationType = operationType;
                this->dataType = dataType;
                this->data = inData;

                serialisedCommand = convertToVector();
            }

            std::vector<byte> convertToVector()
            {
                const byte headersSize = static_cast<byte>(CCUPacketTypes::kCCUPacketHeaderSize + CCUPacketTypes::kCCUCommandHeaderSize);
                const byte payloadSize = data.size();
                const byte padBytes = static_cast<byte>(((length + 3) & ~3) - length);
                std::vector<byte> buffer(headersSize + payloadSize + padBytes, 0);

                // DEBUG_DEBUG("Command Serialize, Header Size is %i, Payload Size is %i, Pad Bytes is %i.", headersSize, payloadSize, padBytes);

                buffer[PacketFormatIndex::Destination] = target;
                buffer[PacketFormatIndex::CommandLength] = length;
                buffer[PacketFormatIndex::CommandId] = static_cast<byte>(commandID);
                buffer[PacketFormatIndex::Source] = reserved;

                buffer[PacketFormatIndex::Category] = static_cast<byte>(category);
                buffer[PacketFormatIndex::Parameter] = parameter;
                buffer[PacketFormatIndex::DataType] = dataType;
                buffer[PacketFormatIndex::OperationType] = static_cast<byte>(operationType);

                for(int payloadIndex = 0; payloadIndex < data.size(); payloadIndex++)
                {
                    int packetIndex = PacketFormatIndex::PayloadStart + payloadIndex;
                    buffer[packetIndex] = data[payloadIndex];
                }

                // When sending commands, debug.
                /*
                if(buffer[PacketFormatIndex::OperationType] == static_cast<int>(CCUPacketTypes::OperationType::AssignValue))
                {
                    DEBUG_VERBOSE("Command Serialize - Assign Value: ");
                    for(int index = 0; index < buffer.size(); index++)
                    {
                        DEBUG_VERBOSE("%i: %i", index, buffer[index]);
                        
                    }
                }
                */
               
                return buffer;
            }

            std::vector<byte> serialize()
            {
                return serialisedCommand;
            }
        };
};

#endif