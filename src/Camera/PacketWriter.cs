using BMDCameraMVVM.Core.CCU;
using BMDCameraMVVM.Core.Models;
using BMDCore.Camera;
using BMDCore.CCU;
using System;
using System.Collections.Generic;
using System.Text;

namespace BMDCameraMVVM.Core.Camera
{
    public class PacketWriter
    {
        public static void validateAndSendCCUCommand(CCUPacketTypes.Command command, ObservableBMDCamera observableBMDCamera)
        {
            bool packetIsValid = CCUValidationFunctions.ValidateCCUPacket(command.serialize());
            if(packetIsValid) {
                observableBMDCamera.onCCUPacketEncoded(command.serialize());
            }
            else
            {
                // command!.Log()
                // Logger.LogError("Trying to send invalid CCU Packet.")
            }
        }

        // Video commands
        public static void writeWhiteBalance(short whiteBalance, short tint, ObservableBMDCamera observableBMDCamera)
        {
            CCUPacketTypes.Command command = CCUEncodingFunctions.CreateVideoWhiteBalanceCommand(whiteBalance, tint);
            validateAndSendCCUCommand(command, observableBMDCamera);
        }

        public static void writeAutoWhiteBalance(ObservableBMDCamera observableBMDCamera)
        {
            CCUPacketTypes.Command command = CCUEncodingFunctions.CreateVideoSetAutoWBCommand();
            validateAndSendCCUCommand(command, observableBMDCamera);
        }


        public static void writeRecordingFormatStatus(ObservableBMDCamera observableBMDCamera)
        {
            CCUPacketTypes.Command command = CCUEncodingFunctions.CreateRecordingFormatStatusCommand();
            validateAndSendCCUCommand(command, observableBMDCamera);
        }

        public static void writeRecordingFormat(CCUPacketTypes.RecordingFormatData recordingFormatData, ObservableBMDCamera observableBMDCamera)
        {
            CCUPacketTypes.Command command = CCUEncodingFunctions.CreateRecordingFormatCommand(recordingFormatData);
            validateAndSendCCUCommand(command, observableBMDCamera);
        }

        public static void writeApertureNormalised(short normalisedApertureValue, ObservableBMDCamera observableBMDCamera)
        {
            CCUPacketTypes.Command command = CCUEncodingFunctions.CreateFixed16Command(normalisedApertureValue, CCUPacketTypes.Category.Lens, (byte)CCUPacketTypes.LensParameter.ApertureNormalised);
            validateAndSendCCUCommand(command, observableBMDCamera);
        }

        public static void writeIris(short apertureValue, ObservableBMDCamera observableBMDCamera)
        {
            CCUPacketTypes.Command command = CCUEncodingFunctions.CreateFixed16Command(apertureValue, CCUPacketTypes.Category.Lens, (byte)CCUPacketTypes.LensParameter.ApertureFstop);
            validateAndSendCCUCommand(command, observableBMDCamera);
        }

        public static void writeShutterSpeed(int shutter, ObservableBMDCamera observableBMDCamera)
        {
            CCUPacketTypes.Command command = CCUEncodingFunctions.CreateCommand(shutter, CCUPacketTypes.Category.Video, (byte)CCUPacketTypes.VideoParameter.ShutterSpeed);
            validateAndSendCCUCommand(command, observableBMDCamera);

        }

        public static void writeShutterAngle(int shutterAngleX100, ObservableBMDCamera observableBMDCamera)
        {
            CCUPacketTypes.Command command = CCUEncodingFunctions.CreateCommand(shutterAngleX100, CCUPacketTypes.Category.Video, (byte)CCUPacketTypes.VideoParameter.ShutterAngle);
            validateAndSendCCUCommand(command, observableBMDCamera);

        }

        public static void writeSensorGain(int sensorGain, ObservableBMDCamera observableBMDCamera)
        {
            byte sensorGainValue = (byte)(sensorGain / BMDCore.Config.VideoConfig.kSentSensorGainBase);
            CCUPacketTypes.Command command = CCUEncodingFunctions.CreateVideoSensorGainCommand(sensorGainValue);
            validateAndSendCCUCommand(command, observableBMDCamera);
        }

        // MS
        public static void writeISO(int iso, ObservableBMDCamera observableBMDCamera)
        {
            CCUPacketTypes.Command command = CCUEncodingFunctions.CreateVideoISOCommand(iso);
            validateAndSendCCUCommand(command, observableBMDCamera);
        }

        // Transport commands
        public static void writeTransportPacket(TransportInfo transportInfo, ObservableBMDCamera observableBMDCamera)
        {
            CCUPacketTypes.Command command = CCUEncodingFunctions.CreateTransportInfoCommand(transportInfo);
            validateAndSendCCUCommand(command, observableBMDCamera);
        }


        /*
        weak var m_packetEncodedDelegate: PacketEncodedDelegate?


        // Slate commands
        func writeReel(_ reel: Int16) {
            let command = CCUEncodingFunctions.CreateCommand(reel, CCUPacketTypes.Category.Metadata, CCUPacketTypes.MetadataParameter.Reel.rawValue)
            validateAndSendCCUCommand(command)
        }

        func writeSceneTags(_ sceneTag: CCUPacketTypes.MetadataSceneTag, _ locationTag: CCUPacketTypes.MetadataLocationTypeTag, _ timeTag: CCUPacketTypes.MetadataDayNightTag) {
            let command = CCUEncodingFunctions.CreateMetadataSceneTagsCommand(sceneTag, locationTag, timeTag)
            validateAndSendCCUCommand(command)
        }

        func writeScene(_ scene: String) {
            let command = CCUEncodingFunctions.CreateStringCommand(scene, CCUPacketTypes.Category.Metadata, CCUPacketTypes.MetadataParameter.Scene.rawValue)
            validateAndSendCCUCommand(command)
        }

        func writeTake(_ takeNumber: Int, _ takeTag: CCUPacketTypes.MetadataTakeTag) {
            let command = CCUEncodingFunctions.CreateMetadataTakeCommand(UInt8(takeNumber), takeTag)
            validateAndSendCCUCommand(command)
        }

        func writeGoodTake(_ goodTake: Bool) {
            let command = CCUEncodingFunctions.CreateCommand(goodTake, CCUPacketTypes.Category.Metadata, CCUPacketTypes.MetadataParameter.GoodTake.rawValue)
            validateAndSendCCUCommand(command)
        }

        func writePower(_ turnPowerOn: Bool) {
            m_packetEncodedDelegate?.onPowerPacketEncoded(turnPowerOn ? CameraStatus.kPowerOn : CameraStatus.kPowerOff)
        }

        // Location commands
        func writeLocation(_ latitude: UInt64, _ longitude: UInt64) {
            let dataArray: [Int64] = [Int64(bitPattern: latitude), Int64(bitPattern: longitude)]
            let command = CCUEncodingFunctions.CreateCommandFromArray(dataArray, CCUPacketTypes.Category.Configuration, CCUPacketTypes.ConfigurationParameter.Location.rawValue)
            validateAndSendCCUCommand(command)
        }
         */
    }
}
