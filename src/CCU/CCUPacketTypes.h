#ifndef CCUPACKETTYPES_H
#define CCUPACKETTYPES_H

#include <stdint.h>
#include <Arduino.h>
#include "Camera\ConstantsTypes.h"

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

        ccu_fixed_t CCUFixedFromFloat(double f);
        double CCUFloatFromFixed(ccu_fixed_t f);
        double CCUFloatFromFixed(uint16_t f);
        int32_t CCUPercentFromFixed(ccu_fixed_t f);

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
            ChangeConfiguration = 0,
            MSTest_GetStatus = 1 // Absolute guess on this one, attempting to see if we can just request information rather than make a change
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
            FocusAssist = 5
        };
        const static byte DisplayParameterValues[6]; // Added to support checking a value is valid within the enum

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
            DisplayParameters = 7
        };
        const static byte StatusParameterValues[8]; // Added to support checking a value is valid within the enum

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
            SSD = 2,
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
            byte* data;

            Command(byte target, CommandID commandID, Category category, byte parameter, OperationType operationType, byte dataType, byte* data)
            {
                byte commandLength = (byte)(CCUPacketTypes::kCCUCommandHeaderSize + sizeof(data));
                int packetSize = commandLength + CCUPacketTypes::kCCUPacketHeaderSize;
                if(packetSize > CCUPacketTypes::kPacketSizeMax)
                {
                    Serial.print("Packet size (");
                    Serial.print(packetSize);
                    Serial.print(") exceeds kPacketSizeMax (");
                    Serial.print(CCUPacketTypes::kPacketSizeMax);
                    Serial.println("). Aborting InitCommand.");
                    // throw "Packet size (" + packetSize + ") exceeds kPacketSizeMax (" + CCUPacketTypes::kPacketSizeMax + "). Aborting InitCommand.";
                }

                this->target = target;
                this->length = commandLength;
                this->commandID = commandID;
                this->reserved = 0;
                this->category = category;
                this->parameter = parameter;
                this->operationType = operationType;
                this->dataType = dataType;
                this->data = data;
            }

            // WARNING - make sure to use "new" and delete the buffer when not needed: delete[] buffer;
            /* LIKE THIS:
                byte* serializedData = obj.serialize();
                // use serializedData
                delete[] serializedData;
            */
            byte* serialize()
            {
                byte headersSize = (byte)(CCUPacketTypes::kCCUPacketHeaderSize + CCUPacketTypes::kCCUCommandHeaderSize);
                byte payloadSize = sizeof(data);
                byte padBytes = (byte)(((length + 3) & ~3) - length);
                byte* buffer = new byte[headersSize + payloadSize + padBytes];
                memset(buffer, 0, headersSize + payloadSize + padBytes);

                buffer[PacketFormatIndex::Destination] = target;
                buffer[PacketFormatIndex::CommandLength] = length;
                buffer[PacketFormatIndex::CommandId] = ((byte)commandID);
                buffer[PacketFormatIndex::Source] = reserved;

                buffer[PacketFormatIndex::Category] = (byte)category;
                buffer[PacketFormatIndex::Parameter] = parameter;
                buffer[PacketFormatIndex::DataType] = dataType;
                buffer[PacketFormatIndex::OperationType] = (byte)operationType;

                for(int payloadIndex = 0; payloadIndex < sizeof(data); payloadIndex++)
                {
                    int packetIndex = PacketFormatIndex::PayloadStart + payloadIndex;
                    buffer[packetIndex] = data[payloadIndex];
                }

                return buffer;
            }
        };
};

#endif