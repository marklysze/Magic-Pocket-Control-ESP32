#include "ConstantsTypes.h"

// Blackmagic Bluetooth constants
const std::string Constants::UUID_BMD_BCS = "291D567A-6D75-11E6-8B77-86F30CA893D3";
const std::string Constants::UUID_BMD_BCS_OUTGOING_CAMERA_CONTROL = "5DD3465F-1AEE-4299-8493-D2ECA2F8E1BB";
const std::string Constants::UUID_BMD_BCS_INCOMING_CAMERA_CONTROL = "B864E140-76A0-416A-BF30-5876504537D9";
const std::string Constants::UUID_BMD_BCS_TIMECODE = "6D8F2110-86F1-41BF-9AFB-451D87E976C8";
const std::string Constants::UUID_BMD_BCS_DEVICE_NAME = "FFAC0C52-C9FB-41A0-B063-CC76282EB89C";
const std::string Constants::UUID_BMD_BCS_PROTOCOL_VERSION = "8F1FD018-B508-456F-8F82-3D392BEE2706";
const std::string Constants::UUID_BMD_BCS_CAMERA_STATUS = "7FE8691D-95DC-4FC5-8ABD-CA74339B51B9";

// When getting enum value as string, if there's no matching value, return this string
const std::string Constants::UnknownEnumValue = "[Unknown]";