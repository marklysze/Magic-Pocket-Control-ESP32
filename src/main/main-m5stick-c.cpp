// Magic Pocket Control, by Mark Sze
// For viewing and changing basic settings on Blackmagic Design Pocket Cinema Cameras and URSA 4.6K G2 and 12K.

#define USING_TFT_ESPI 0  // Using the TFT_eSPI graphics library <-- must include this in every main file, 0 = not using, 1 = using

#include <M5StickCPlus.h>
//#include <algorithm>

// M5Stack include a "min" macro in their In_eSPI.h file, we don't want to use that, so undefine it
#undef min

// Main BMD Libraries
// #include "Camera\ConstantsTypes.h"
#include "Camera\PacketWriter.h"
// #include "CCU\CCUUtility.h"
// #include "CCU\CCUPacketTypes.h"
// #include "CCU\CCUValidationFunctions.h"
#include "Camera\BMDCameraConnection.h"
// #include "Camera\BMDCamera.h"
// #include "BMDControlSystem.h"

// Core variables and control system
BMDCameraConnection cameraConnection;
std::shared_ptr<BMDControlSystem> BMDControlSystem::instance = nullptr; // Required for Singleton pattern and the constructor for BMDControlSystem
BMDCameraConnection* BMDCameraConnection::instancePtr = &cameraConnection; // Required for the scan function to run non-blocking and call the object back with the result

void setup() {

    Serial.begin(115200);

    // SET DEBUG LEVEL
    Debug.setDebugLevel(DBG_VERBOSE);
    Debug.timestampOn();

    M5.begin();

    // Initialise the screen
    M5.Lcd.setRotation(3);

    // Prepare for Bluetooth connections and start scanning for cameras
    cameraConnection.initialise(); // For Serial-based pass key entry

}

void loop() {

    static unsigned long lastConnectedTime = 0;
    const unsigned long reconnectInterval = 5000;  // 5 seconds (milliseconds)

    unsigned long currentTime = millis();

    static bool hitRecord = false; // Let's just hit record once.


    if (cameraConnection.status == BMDCameraConnection::ConnectionStatus::Disconnected && currentTime - lastConnectedTime >= reconnectInterval) {
        DEBUG_VERBOSE("Disconnected for too long, trying to reconnect");

        cameraConnection.scan();
    }
  else if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::Connected)
  {

    // Example of hitting record once we know the state of the camera
    if(!hitRecord)
    {
        DEBUG_DEBUG("We're connected! Let's hit record once we have the TransportInfo back from the camera as to the state of the camera.");
        
        // Do we have a camera instance created (happens when connected)
        if(BMDControlSystem::getInstance()->hasCamera())
        {
            // Get the camera instance so we can check the state of it
            auto camera = BMDControlSystem::getInstance()->getCamera();

            // Only hit record if we have the Transport Mode info (knowing if it's recording) and we're not already recording.
            if(camera->hasTransportMode() && !camera->isRecording)
            {
                // Record button
                auto transportInfo = camera->getTransportMode();

                DEBUG_VERBOSE("Record Start");
                transportInfo.mode = CCUPacketTypes::MediaTransportMode::Record;

                // Send the packet to the camera to start recording
                PacketWriter::writeTransportInfo(transportInfo, &cameraConnection);
            }
        }
    }

    // WHERE THE ACTION HAPPENS
    // We can do other important things in here, such as call a function to look for the status of the camera, use buttons / keypads to update the camera settings

    lastConnectedTime = currentTime;
  }
  else if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::ScanningFound)
  {
    DEBUG_DEBUG("Cameras found!");

    cameraConnection.connect(cameraConnection.cameraAddresses[0]);

    lastConnectedTime = currentTime;
  }
  else if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::ScanningNoneFound)
  {
    DEBUG_VERBOSE("Status Scanning NONE Found. Marking as Disconnected.");
    cameraConnection.status = BMDCameraConnection::Disconnected;
    lastConnectedTime = currentTime;
  }

  delay(500);
}