#include <Arduino.h>
#include <string.h>

#include "Arduino_DebugUtils.h" // Debugging to Serial - https://github.com/arduino-libraries/Arduino_DebugUtils

// Main BMD Libraries
#include "Camera/ConstantsTypes.h"
#include "Camera/PacketWriter.h"
#include "CCU/CCUUtility.h"
#include "CCU/CCUPacketTypes.h"
#include "CCU/CCUValidationFunctions.h"
#include "Camera/BMDCameraConnection.h"
#include "Camera/BMDCamera.h"
#include "BMDControlSystem.h"

BMDCameraConnection cameraConnection;
std::shared_ptr<BMDControlSystem> BMDControlSystem::instance = nullptr; // Required for Singleton pattern and the constructor for BMDControlSystem
BMDCameraConnection* BMDCameraConnection::instancePtr = &cameraConnection; // Required for the scan function to run non-blocking and call the object back with the result


#define USING_TFT_ESPI 0    // Not using the TFT_eSPI graphics library <-- must include this in every main file, 0 = not using, 1 = using
#define USING_M5GFX = 0   // Using the M5GFX graphics library <-- must include this in every main file, 0 = not using, 1 = using

void setup() {

  Serial.begin(115200);

  // SET DEBUG LEVEL
  Debug.setDebugLevel(DBG_VERBOSE);
  Debug.timestampOn();

  // Prepare for Bluetooth connections and start scanning for cameras
  cameraConnection.initialise(); // Serial pin code entry, not touch screen
}

int memoryLoopCounter;
bool hitRecord = false; // Let's just hit record once.

void loop() {

  static unsigned long lastConnectedTime = 0;
  const unsigned long reconnectInterval = 5000;  // 5 seconds (milliseconds)

  unsigned long currentTime = millis();

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

  // Keep track of the memory use to check that there aren't memory leaks (or significant memory leaks)
  if(Debug.getDebugLevel() >= DBG_VERBOSE && memoryLoopCounter++ % 400 == 0)
  {
    DEBUG_VERBOSE("Heap Size Free: %d of %d", ESP.getFreeHeap(), ESP.getHeapSize());
  }

  delay(500);
}