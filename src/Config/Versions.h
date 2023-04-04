#ifndef VERSIONS_H
#define VERSIONS_H

#include <string>
#include <vector>

class ProtocolVersionNumber {
    public:

        // THESE THREE VALUES DETERMINE THE COMPATIBILITY OF THIS PROGRAM - ONLY kMajor IS CONSIDERED, SEE CompatibilityVerified FUNCTION
        static const int kMajor = 0;
        static const int kMinor = 1;
        static const int kPatch = 0;

        // The Protocol of the camera is compatible if the major versions are compatible
        static bool CompatibilityVerified(int major, int minor, int patch)
        {
            return kMajor == major;
        }

        // Takes a version string (e.g. "0.1.0"), and returns it as a vector of ints {0, 1, 0};
        static std::vector<int> ConvertVersionStringToInts(std::string versionString)
        {
            std::vector<int> cameraVersionNumbers;
            if (!versionString.empty()) {
                std::string protocolVersion = versionString;
                size_t pos = 0;
                std::string delimiter = ".";
                while ((pos = protocolVersion.find(delimiter)) != std::string::npos) {
                    int versionNumber = std::stoi(protocolVersion.substr(0, pos));
                    cameraVersionNumbers.push_back(versionNumber);
                    protocolVersion.erase(0, pos + delimiter.length());
                }
                cameraVersionNumbers.push_back(std::stoi(protocolVersion));
            }

            return cameraVersionNumbers;
        }
};

#endif