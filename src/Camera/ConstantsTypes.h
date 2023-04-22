#ifndef CONSTANTSTYPES_H
#define CONSTANTSTYPES_H

#include <string>

// Data types
typedef int16_t ccu_fixed_t; // System.Int16;
typedef uint8_t byte;
typedef int8_t sbyte;
typedef unsigned short ushort;

class Constants
{
    public:
        static const std::string UUID_BMD_BCS;
        static const std::string UUID_BMD_BCS_OUTGOING_CAMERA_CONTROL;
        static const std::string UUID_BMD_BCS_INCOMING_CAMERA_CONTROL;
        static const std::string UUID_BMD_BCS_TIMECODE;
        static const std::string UUID_BMD_BCS_DEVICE_NAME;
        static const std::string UUID_BMD_BCS_PROTOCOL_VERSION;
        static const std::string UUID_BMD_BCS_CAMERA_STATUS;

        static const std::string UnknownEnumValue;

        // Colour RGB565 calculator: http://www.rinkydinkelectronics.com/calc_rgb565.php
        const static int DARK_RED = 0x8800; /* 140, 0, 0 */
};

#endif