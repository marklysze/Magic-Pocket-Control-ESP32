#ifndef CCUUTILITY_H
#define CCUUTILITY_H

#include <Arduino.h>
#include <stdint.h>
#include "Camera/ConstantsTypes.h"

class CCUUtility
{
public:
    const int kCCUCommandHeaderSize = 4;
    const int kCUUPayloadOffset = 8;

    static byte* StringToByteArray(String hex) {
        int NumberChars = hex.length();
        byte* bytes = new byte[NumberChars / 2];
        for (int i = 0; i < NumberChars; i += 2)
            bytes[i / 2] = strtol(hex.substring(i, 2).c_str(), NULL, 16);
        return bytes;
    }

    static bool byteValueExistsInArray(const byte arr[], int len, byte value) {
        for (int i = 0; i < len; i++) {
            if (arr[i] == value) {
                return true;
            }
        }
        return false;
    }

    // Note that the function returns a pointer to the new byte array, so you'll need to make sure to free the memory once you're done using it.
    /*
    template<typename T>
    static byte* ToByteArrayFromArray(T* value, size_t length) {
        byte* byteArray = reinterpret_cast<byte*>(value);
        byte* result = new byte[length * sizeof(T)];
        memcpy(result, byteArray, length * sizeof(T));
        return result;
    }
    */
   
    // This implementation returns a std::vector<byte> rather than a pointer so memory management is simpler
    template<typename T>
    static std::vector<byte> ToByteArrayFromArray(T* value, size_t length) {
        std::vector<byte> result(length * sizeof(T));
        byte* byteArray = reinterpret_cast<byte*>(value);
        memcpy(result.data(), byteArray, length * sizeof(T));
        return result;
    }

    // This function creates a new byte array of size sizeof(T), copies the bytes of the input value to the
    // byte array using the memcpy function, and returns the byte array. Note that you will need to
    // delete[] the returned byte array after use to avoid memory leaks.
    /*
    template<typename T>
    static byte* ToByteArray(T value)
    {
        byte* byteArray = new byte[sizeof(T)];
        memcpy(byteArray, &value, sizeof(T));
        return byteArray;
    }
    */

    // This implementation returns a std::vector<byte> rather than a pointer so memory management is simpler
    template<typename T>
    static std::vector<byte> ToByteArray(T value)
    {
        std::vector<byte> result(sizeof(T));
        memcpy(result.data(), &value, sizeof(T));
        return result;
    }
};

#endif