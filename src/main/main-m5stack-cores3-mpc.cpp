#define USING_TFT_ESPI 0    // Not using the TFT_eSPI graphics library <-- must include this in every main file, 0 = not using, 1 = using
#define USING_M5GFX 1       // Using the M5GFX library

#include <Arduino.h>
#include <string.h>

#include "Arduino_DebugUtils.h" // Debugging to Serial - https://github.com/arduino-libraries/Arduino_DebugUtils

// Main BMD Libraries
#include "Camera\ConstantsTypes.h"
#include "Camera\PacketWriter.h"
#include "CCU\CCUUtility.h"
#include "CCU\CCUPacketTypes.h"
#include "CCU\CCUValidationFunctions.h"
#include "Camera\BMDCameraConnection.h"
#include "Camera\BMDCamera.h"
#include "BMDControlSystem.h"

// Include the watchdog library so we can stop it timing out while pass key entry.
#include "esp_task_wdt.h"

// Based on M5CoreS3 Demo - M5 libraries
#include <nvs_flash.h>
#include "config.h"
#include "M5GFX.h"
#include "M5Unified.h"
#include "lgfx/v1/Touch.hpp"
static M5GFX display;
static M5Canvas window(&M5.Display);
static M5Canvas spritePassKey(&M5.Display);

// Lato font from Google Fonts
// Agency FB font is free for commercial use, copied from Windows fonts
// Rather than using font sizes, we use specific fonts for each size as it renders better on screen
#include "Lato_Regular12pt7b.h" // Slightly larger version
#include "Lato_Regular11pt7b.h" // Standard font
#include "Lato_Regular5pt7b.h" // Small version
#include "AgencyFB_Regular7pt7b.h" // Agency FB for tiny text
#include "AgencyFB_Bold9pt7b.h" // Agency FB small for above buttons
#include "AgencyFB_Regular9pt7b.h" // Agency FB small-medium for above buttons


// Screen width and height
#define IWIDTH 320
#define IHEIGHT 240

// Sprites for Images
LGFX_Sprite spriteMPCSplash;
LGFX_Sprite spriteBluetooth;
LGFX_Sprite spritePocket4k;
LGFX_Sprite spriteWBBright;
LGFX_Sprite spriteWBCloud;
LGFX_Sprite spriteWBFlourescent;
LGFX_Sprite spriteWBIncandescent;
LGFX_Sprite spriteWBMixedLight;
LGFX_Sprite spriteWBBrightBG;
LGFX_Sprite spriteWBCloudBG;
LGFX_Sprite spriteWBFlourescentBG;
LGFX_Sprite spriteWBIncandescentBG;
LGFX_Sprite spriteWBMixedLightBG;

// Images
#include "Images\MPCSplash-M5Stack-CoreS3.h"
#include "Images\ImageBluetooth.h"
#include "Images\ImagePocket4k.h"
#include "Images\WBBright.h"
#include "Images\WBCloud.h"
#include "Images\WBFlourescent.h"
#include "Images\WBIncandescent.h"
#include "Images\WBMixedLight.h"
#include "Images\WBBrightBG.h"
#include "Images\WBCloudBG.h"
#include "Images\WBFlourescentBG.h"
#include "Images\WBIncandescentBG.h"
#include "Images\WBMixedLightBG.h"

BMDCameraConnection cameraConnection;
std::shared_ptr<BMDControlSystem> BMDControlSystem::instance = nullptr; // Required for Singleton pattern and the constructor for BMDControlSystem
BMDCameraConnection* BMDCameraConnection::instancePtr = &cameraConnection; // Required for the scan function to run non-blocking and call the object back with the result

enum class Screens : byte
{
  PassKey = 9,
  NoConnection = 10,
  Dashboard = 100,
  Recording = 101,
  ISO = 102,
  ShutterAngleSpeed = 103,
  WhiteBalanceTint = 104,
  Codec = 105,
  Framerate = 106,
  Resolution = 107,
  Media = 108,
  Lens = 109,
  Slate = 110,
  Project = 111
};

Screens connectedScreenIndex = Screens::NoConnection; // The index of the screen we're on:
// 9 is Pass Key
// 10 is No Connection
// 100 is Dashboard
// 101 is Recording
// 102 is ISO
// 103 is Shutter Angle & Shutter Speed
// 104 is WB / Tint
// 105 is Codec
// 106 is Framerate
// 107 is Resolution (one for each camera group, 4K, 6K/G2/Pro, Mini Pro G2, Mini Pro 12K)
// 108 is Media
// 109 is Lens

// Keep track of the last camera modified time that we refreshed a screen so we don't keep refreshing a screen when the camera object remains unchanged.
static unsigned long lastRefreshedScreen = 0;

int tapped_x = -1;
int tapped_y = -1;

// Display elements on the screen common to all pages
void Screen_Common(int sideBarColour)
{
    // Sidebar colour
    window.fillRect(0, 0, 13, IHEIGHT, sideBarColour);
    window.fillRect(13, 0, 2, IHEIGHT, TFT_DARKGREY);

    if(BMDControlSystem::getInstance()->hasCamera())
    {
      auto camera = BMDControlSystem::getInstance()->getCamera();

      // Bottom Buttons
      window.fillSmoothRoundRect(30, 210, 80, 40, 3, TFT_DARKCYAN);
      window.setTextColor(TFT_WHITE);
      window.drawCenterString("DASH", 70, 217, &AgencyFB_Bold9pt7b);

      if(camera->isRecording)
        window.fillSmoothCircle(IWIDTH / 2, 240, 30, TFT_RED);
      else
      {
        // Two outlines to make it a bit thicker
        window.drawCircle(IWIDTH / 2, 240, 30, TFT_RED);
        window.drawCircle(IWIDTH / 2, 240, 29, TFT_RED);
      }

      window.fillSmoothRoundRect(210, 210, 80, 40, 3, TFT_DARKCYAN);
      window.drawCenterString("CODEC", 250, 217, &AgencyFB_Bold9pt7b); //fonts::FreeMono9pt7b);

    }
}

void Screen_Common_Connected()
{
  if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::Connected && BMDControlSystem::getInstance()->hasCamera())
  {
    Screen_Common(cameraConnection.getInitialPayloadTime() < millis() ? TFT_GREEN : TFT_DARKGREY);

    // Show the recording outline
    if(BMDControlSystem::getInstance()->getCamera()->isRecording)
    {
      window.drawRect(15, 0, IWIDTH - 15, IHEIGHT, TFT_RED);
      window.drawRect(16, 1, IWIDTH - 13, IHEIGHT - 2, TFT_RED);
    }
  }
}


// Screen for when there's no connection, it's scanning, and it's trying to connect.
void Screen_NoConnection()
{
  // The camera to connect to.
  int connectToCameraIndex = -1;

  connectedScreenIndex = Screens::NoConnection;

  spriteMPCSplash.pushSprite(&window, 0, 0);

  // Black background for text and Bluetooth Logo
  window.fillRect(0, 3, IWIDTH, 51, TFT_BLACK);

  // Bluetooth Image
  spriteBluetooth.pushSprite(&window, 26, 6);

  switch(cameraConnection.status)
  {
    case BMDCameraConnection::Scanning:
      Screen_Common(TFT_BLUE); // Common elements
      window.drawString("Scanning...", 70, 20);
      break;
    case BMDCameraConnection::ScanningFound:
      Screen_Common(TFT_BLUE); // Common elements
      if(cameraConnection.cameraAddresses.size() == 1)
      {
        window.drawString("Found, connecting...", 70, 20);
        connectToCameraIndex = 0;
      }
      else
        window.drawString("Found cameras", 70, 20); // Multiple camera selection is below
      break;
    case BMDCameraConnection::ScanningNoneFound:
      Screen_Common(TFT_RED); // Common elements
      window.drawString("No camera found", 70, 20);
      break;
    case BMDCameraConnection::Connecting:
      Screen_Common(TFT_YELLOW); // Common elements
      window.drawString("Connecting...", 70, 20);
      break;
    case BMDCameraConnection::NeedPassKey:
      Screen_Common(TFT_PURPLE); // Common elements
      window.drawString("Need Pass Key", 70, 20);
      break;
    case BMDCameraConnection::FailedPassKey:
      Screen_Common(TFT_ORANGE); // Common elements
      window.drawString("Wrong Pass Key", 70, 20);
      break;
    case BMDCameraConnection::Disconnected:
      Screen_Common(TFT_RED); // Common elements
      window.drawString("Disconnected (wait)", 70, 20);
      break;
    case BMDCameraConnection::IncompatibleProtocol:
      // Note: This needs to be worked on as there's no incompatible protocol connections yet.
      Screen_Common(TFT_RED); // Common elements
      window.drawString("Incompatible Protocol", 70, 20);
      break;
    default:
      break;
  }

  // Show up to two cameras
  int cameras = cameraConnection.cameraAddresses.size();
  for(int count = 0; count < cameras && count < 2; count++)
  {
    // Cameras
    window.fillRoundRect(25 + (count * 125) + (count * 10), 60, 125, 100, 5, TFT_DARKGREY);

    // Highlight the camera to connect to
    if(connectToCameraIndex != -1 && connectToCameraIndex == count)
    {
      window.drawRoundRect(25 + (count * 125) + (count * 10), 60, 5, 2, TFT_GREEN);
    }

    spritePocket4k.pushSprite(&window, 33 + (count * 125) + (count * 10), 69);

    window.drawString(cameraConnection.cameraAddresses[count].toString().c_str(), 33 + (count * 125) + (count * 10), 144, &Lato_Regular5pt7b);
  }

  // If there's more than one camera, check for a tap to see if they have nominated one to connect to
  if(cameras > 1 && tapped_x != -1)
  {
      if(tapped_x >= 25 && tapped_y >= 60 && tapped_x <= 150 && tapped_y <= 160)
      {
        // First camera tapped
        connectToCameraIndex = 0;
      }
      else if(tapped_x >= 160 && tapped_y >= 60 && tapped_x <= 285 && tapped_y <= 160)
      {
        // Second camera tapped
        connectToCameraIndex = 1;
      }
  }

  window.pushSprite(0, 0);

  if(connectToCameraIndex != -1)
  {
    cameraConnection.connect(cameraConnection.cameraAddresses[connectToCameraIndex]);
    connectToCameraIndex = -1;
  }
}

// Default screen for connected state
void Screen_Dashboard(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Dashboard;

  auto camera = BMDControlSystem::getInstance()->getCamera();
  int xshift = 0;

  // If we have a tap, we should determine if it is on anything
  bool tappedAction = false;
  if(tapped_x != -1)
  {

    if(tapped_x >= 20 && tapped_y >= 5 && tapped_x <= 315 && tapped_y <= 205)
    {
      if(tapped_x >= 20 && tapped_y >= 5 && tapped_x <= 95 && tapped_y <= 70)
      {
        // ISO
        connectedScreenIndex = Screens::ISO;
        lastRefreshedScreen = 0; // Forces a refresh
        return;
      }
      else if(tapped_x >= 100 && tapped_y >= 5 && tapped_x <= 175 && tapped_y <= 70)
      {
        // Shutter Speed / Angle
        connectedScreenIndex = Screens::ShutterAngleSpeed;
        lastRefreshedScreen = 0; // Forces a refresh
        return;
      }
      else if(tapped_x >= 180 && tapped_y >= 5 && tapped_x <= 315 && tapped_y <= 70)
      {
        // White Balance & Tint
        connectedScreenIndex = Screens::WhiteBalanceTint;
        lastRefreshedScreen = 0; // Forces a refresh
        return;
      }
      else if(tapped_x >= 20 && tapped_y >= 75 && tapped_x <= 175 && tapped_y <= 115)
      {
        // Codec
        connectedScreenIndex = Screens::Codec;
        lastRefreshedScreen = 0; // Forces a refresh
        return;
      }
      else if(tapped_x >= 180 && tapped_y >= 75 && tapped_x <= 315 && tapped_y <= 115)
      {
        // Frame Rate
        DEBUG_INFO("No Frame Rate Created Yet");
      }
      else if(tapped_x >= 20 && tapped_y >= 120 && tapped_x <= 120 && tapped_y <= 160)
      {
        connectedScreenIndex = Screens::Media;
        lastRefreshedScreen = 0; // Forces a refresh
        return;
      }
      else if(tapped_x >= 125 && tapped_y >= 120 && tapped_x <= 315 && tapped_y <= 160)
      {
        // Resolution
        connectedScreenIndex = Screens::Resolution;
        lastRefreshedScreen = 0; // Forces a refresh
        return;
      }
      else if(camera->hasHasLens() && tapped_x >= 20 && tapped_y >= 165 && tapped_x <= 315 && tapped_y <= 205)
      {
        // Lens
        connectedScreenIndex = Screens::Lens;
        lastRefreshedScreen = 0; // Forces a refresh
        return;
      }
    }
  }

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen Dashboard Refreshing.");

  window.fillSprite(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // M5GFX, set font here rather than on each drawString line
  window.setFont(&Lato_Regular11pt7b);

  // ISO
  if(camera->hasSensorGainISOValue())
  {
    window.fillSmoothRoundRect(20, 5, 75, 65, 3, TFT_DARKGREY);
    window.setTextColor(TFT_WHITE);

    window.drawCentreString(String(camera->getSensorGainISOValue()), 58, 23);

    window.drawCentreString("ISO", 58, 50, &AgencyFB_Regular7pt7b);
  }

  // Shutter
  xshift = 80;
  if(camera->hasShutterAngle())
  {
    window.fillSmoothRoundRect(20 + xshift, 5, 75, 65, 3, TFT_DARKGREY);
    window.setTextColor(TFT_WHITE);

    if(camera->shutterValueIsAngle)
    {
      // Shutter Angle
      int currentShutterAngle = camera->getShutterAngle();
      float ShutterAngleFloat = currentShutterAngle / 100.0;

      window.drawCentreString(String(ShutterAngleFloat, (currentShutterAngle % 100 == 0 ? 0 : 1)), 58 + xshift, 23);
    }
    else
    {
      // Shutter Speed
      int currentShutterSpeed = camera->getShutterSpeed();

      window.drawCentreString("1/" + String(currentShutterSpeed), 58 + xshift, 23);
    }

    window.drawCentreString(camera->shutterValueIsAngle ? "DEGREES" : "SPEED", 58 + xshift, 50, &AgencyFB_Regular7pt7b); //  "SHUTTER"
  }

  // WhiteBalance and Tint
  xshift += 80;
  if(camera->hasWhiteBalance() || camera->hasTint())
  {
    window.fillSmoothRoundRect(20 + xshift, 5, 135, 65, 3, TFT_DARKGREY);
    window.setTextColor(TFT_WHITE);

    if(camera->hasWhiteBalance())
      window.drawCentreString(String(camera->getWhiteBalance()), 58 + xshift, 23);

    window.drawCentreString("WB", 58 + xshift, 50, &AgencyFB_Regular7pt7b);

    xshift += 66;

    if(camera->hasTint())
      window.drawCentreString(String(camera->getTint()), 58 + xshift, 23);

    window.drawCentreString("TINT", 58 + xshift, 50, &AgencyFB_Regular7pt7b);
  }

  // Codec
  if(camera->hasCodec())
  {
    window.fillSmoothRoundRect(20, 75, 155, 40, 3, TFT_DARKGREY);

    window.drawCentreString(camera->getCodec().to_string().c_str(), 97, 84);
  }

  // Media
  if(camera->getMediaSlots().size() != 0)
  {
    std::string slotString;

    for(int i = 0; i < camera->getMediaSlots().size(); i++)
    {
      if(camera->getMediaSlots()[i].active)
      {
        switch(camera->getMediaSlots()[i].medium)
        {
          case CCUPacketTypes::ActiveStorageMedium::CFastCard:
            slotString = "CFAST";
            break;
          case CCUPacketTypes::ActiveStorageMedium::SDCard:
            slotString = "SD";
            break;
          case CCUPacketTypes::ActiveStorageMedium::SSDRecorder:
            slotString = "SSD";
            break;
          case CCUPacketTypes::ActiveStorageMedium::USB:
            slotString = "USB";
            break;
        }

        if(!slotString.empty())
          break;
      }
    }

    if(!slotString.empty())
    {
      window.fillSmoothRoundRect(20, 120, 100, 40, 3, TFT_DARKGREY);

      window.drawCentreString(slotString.c_str(), 70, 130);

      // Show recording error
      if(camera->hasRecordError())
        window.drawRoundRect(20, 120, 100, 40, 3, TFT_RED);
    }
    else
    {
      // Show no Media
      window.fillSmoothRoundRect(20, 120, 100, 40, 3, TFT_DARKGREY);
      window.drawCentreString("NO MEDIA", 70, 135, &AgencyFB_Regular7pt7b);
    }
  }

  // Recording Format - Frame Rate and Resolution
  if(camera->hasRecordingFormat())
  {
    // Frame Rate
    window.fillSmoothRoundRect(180, 75, 135, 40, 3, TFT_DARKGREY);

    window.drawCentreString(camera->getRecordingFormat().frameRate_string().c_str(), 237, 84);

    window.drawCentreString("fps", 285, 89, &AgencyFB_Regular7pt7b);

    // Resolution
    window.fillSmoothRoundRect(125, 120, 190, 40, 3, TFT_DARKGREY);

    std::string resolution = camera->getRecordingFormat().frameDimensionsShort_string();
    window.drawCentreString(resolution.c_str(), 220, 130);
  }

  // Lens
  if(camera->hasHasLens())
  {
    // Lens
    window.fillSmoothRoundRect(20, 165, 295, 40, 3, TFT_DARKGREY);

    if(camera->hasFocalLengthMM() && camera->hasApertureFStopString())
    {
      auto focalLength = camera->getFocalLengthMM();
      std::string focalLengthMM = std::to_string(focalLength);
      std::string combined = focalLengthMM + "mm";

      window.drawString(combined.c_str(), 30, 174);
      window.drawString(camera->getApertureFStopString().c_str(), 100, 174);
    }
  }

  window.pushSprite(0, 0);
}


void Screen_Recording(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Recording;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // If we have a tap, we should determine if it is on anything
  bool tappedAction = false;
  if(tapped_x != -1 && camera->hasTransportMode())
  {
    if(tapped_x >= 195 && tapped_y <= 128)
    {
      // Record button
      auto transportInfo = camera->getTransportMode();

      if(camera->isRecording)
      {
        DEBUG_VERBOSE("Record Stop");
        transportInfo.mode = CCUPacketTypes::MediaTransportMode::Preview;
      }
      else
      {
        DEBUG_VERBOSE("Record Start");
        transportInfo.mode = CCUPacketTypes::MediaTransportMode::Record;
      }

      PacketWriter::writeTransportInfo(transportInfo, &cameraConnection);

      tappedAction = true;
    }
  }

    // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();

  DEBUG_DEBUG("Screen Recording Refreshed.");

  window.fillSprite(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // M5GFX, set font here rather than on each drawString line
  window.setFont(&Lato_Regular11pt7b);

  // Record button
  if(camera->isRecording) window.fillSmoothCircle(257, 63, 58, Constants::DARK_RED); // Recording solid
  window.drawCircle(257, 63, 57, (camera->isRecording ? TFT_RED : TFT_LIGHTGREY)); // Outer
  window.fillSmoothCircle(257, 63, 38, camera->isRecording ? TFT_RED : TFT_LIGHTGREY); // Inner

  // Timecode
  window.setTextColor(camera->isRecording ? TFT_RED : TFT_WHITE);
  window.drawString(camera->getTimecodeString().c_str(), 30, 57);

  // Remaining time and any errors
  if(camera->getMediaSlots().size() != 0 && camera->hasActiveMediaSlot())
  {
    window.setTextColor(TFT_LIGHTGREY);
    window.drawString((camera->getActiveMediaSlot().GetMediumString() + " " + camera->getActiveMediaSlot().remainingRecordTimeString).c_str(), 30, 130);

    window.drawString("REMAINING TIME", 30, 153, &Lato_Regular5pt7b);

    // Show any media record errors
    if(camera->hasRecordError())
    {
      window.setTextColor(TFT_RED);
      window.drawString("RECORD ERROR", 30, 20);
    }
  }

  window.pushSprite(0, 0);
}

void Screen_ISO(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::ISO;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // ISO Values, 200 / 400 / 1250 / 3200 / 8000

  // If we have a tap, we should determine if it is on anything
  bool tappedAction = false;
  if(tapped_x != -1)
  {
    if(tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 310 && tapped_y <= 160)
    {
      // Tapped within the area
      int newISO = 0;

      if(tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 110 && tapped_y <= 70)
        newISO = 200;
      else if(tapped_x >= 115 && tapped_y >= 30 && tapped_x <= 205 && tapped_y <= 70)
        newISO = 400;
      else if(tapped_x >= 210 && tapped_y >= 30 && tapped_x <= 310 && tapped_y <= 70)
        newISO = 8000;

      else if(tapped_x >= 20 && tapped_y >= 75 && tapped_x <= 110 && tapped_y <= 115)
        newISO = 640;
      else if(tapped_x >= 115 && tapped_y >= 75 && tapped_x <= 205 && tapped_y <= 115)
        newISO = 800;
      else if(tapped_x >= 210 && tapped_y >= 75 && tapped_x <= 310 && tapped_y <= 115)
        newISO = 12800;

      else if(tapped_x >= 20 && tapped_y >= 120 && tapped_x <= 110 && tapped_y <= 160)
        newISO = 1250;
      else if(tapped_x >= 115 && tapped_y >= 120 && tapped_x <= 205 && tapped_y <= 160)
        newISO = 3200;

      if(newISO != 0)
      {
        // ISO selected, send it to the camera
        PacketWriter::writeISO(newISO, &cameraConnection);

        tappedAction = true;
      }
    }

  }

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen ISO Refreshed.");

  window.fillSprite(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // M5GFX, set font here rather than on each drawString line
  window.setFont(&Lato_Regular11pt7b);

  // Get the current ISO value
  int currentISO = 0;
  if(camera->hasSensorGainISOValue())
    currentISO = camera->getSensorGainISOValue();

  // ISO label
  window.setTextColor(TFT_WHITE);
  window.drawString("ISO", 30, 9, &AgencyFB_Bold9pt7b);

  // window.textbgcolor = TFT_DARKGREY;

  // 200
  int labelISO = 200;
  window.fillSmoothRoundRect(20, 30, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelISO).c_str(), 65, 41);

  // 400
  labelISO = 400;
  window.fillSmoothRoundRect(115, 30, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelISO).c_str(), 160, 36);
  window.drawCentreString("NATIVE", 160, 59, &Lato_Regular5pt7b);

  // 8000
  labelISO = 8000;
  window.fillSmoothRoundRect(210, 30, 100, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelISO).c_str(), 260, 41);

  // 640
  labelISO = 640;
  window.fillSmoothRoundRect(20, 75, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelISO).c_str(), 65, 87);

  // 800
  labelISO = 800;
  window.fillSmoothRoundRect(115, 75, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelISO).c_str(), 160, 87);

  // 12800
  labelISO = 12800;
  window.fillSmoothRoundRect(210, 75, 100, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelISO).c_str(), 260, 87);

  // 1250
  labelISO = 1250;
  window.fillSmoothRoundRect(20, 120, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelISO).c_str(), 65, 131);

  // 3200
  labelISO = 3200;
  window.fillSmoothRoundRect(115, 120, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelISO).c_str(), 160, 126);
  window.drawCentreString("NATIVE", 160, 149, &Lato_Regular5pt7b);

  // Custom ISO - show if ISO is not one of the above values
  if(currentISO != 0)
  {
    // Only show the ISO value if it's not a standard one
    if(currentISO != 200 && currentISO != 400 && currentISO != 640 && currentISO != 800 && currentISO != 1250 && currentISO != 3200 && currentISO != 8000 && currentISO != 12800)
    {
      window.fillSmoothRoundRect(210, 120, 100, 40, 3, TFT_DARKGREEN);
      // window.textbgcolor = TFT_DARKGREEN;
      window.drawCentreString(String(currentISO).c_str(), 260, 126);
      window.drawCentreString("CUSTOM", 260, 149, &Lato_Regular5pt7b);
    }
  }

  window.pushSprite(0, 0);
}


void Screen_ShutterAngle(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::ShutterAngleSpeed;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // Shutter Angle Values: 15, 60, 90, 120, 150, 180, 270, 360, CUSTOM
  // Note that the protocol takes shutter angle times 100, so 180 = 180 x 100 = 18000. This is so it can accommodate decimal places, like 172.8 degrees = 17280 for the protocol.

  // If we have a tap, we should determine if it is on anything
  bool tappedAction = false;
  if(tapped_x != -1)
  {
    if(tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 310 && tapped_y <= 160)
    {
      // Tapped within the area
      int newShutterAngle = 0;

      if(tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 110 && tapped_y <= 70)
        newShutterAngle = 1500;
      else if(tapped_x >= 115 && tapped_y >= 30 && tapped_x <= 205 && tapped_y <= 70)
        newShutterAngle = 6000;
      else if(tapped_x >= 210 && tapped_y >= 30 && tapped_x <= 310 && tapped_y <= 70)
        newShutterAngle = 9000;

      else if(tapped_x >= 20 && tapped_y >= 75 && tapped_x <= 110 && tapped_y <= 115)
        newShutterAngle = 12000;
      else if(tapped_x >= 115 && tapped_y >= 75 && tapped_x <= 205 && tapped_y <= 115)
        newShutterAngle = 15000;
      else if(tapped_x >= 210 && tapped_y >= 75 && tapped_x <= 310 && tapped_y <= 115)
        newShutterAngle = 18000;

      else if(tapped_x >= 20 && tapped_y >= 120 && tapped_x <= 110 && tapped_y <= 160)
        newShutterAngle = 27000;
      else if(tapped_x >= 115 && tapped_y >= 120 && tapped_x <= 205 && tapped_y <= 160)
        newShutterAngle = 36000;

      if(newShutterAngle != 0)
      {
        // Shutter Angle selected, send it to the camera
        PacketWriter::writeShutterAngle(newShutterAngle, &cameraConnection);

        tappedAction = true;
      }
    }
  }

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen Shutter Angle Refreshed.");

  window.fillSprite(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // Get the current Shutter Angle (comes through X100, so 180 degrees = 18000)
  int currentShutterAngle = 0;
  if(camera->hasShutterAngle())
    currentShutterAngle = camera->getShutterAngle();

  // Shutter Angle label
  window.setTextColor(TFT_WHITE);
  window.drawString("DEGREES", 265, 9, &AgencyFB_Regular7pt7b);
  window.drawString("SHUTTER ANGLE", 30, 9, &AgencyFB_Bold9pt7b);

  // 15
  int labelShutterAngle = 1500;
  window.fillSmoothRoundRect(20, 30, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelShutterAngle / 100).c_str(), 65, 41);

  // 60
  labelShutterAngle = 6000;
  window.fillSmoothRoundRect(115, 30, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelShutterAngle / 100).c_str(), 160, 41);

  // 90
  labelShutterAngle = 9000;
  window.fillSmoothRoundRect(210, 30, 100, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelShutterAngle / 100).c_str(), 260, 41);

  // 120
  labelShutterAngle = 12000;
  window.fillSmoothRoundRect(20, 75, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelShutterAngle / 100).c_str(), 65, 87);

  // 150
  labelShutterAngle = 15000;
  window.fillSmoothRoundRect(115, 75, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelShutterAngle / 100).c_str(), 160, 87);

  // 180 (with a border around it)
  labelShutterAngle = 18000;
  window.fillSmoothRoundRect(210, 75, 100, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawRoundRect(210, 75, 100, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelShutterAngle / 100).c_str(), 260, 87);

  // 270
  labelShutterAngle = 27000;
  window.fillSmoothRoundRect(20, 120, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelShutterAngle / 100).c_str(), 65, 131);

  // 360
  labelShutterAngle = 36000;
  window.fillSmoothRoundRect(115, 120, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString(String(labelShutterAngle / 100).c_str(), 160, 131);

  // Custom Shutter Angle - show if not one of the above values
  if(currentShutterAngle != 0)
  {
    // Only show the Shutter angle value if it's not a standard one
    if(currentShutterAngle != 1500 && currentShutterAngle != 6000 && currentShutterAngle != 9000 && currentShutterAngle != 12000 && currentShutterAngle != 15000 && currentShutterAngle != 18000 && currentShutterAngle != 27000 && currentShutterAngle != 36000)
    {
      float customShutterAngle = currentShutterAngle / 100.0;

      window.fillSmoothRoundRect(210, 120, 100, 40, 3, TFT_DARKGREEN);
      window.drawCentreString(String(customShutterAngle, (currentShutterAngle % 100 == 0 ? 0 : 1)).c_str(), 260, 126);
      window.drawCentreString("CUSTOM", 260, 149, &Lato_Regular5pt7b);
    }
  }

  window.pushSprite(0, 0);
}

void Screen_ShutterSpeed(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::ShutterAngleSpeed;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // Shutter Speed Values: 1/30, 1/50, 1/60, 1/125, 1/200, 1/250, 1/500, 1/2000, CUSTOM
  // Note that the protocol takes the denominator as its parameter value. So for 1/60 we'll pass 60.

  // If we have a tap, we should determine if it is on anything
  bool tappedAction = false;
  if(tapped_x != -1)
  {
    if(tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 310 && tapped_y <= 160)
    {
      // Tapped within the area
      int newShutterSpeed = 0;

      if(tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 110 && tapped_y <= 70)
        newShutterSpeed = 30;
      else if(tapped_x >= 115 && tapped_y >= 30 && tapped_x <= 205 && tapped_y <= 70)
        newShutterSpeed = 50;
      else if(tapped_x >= 210 && tapped_y >= 30 && tapped_x <= 310 && tapped_y <= 70)
        newShutterSpeed = 60;

      else if(tapped_x >= 20 && tapped_y >= 75 && tapped_x <= 110 && tapped_y <= 115)
        newShutterSpeed = 125;
      else if(tapped_x >= 115 && tapped_y >= 75 && tapped_x <= 205 && tapped_y <= 115)
        newShutterSpeed = 200;
      else if(tapped_x >= 210 && tapped_y >= 75 && tapped_x <= 310 && tapped_y <= 115)
        newShutterSpeed = 250;

      else if(tapped_x >= 20 && tapped_y >= 120 && tapped_x <= 110 && tapped_y <= 160)
        newShutterSpeed = 500;
      else if(tapped_x >= 115 && tapped_y >= 120 && tapped_x <= 205 && tapped_y <= 160)
        newShutterSpeed = 2000;

      if(newShutterSpeed != 0)
      {
        // Shutter Speed selected, send it to the camera
        PacketWriter::writeShutterSpeed(newShutterSpeed, &cameraConnection);

        tappedAction = true;
      }
    }
  }

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen Shutter Speed Refreshed.");


  window.fillSprite(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // Get the current Shutter Speed
  int currentShutterSpeed = 0;
  if(camera->hasShutterSpeed())
    currentShutterSpeed = camera->getShutterSpeed();
  else
    DEBUG_DEBUG("DO NOT HAVE SHUTTER SPEED!");

  // Shutter Speed label
  window.setTextColor(TFT_WHITE);

  if(camera->hasRecordingFormat())
  {
    window.drawRightString(camera->getRecordingFormat().frameRate_string().c_str() + String(" fps"), 310, 9, &Lato_Regular5pt7b);
  }

  window.drawString("SHUTTER SPEED", 30, 9);

  // 1/30
  int labelShutterSpeed = 30;
  window.fillSmoothRoundRect(20, 30, 90, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString("1/" + String(labelShutterSpeed), 65, 41);

  // 1/50
  labelShutterSpeed = 50;
  window.fillSmoothRoundRect(115, 30, 90, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString("1/" + String(labelShutterSpeed), 160, 41);

  // 1/60
  labelShutterSpeed = 60;
  window.fillSmoothRoundRect(210, 30, 100, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString("1/" + String(labelShutterSpeed), 260, 41);

  // 1/125
  labelShutterSpeed = 125;
  window.fillSmoothRoundRect(20, 75, 90, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString("1/" + String(labelShutterSpeed), 65, 87);

  // 1/200
  labelShutterSpeed = 200;
  window.fillSmoothRoundRect(115, 75, 90, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString("1/" + String(labelShutterSpeed), 160, 87);

  // 1/250
  labelShutterSpeed = 250;
  window.fillSmoothRoundRect(210, 75, 100, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString("1/" + String(labelShutterSpeed), 260, 87);

  // 1/500
  labelShutterSpeed = 500;
  window.fillSmoothRoundRect(20, 120, 90, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString("1/" + String(labelShutterSpeed), 65, 131);

  // 1/2000
  labelShutterSpeed = 2000;
  window.fillSmoothRoundRect(115, 120, 90, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString("1/" + String(labelShutterSpeed), 160, 131);

  // Custom Shutter Speed - show if not one of the above values
  if(currentShutterSpeed != 0)
  {
    // Only show the Shutter Speed value if it's not a standard one
    if(currentShutterSpeed != 30 && currentShutterSpeed != 50 && currentShutterSpeed != 60 && currentShutterSpeed != 125 && currentShutterSpeed != 200 && currentShutterSpeed != 250 && currentShutterSpeed != 500 && currentShutterSpeed != 2000)
    {
      window.fillSmoothRoundRect(210, 120, 100, 40, 3, TFT_DARKGREEN);
      window.drawCentreString("1/" + String(currentShutterSpeed), 260, 126);
      window.drawCentreString("CUSTOM", 260, 149, &Lato_Regular5pt7b);
    }
  }

  window.pushSprite(0, 0);
}

void Screen_WBTint(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::WhiteBalanceTint;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // White Balance and Tint
  // WB: Bright, Incandescent, Fluorescent, Mixed Light, Cloud

  // Get the current White Balance value
  int currentWB = 0;
  if(camera->hasWhiteBalance())
    currentWB = camera->getWhiteBalance();

  // Get the current Tint value
  int currentTint = 0;
  if(camera->hasTint())
    currentTint = camera->getTint();

  // If we have a tap, we should determine if it is on anything
  bool tappedAction = false;
  if(tapped_x != -1)
  {
    if(tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 315 && tapped_y <= 160)
    {
      // Tapped within the area
      int newWB = 0;
      int newTint = 0;

      // Check Preset Clicks
      if(tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 90 && tapped_y <= 70)
      {
        // Bright
        newWB = 5600;
        newTint = 10;
      }
      else if(tapped_x >= 95 && tapped_y >= 30 && tapped_x <= 165 && tapped_y <= 70)
      {
        // Incandescent
        newWB = 3200;
        newTint = 0;
      }
      else if(tapped_x >= 170 && tapped_y >= 30 && tapped_x <= 240 && tapped_y <= 70)
      {
        // Fluorescent
        newWB = 4000;
        newTint = 15;
      }
      else if(tapped_x >= 245 && tapped_y >= 30 && tapped_x <= 315 && tapped_y <= 70)
      {
        // Mixed Light
        newWB = 4500;
        newTint = 15;
      }
      else if(tapped_x >= 20 && tapped_y >= 75 && tapped_x <= 90 && tapped_y <= 115)
      {
        // Cloud
        newWB = 6500;
        newTint = 10;
      }

      // Preset clicked
      if(newWB != 0)
      {
        // Send preset to camera
        PacketWriter::writeWhiteBalance(newWB, newTint, &cameraConnection);

        tappedAction = true;
      }
      else
      {
        // Not a preset

        // WB Left
        if(currentWB != 0 && currentWB >= 2550 && tapped_x >= 95 && tapped_y >= 75 && tapped_x <= 155 && tapped_y <= 115)
        {
          // Adjust down by 50
          PacketWriter::writeWhiteBalance(currentWB - 50, currentTint, &cameraConnection);

          tappedAction = true;
        }
        else if(currentWB != 0 && currentWB <= 9950 && tapped_x >= 255 && tapped_y >= 75 && tapped_x <= 315 && tapped_y <= 115)
        {
          // Adjust up by 50
          PacketWriter::writeWhiteBalance(currentWB + 50, currentTint, &cameraConnection);

          tappedAction = true;
        }
        else if(currentWB != 0 && currentTint > -50 && tapped_x >= 95 && tapped_y >= 120 && tapped_x <= 155 && tapped_y <= 160)
        {
          // Adjust down by 1
          PacketWriter::writeWhiteBalance(currentWB, currentTint - 1, &cameraConnection);

          tappedAction = true;
        }
        else if(currentWB != 0 && currentWB <= 9950 && tapped_x >= 255 && tapped_y >= 120 && tapped_x <= 315 && tapped_y <= 160)
        {
          // Adjust up by 1
          PacketWriter::writeWhiteBalance(currentWB, currentTint + 1, &cameraConnection);

          tappedAction = true;
        }
      }
    }
  }
  

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen WB Tint Refreshed.");

  window.fillSprite(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // ISO label
  window.setTextColor(TFT_WHITE);
  window.drawString("WHITE BALANCE", 30, 9, &AgencyFB_Bold9pt7b);
  window.drawCentreString("TINT", 54, 132, &AgencyFB_Bold9pt7b);

  // Bright, 5600K
  int lblWBKelvin = 5600;
  int lblTint = 10;
  window.fillSmoothRoundRect(20, 30, 70, 40, 3, (currentWB == lblWBKelvin && currentTint == lblTint ? TFT_DARKGREEN : TFT_DARKGREY));
  if(currentWB == lblWBKelvin && currentTint == lblTint)
    spriteWBBrightBG.pushSprite(&window, 40, 35);
  else
    spriteWBBright.pushSprite(&window, 40, 35);

  // Incandescent, 3200K
  lblWBKelvin = 3200;
  lblTint = 0;
  window.fillSmoothRoundRect(95, 30, 70, 40, 3, (currentWB == lblWBKelvin && currentTint == lblTint ? TFT_DARKGREEN : TFT_DARKGREY));
  if(currentWB == lblWBKelvin && currentTint == lblTint)
    spriteWBIncandescentBG.pushSprite(&window, 115, 35);
  else
    spriteWBIncandescent.pushSprite(&window, 115, 35);

  // Fluorescent, 4000K
  lblWBKelvin = 4000;
  lblTint = 15;
  window.fillSmoothRoundRect(170, 30, 70, 40, 3, (currentWB == lblWBKelvin && currentTint == lblTint ? TFT_DARKGREEN : TFT_DARKGREY));
  if(currentWB == lblWBKelvin && currentTint == lblTint)
    spriteWBFlourescentBG.pushSprite(&window, 190, 35);
  else
    spriteWBFlourescent.pushSprite(&window, 190, 35);

  // Mixed Light, 4500K
  lblWBKelvin = 4500;
  lblTint = 15;
  window.fillSmoothRoundRect(245, 30, 70, 40, 3, (currentWB == lblWBKelvin && currentTint == lblTint ? TFT_DARKGREEN : TFT_DARKGREY));
  if(currentWB == lblWBKelvin && currentTint == lblTint)
    spriteWBMixedLightBG.pushSprite(&window, 265, 35);
  else
    spriteWBMixedLight.pushSprite(&window, 265, 35);

  // Cloud, 6500K
  lblWBKelvin = 6500;
  lblTint = 10;
  window.fillSmoothRoundRect(20, 75, 70, 40, 3, (currentWB == lblWBKelvin && currentTint == lblTint ? TFT_DARKGREEN : TFT_DARKGREY));
  if(currentWB == lblWBKelvin && currentTint == lblTint)
    spriteWBCloudBG.pushSprite(&window, 40, 80);
  else
    spriteWBCloud.pushSprite(&window, 40, 80);

  // WB Adjust Left <
  window.fillSmoothRoundRect(95, 75, 60, 40, 3, TFT_DARKGREY);
  window.drawCentreString("<", 125, 87);

  // Current White Balance Kelvin
  window.fillSmoothRoundRect(160, 75, 90, 40, 3, TFT_DARKGREEN);
  window.drawCentreString(String(currentWB), 205, 80);
  window.drawCentreString("KELVIN", 205, 103, &Lato_Regular5pt7b);

  // WB Adjust Right >
  window.fillSmoothRoundRect(255, 75, 60, 40, 3, TFT_DARKGREY);
  window.drawCentreString(">", 284, 87);

  // Tint Adjust Left <
  window.fillSmoothRoundRect(95, 120, 60, 40, 3, TFT_DARKGREY);
  window.drawCentreString("<", 125, 132);

  // Current Tint
  window.fillSmoothRoundRect(160, 120, 90, 40, 3, TFT_DARKGREEN);
  window.drawCentreString(String(currentTint), 205, 132);

  // Tint Adjust Right >
  window.fillSmoothRoundRect(255, 120, 60, 40, 3, TFT_DARKGREY);
  window.drawCentreString(">", 284, 132);

  window.pushSprite(0, 0);
}

// Codec Screen for Pocket 4K and 6K + Variants
void Screen_Codec4K6K(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Codec;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // Get the current Codec values
  CodecInfo currentCodec = camera->getCodec();

  // Codec: BRAW and ProRes

  // If we have a tap, we should determine if it is on anything
  bool tappedAction = false;
  if(tapped_x != -1 && camera->hasCodec())
  {
    if(tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 315 && tapped_y <= 160)
    {
      // Switching between BRAW and ProRes
      if(currentCodec.basicCodec != CCUPacketTypes::BasicCodec::BRAW && tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 165 && tapped_y <= 70)
      {
        // Switch to BRAW (using the last known setting)
        DEBUG_DEBUG("Changing to last unknown BRAW");
        DEBUG_DEBUG("Changing Codecs through Bluetooth is a known bug from Blackmagic Design as of April 2023");

        PacketWriter::writeCodec(camera->lastKnownBRAWIsBitrate ? camera->lastKnownBRAWBitrate : camera->lastKnownBRAWQuality, &cameraConnection);

        tappedAction = true;
      }
      else if(currentCodec.basicCodec != CCUPacketTypes::BasicCodec::ProRes && tapped_x >= 170 && tapped_y >= 30 && tapped_x <= 315 && tapped_y <= 70)
      {
        // Switch to ProRes (using the last known setting)
        DEBUG_DEBUG("Changing to last unknown ProRes");
        DEBUG_DEBUG("Changing Codecs through Bluetooth is a known bug from Blackmagic Design as of April 2023");

        PacketWriter::writeCodec(camera->lastKnownProRes, &cameraConnection);

        tappedAction = true;
      }
      else if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW)
      {
        // BRAW

        // Are we Constant Bitrate or Constant Quality
        std::string currentCodecString = currentCodec.to_string();
        auto pos = std::find(currentCodecString.begin(), currentCodecString.end(), ':');
        bool isConstantBitrate = pos != currentCodecString.end(); // Is there a colon, :, in the string? If so, it's Constant Bitrate

        // Constant Bitrate and Constant Quality
        if(tapped_x >= 20 && tapped_y >= 75 && tapped_x <= 165 && tapped_y <= 115)
        {
          // Change to Constant Bitrate if we're not already on Constant Bitrate. If it is we do nothing
          if(!isConstantBitrate)
          {
            PacketWriter::writeCodec(camera->lastKnownBRAWBitrate, &cameraConnection);

            tappedAction = true;
          }
        } 
        else if(tapped_x >= 170 && tapped_y >= 75 && tapped_x <= 315 && tapped_y <= 115)
        {
          // Change to Constant Quality if we're not already on Constant Quality. If it is we do nothing
          if(isConstantBitrate)
          {
            PacketWriter::writeCodec(camera->lastKnownBRAWQuality, &cameraConnection);

            tappedAction = true;
          }
        } 
        else
        {
          // Setting

          if(tapped_x >= 20 && tapped_y >= 120 && tapped_x <= 90 && tapped_y <= 160)
          {
            // 3:1 / Q0
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, isConstantBitrate ? CCUPacketTypes::CodecVariants::kBRAW3_1 : CCUPacketTypes::CodecVariants::kBRAWQ0), &cameraConnection);

            tappedAction = true;
          }
          else if(tapped_x >= 95 && tapped_y >= 120 && tapped_x <= 165 && tapped_y <= 160)
          {
            // 5:1 / Q1
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, isConstantBitrate ? CCUPacketTypes::CodecVariants::kBRAW5_1 : CCUPacketTypes::CodecVariants::kBRAWQ1), &cameraConnection);

            tappedAction = true;
          }
          else if(tapped_x >= 170 && tapped_y >= 120 && tapped_x <= 240 && tapped_y <= 160)
          {
            // 8:1 / Q3
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, isConstantBitrate ? CCUPacketTypes::CodecVariants::kBRAW8_1 : CCUPacketTypes::CodecVariants::kBRAWQ3), &cameraConnection);

            tappedAction = true;
          }
          else if(tapped_x >= 245 && tapped_y >= 120 && tapped_x <= 315 && tapped_y <= 160)
          {
            // 12:1 / Q5
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, isConstantBitrate ? CCUPacketTypes::CodecVariants::kBRAW12_1 : CCUPacketTypes::CodecVariants::kBRAWQ5), &cameraConnection);

            tappedAction = true;
          }
        }
      }
      else
      {
        // ProRes

        if(tapped_x >= 20 && tapped_y >= 75 && tapped_x <= 165 && tapped_y <= 115)
          {
            // HQ
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::ProRes, CCUPacketTypes::CodecVariants::kProResHQ), &cameraConnection);

            tappedAction = true;
          }
          else if(tapped_x >= 170 && tapped_y >= 75 && tapped_x <= 315 && tapped_y <= 115)
          {
            // 422
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::ProRes, CCUPacketTypes::CodecVariants::kProRes422), &cameraConnection);

            tappedAction = true;
          }
          else if(tapped_x >= 20 && tapped_y >= 120 && tapped_x <= 165 && tapped_y <= 160)
          {
            // LT
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::ProRes, CCUPacketTypes::CodecVariants::kProResLT), &cameraConnection);

            tappedAction = true;
          }
          else if(tapped_x >= 170 && tapped_y >= 120 && tapped_x <= 315 && tapped_y <= 160)
          {
            // PXY
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::ProRes, CCUPacketTypes::CodecVariants::kProResProxy), &cameraConnection);

            tappedAction = true;
          }
      }
    }

  }

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen Codec 4K/6K Refreshed.");

  window.fillSprite(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // We need to have the Codec information to show the screen
  if(!camera->hasCodec())
  {
    window.setTextColor(TFT_WHITE);
    window.drawString("NO CODEC INFO.", 30, 9);

    window.pushSprite(0, 0);

    return;
  }

  // Codec label
  window.setTextColor(TFT_WHITE);
  window.drawString("CODEC", 30, 9, &AgencyFB_Bold9pt7b);

  // BRAW and ProRes selector buttons

  // BRAW
  window.fillSmoothRoundRect(20, 30, 145, 40, 5, (currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawRoundRect(20, 30, 145, 40, 3, (currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString("BRAW", 93, 41);

  // ProRes
  window.fillSmoothRoundRect(170, 30, 145, 40, 5, (currentCodec.basicCodec == CCUPacketTypes::BasicCodec::ProRes ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawRoundRect(170, 30, 145, 40, 3, (currentCodec.basicCodec == CCUPacketTypes::BasicCodec::ProRes ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawCentreString("ProRes", 242, 41);

  if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW)
  {
    // BRAW

    // Are we Constant Bitrate or Constant Quality
    std::string currentCodecString = currentCodec.to_string();
    auto pos = std::find(currentCodecString.begin(), currentCodecString.end(), ':');
    bool isConstantBitrate = pos != currentCodecString.end(); // Is there a colon, :, in the string? If so, it's Constant Bitrate

    // Get the bitrate or quality setting
    std::size_t spaceIndex = currentCodecString.find(" ");
    std::string qualityBitrateSetting = currentCodecString.substr(spaceIndex + 1); // e.g. 3:1, Q3, etc.

    // Constant Bitrate
    window.fillSmoothRoundRect(20, 75, 145, 40, 3, (isConstantBitrate ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("BITRATE", 93, 80);
    window.drawCentreString("CONSTANT", 93, 102, &Lato_Regular5pt7b);

    // Constant Quality
    window.fillSmoothRoundRect(170, 75, 145, 40, 3, (!isConstantBitrate ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("QUALITY", 242, 80);
    window.drawCentreString("CONSTANT", 242, 102, &Lato_Regular5pt7b);

    // Setting 1 of 4
    std::string optionString = (isConstantBitrate ? "3:1" : "Q0");
    window.fillSmoothRoundRect(20, 120, 70, 40, 3, (optionString == qualityBitrateSetting ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString(optionString.c_str(), 55, 131);

    // Setting 2 of 4
    optionString = (isConstantBitrate ? "5:1" : "Q1");
    window.fillSmoothRoundRect(95, 120, 70, 40, 3, (optionString == qualityBitrateSetting ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString(optionString.c_str(), 130, 131);

  //   Setting 3 of 4
    optionString = (isConstantBitrate ? "8:1" : "Q3");
    window.fillSmoothRoundRect(170, 120, 70, 40, 3, (optionString == qualityBitrateSetting ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString(optionString.c_str(), 205, 131);

    // Setting 4 of 4
    optionString = (isConstantBitrate ? "12:1" : "Q5");
    window.fillSmoothRoundRect(245, 120, 70, 40, 3, (optionString == qualityBitrateSetting ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString(optionString.c_str(), 280, 131);
  }
  else
  {
    // ProRes

    // Get the ProRes Setting
    std::string currentCodecString = currentCodec.to_string();
    std::size_t spaceIndex = currentCodecString.find(" ");
    std::string currentProResSetting = currentCodecString.substr(spaceIndex + 1); // e.g. HQ, 422, LT, PXY

    // HQ
    std::string proResLabel = "HQ";
    window.fillSmoothRoundRect(20, 75, 145, 40, 3, (currentProResSetting == proResLabel ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString(proResLabel.c_str(), 93, 87);

    // 422
    proResLabel = "422";
    window.fillSmoothRoundRect(170, 75, 145, 40, 3, (currentProResSetting == proResLabel ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString(proResLabel.c_str(), 242, 87);

    // LT
    proResLabel = "LT";
    window.fillSmoothRoundRect(20, 120, 145, 40, 3, (currentProResSetting == proResLabel ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString(proResLabel.c_str(), 93, 131);

    // PXY
    proResLabel = "PXY";
    window.fillSmoothRoundRect(170, 120, 145, 40, 3, (currentProResSetting == proResLabel ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString(proResLabel.c_str(), 242, 131);
  }

  window.pushSprite(0, 0);
}

// Codec Screen for URSA Mini Pro G2
void Screen_CodecURSAMiniProG2(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Codec;

  auto camera = BMDControlSystem::getInstance()->getCamera();
  
  // TO DO
}

// Codec Screen for URSA Mini Pro 12K
void Screen_CodecURSAMiniPro12K(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Codec;

  auto camera = BMDControlSystem::getInstance()->getCamera();
  
  // TO DO
}

// Codec screen - redirects to appropriate screen for camera
void Screen_Codec(bool forceRefresh = false)
{
  auto camera = BMDControlSystem::getInstance()->getCamera();

  if(camera->hasCodec())
  {
    if(camera->hasModelName())
    {
      if(camera->isPocket4K6K())
        Screen_Codec4K6K(forceRefresh); // Pocket camera
      else if(camera->isURSAMiniProG2())
        Screen_CodecURSAMiniProG2(forceRefresh); // URSA Mini Pro G2
      else if(camera->isURSAMiniPro12K())
        Screen_CodecURSAMiniPro12K(forceRefresh); // URSA Mini Pro 12K
      else
        DEBUG_DEBUG("No Codec screen for this camera.");
    } 
    else
      Screen_Codec4K6K(forceRefresh); // Handle no model name in 4K/6K screen
  }
  else
    Screen_Codec4K6K(forceRefresh); // If we don't have any codec info, we show the 4K/6K screen that shows no codec

}


// Resolution screen for Pocket 4K
void Screen_Resolution4K(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Resolution;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // Pocket 4K Resolution and Sensor Windows

  // Get the current Resolution and Codec
  CCUPacketTypes::RecordingFormatData currentRecordingFormat;
  if(camera->hasRecordingFormat())
    currentRecordingFormat = camera->getRecordingFormat();

  // Get the current Codec values
  CodecInfo currentCodec = camera->getCodec();

  // If we have a tap, we should determine if it is on anything
  bool tappedAction = false;
  if(tapped_x != -1)
  {
    if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW)
    {
      int width = 0;
      int height = 0;
      bool window = false;

      if(tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 110 && tapped_y <= 70)
      {
        // 4K DCI
        width = 4096; height = 2160;
      }
      else if(tapped_x >= 115 && tapped_y >= 30 && tapped_x <= 205 && tapped_y <= 70)
      {
        // 4K 2.4:1
        width = 4096; height = 1720;
        window = true;
      }
      else if(tapped_x >= 210 && tapped_y >= 30 && tapped_x <= 310 && tapped_y <= 70)
      {
        // 4K UHD
        width = 3840; height = 2160;
        window = true;
      }

      else if(tapped_x >= 20 && tapped_y >= 75 && tapped_x <= 110 && tapped_y <= 115)
      {
        // 2.8K Ana
        width = 2880; height = 2160;
        window = true;
      }
      else if(tapped_x >= 115 && tapped_y >= 75 && tapped_x <= 205 && tapped_y <= 115)
      {
        // 2.6K 16:9
        width = 2688; height = 1512;
        window = true;
      }
      else if(tapped_x >= 210 && tapped_y >= 75 && tapped_x <= 310 && tapped_y <= 115)
      {
        // HD
        width = 1920; height = 1080;
        window = true;
      }

      if(width != 0)
      {
        // Resolution selected, write to camera
        CCUPacketTypes::RecordingFormatData newRecordingFormat = currentRecordingFormat;
        newRecordingFormat.width = width;
        newRecordingFormat.height = height;
        newRecordingFormat.windowedModeEnabled = window;
        PacketWriter::writeRecordingFormat(newRecordingFormat, &cameraConnection);

        tappedAction = true;
      }
    }
    else
    {
      // ProRes

      int width = 0;
      int height = 0;
      bool window = false;

      String currentRes = currentRecordingFormat.frameDimensionsShort_string().c_str(); // "4K DCI", "4K UHD", "HD"

      if(tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 110 && tapped_y <= 70)
      {
        // 4K
        width = 4096; height = 2160;
        window = false; // true;
      }
      else if(tapped_x >= 115 && tapped_y >= 30 && tapped_x <= 205 && tapped_y <= 70)
      {
        // 4K UHD
        width = 3840; height = 2160;
        window = true; // currentRecordingFormat.windowedModeEnabled;
      }
      else if(tapped_x >= 210 && tapped_y >= 30 && tapped_x <= 310 && tapped_y <= 70)
      {
        // HD
        width = 1920; height = 1080;
        window = true; // currentRecordingFormat.windowedModeEnabled;
      }

      // Windowed options
      /*
      else if(currentRes == "HD" && tapped_x >= 20 && tapped_y >= 120 && tapped_x <= 310 && tapped_y <= 160)
      {
        // HD - Scaled from Full sensor, 2.6K, or Windowed, can't distinguish
        width = currentRecordingFormat.width; height = currentRecordingFormat.height;
        window = true;
      }
      */

      if(width != 0)
      {
        // Resolution or Sensor Area selected, write to camera
        CCUPacketTypes::RecordingFormatData newRecordingFormat = currentRecordingFormat;
        newRecordingFormat.width = width;
        newRecordingFormat.height = height;
        newRecordingFormat.windowedModeEnabled = window;
        PacketWriter::writeRecordingFormat(newRecordingFormat, &cameraConnection);

        tappedAction = true;
      }
    }
  }

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen Resolution Pocket 4K Refreshed.");

  window.fillSprite(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW)
  {
    // Main label
    window.setTextColor(TFT_WHITE);
    window.drawString("BRAW RESOLUTION", 30, 9, &AgencyFB_Bold9pt7b);

    String currentRes = currentRecordingFormat.frameDimensionsShort_string().c_str(); // "4K DCI", "4K 2.4:1", "4K UHD", "2.8K Ana", "2.6K 16:9", "HD"

    // 4K DCI
    String labelRes = "4K DCI";
    window.fillSmoothRoundRect(20, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("4K", 65, 35);
    window.drawCentreString("DCI", 65, 58, &Lato_Regular5pt7b);

    // 4K 2.4:1
    labelRes = "4K 2.4:1";
    window.fillSmoothRoundRect(115, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("4K", 160, 35);
    window.drawCentreString("2.4:1", 160, 58, &Lato_Regular5pt7b);

    // 4K UHD
    labelRes = "4K UHD";
    window.fillSmoothRoundRect(210, 30, 100, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("4K", 260, 35);
    window.drawCentreString("UHD", 260, 58, &Lato_Regular5pt7b);

    // 2.8K Ana
    labelRes = "2.8K Ana";
    window.fillSmoothRoundRect(20, 75, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("2.8K", 65, 81);
    window.drawCentreString("ANAMORPHIC", 65, 104, &Lato_Regular5pt7b);

    // 2.6K 16:9
    labelRes = "2.6K 16:9";
    window.fillSmoothRoundRect(115, 75, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("2.6K", 160, 81);
    window.drawCentreString("16:9", 160, 104, &Lato_Regular5pt7b);

    // HD
    labelRes = "HD";
    window.fillSmoothRoundRect(210, 75, 100, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("HD", 260, 87);

    // Sensor Area

    // Pocket 4K all are Windowed except 4K
    // window.drawSmoothRoundRect(20, 120, 5, 3, 290, 40, TFT_DARKGREY); // Optionally draw a rectangle around this info.
    window.drawCentreString(currentRes == "4K" ? "FULL SENSOR" :"SENSOR WINDOWED", 165, 129);
    window.drawCentreString(currentRecordingFormat.frameWidthHeight_string().c_str(), 165, 150, &Lato_Regular5pt7b);
  }
  else if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::ProRes)
  {
    // Main label
    window.setTextColor(TFT_WHITE);
    window.drawString("ProRes RESOLUTION", 30, 9, &AgencyFB_Bold9pt7b);

    String currentRes = currentRecordingFormat.frameDimensionsShort_string().c_str(); // "4K DCI", "4K UHD", "HD"

    // 4K
    String labelRes = "4K DCI";
    window.fillSmoothRoundRect(20, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("4K", 65, 36);
    window.drawCentreString("DCI", 65, 58, &Lato_Regular5pt7b);

    // 4K UHD
    labelRes = "4K UHD";
    window.fillSmoothRoundRect(115, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("UHD", 160, 41);

    // HD
    labelRes = "HD";
    window.fillSmoothRoundRect(210, 30, 100, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("HD", 260, 41);

    // Sensor Area

    // 4K DCI is Scaled from 5.7K, Ultra HD is Scaled from Full or 5.7K, HD is Scaled from Full, 5.7K, or 2.8K (however we can't change the 5.7K or 2.8K)

    window.drawString(currentRecordingFormat.frameWidthHeight_string().c_str(), 30, 90, &Lato_Regular5pt7b);
    window.drawString("SCALED / SENSOR AREA", 30, 105);

    if(currentRes == "4K DCI")
    {
      // Full sensor area only]
      window.fillSmoothRoundRect(20, 135, 90, 40, 3, TFT_DARKGREEN);
      window.drawCentreString("FULL", 65, 145);
    }
    else if(currentRes == "4K UHD")
    {
      // Full sensor area only]
      window.fillSmoothRoundRect(20, 135, 120, 40, 3, TFT_DARKGREEN);
      window.drawCentreString("WINDOW", 80, 145);
    }
    else
    {
      // HD

      // Scaled from Full, 2.6K or Windowed (however we can't tell which)
      window.fillSmoothRoundRect(20, 135, 290, 40, 3, TFT_DARKGREEN);
      window.drawCentreString("FULL / 2.6K / WINDOW", 165, 140);
      window.drawCentreString("CHECK/SET ON CAMERA", 165, 161, &Lato_Regular5pt7b);
    }
  }
  else
    DEBUG_ERROR("Resolution Pocket 4K - Codec not catered for.");

  window.pushSprite(0, 0);
}

// Resolution screen for Pocket 6K
void Screen_Resolution6K(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Resolution;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // Pocket 6K Resolution and Sensor Windows

  // Get the current Resolution and Codec
  CCUPacketTypes::RecordingFormatData currentRecordingFormat;
  if(camera->hasRecordingFormat())
    currentRecordingFormat = camera->getRecordingFormat();

  // Get the current Codec values
  CodecInfo currentCodec = camera->getCodec();

  // If we have a tap, we should determine if it is on anything
  bool tappedAction = false;
  if(tapped_x != -1)
  {
    if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW)
    {
      int width = 0;
      int height = 0;
      bool window = false;

      if(tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 110 && tapped_y <= 70)
      {
        // 6K
        width = 6144; height = 3456;
      }
      else if(tapped_x >= 115 && tapped_y >= 30 && tapped_x <= 205 && tapped_y <= 70)
      {
        // 6K 2.4:1
        width = 6144; height = 2560;
        window = true;
      }
      else if(tapped_x >= 210 && tapped_y >= 30 && tapped_x <= 310 && tapped_y <= 70)
      {
        // 5.7K 17:9
        width = 5744; height = 3024;
        window = true;
      }

      else if(tapped_x >= 20 && tapped_y >= 75 && tapped_x <= 110 && tapped_y <= 115)
      {
        // 4K DCI
        width = 4096; height = 2160;
        window = true;
      }
      else if(tapped_x >= 115 && tapped_y >= 75 && tapped_x <= 205 && tapped_y <= 115)
      {
        // 3.7K Anamorphic
        width = 3728; height = 3104;
        window = true;
      }
      else if(tapped_x >= 210 && tapped_y >= 75 && tapped_x <= 310 && tapped_y <= 115)
      {
        // 2.8K 17:9
        width = 2868; height = 1512;
        window = true;
      }

      if(width != 0)
      {
        // Resolution selected, write to camera
        CCUPacketTypes::RecordingFormatData newRecordingFormat = currentRecordingFormat;
        newRecordingFormat.width = width;
        newRecordingFormat.height = height;
        newRecordingFormat.windowedModeEnabled = window;
        PacketWriter::writeRecordingFormat(newRecordingFormat, &cameraConnection);

        tappedAction = true;
      }
    }
    else
    {
      // ProRes

      int width = 0;
      int height = 0;
      bool window = false;

      String currentRes = currentRecordingFormat.frameDimensionsShort_string().c_str(); // "4K DCI", "4K UHD", "HD"

      if(tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 110 && tapped_y <= 70)
      {
        // 4K
        width = 4096; height = 2160;
        window = true;
      }
      else if(tapped_x >= 115 && tapped_y >= 30 && tapped_x <= 205 && tapped_y <= 70)
      {
        // 4K UHD
        width = 3840; height = 2160;
        window = currentRecordingFormat.windowedModeEnabled;
      }
      else if(tapped_x >= 210 && tapped_y >= 30 && tapped_x <= 310 && tapped_y <= 70)
      {
        // HD
        width = 1920; height = 1080;
        window = currentRecordingFormat.windowedModeEnabled;
      }

      // Windowed options
      else if(currentRes == "4K UHD" && tapped_x >= 20 && tapped_y >= 120 && tapped_x <= 110 && tapped_y <= 160)
      {
        // 4K UHD - Full Sensor
        width = currentRecordingFormat.width; height = currentRecordingFormat.height;
        window = false;
      }
      else if(currentRes == "4K UHD" && tapped_x >= 115 && tapped_y >= 120 && tapped_x <= 205 && tapped_y <= 160)
      {
        // 4K UHD - Windowed 5.7K
        width = currentRecordingFormat.width; height = currentRecordingFormat.height;
        window = true;
      }
      else if(currentRes == "HD" && tapped_x >= 20 && tapped_y >= 120 && tapped_x <= 110 && tapped_y <= 160)
      {
        // HD - Full Sensor
        width = currentRecordingFormat.width; height = currentRecordingFormat.height;
        window = false;
      }
      else if(currentRes == "HD" && tapped_x >= 115 && tapped_y >= 120 && tapped_x <= 310 && tapped_y <= 160)
      {
        // HD - Scaled from 5.7K or 2.8K, can't distinguish which unfortunately
        width = currentRecordingFormat.width; height = currentRecordingFormat.height;
        window = true;
      }

      if(width != 0)
      {
        // Resolution or Sensor Area selected, write to camera
        CCUPacketTypes::RecordingFormatData newRecordingFormat = currentRecordingFormat;
        newRecordingFormat.width = width;
        newRecordingFormat.height = height;
        newRecordingFormat.windowedModeEnabled = window;
        PacketWriter::writeRecordingFormat(newRecordingFormat, &cameraConnection);

        tappedAction = true;
      }
    }
  }

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen Resolution Pocket 6K Refreshed.");

  window.fillSprite(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW)
  {
    // Main label
    window.setTextColor(TFT_WHITE);
    window.drawString("BRAW RESOLUTION", 30, 9);

    String currentRes = currentRecordingFormat.frameDimensionsShort_string().c_str(); // "6K", "6K 2.4:1", "5.7K 17:9", "4K DCI", "3.7K 6:5A", "2.8K 17:9"

    // 6K
    String labelRes = "6K";
    window.fillSmoothRoundRect(20, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString(String(labelRes).c_str(), 65, 41);

    // 6K, 2.4:1
    labelRes = "6K 2.4:1";
    window.fillSmoothRoundRect(115, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("6K", 160, 36);
    window.drawCentreString("2.4:1", 160, 58, &Lato_Regular5pt7b);
  
    // 5.7K, 17:9
    labelRes = "5.7K 17:9";
    window.fillSmoothRoundRect(210, 30, 100, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("5.7K", 260, 36);
    window.drawCentreString("17:9", 260, 58, &Lato_Regular5pt7b);

    // 4K DCI
    labelRes = "4K DCI";
    window.fillSmoothRoundRect(20, 75, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("4K", 65, 82);
    window.drawCentreString("DCI", 65, 104, &Lato_Regular5pt7b);

    // 3.7K 6:5A
    labelRes = "3.7K 6:5A";
    window.fillSmoothRoundRect(115, 75, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("3.7K", 160, 82);
    window.drawCentreString("6:5 ANA", 160, 104, &Lato_Regular5pt7b);

    // 2.8K 17:9
    labelRes = "2.8K 17:9";
    window.fillSmoothRoundRect(210, 75, 100, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("2.8K", 260, 82);
    window.drawCentreString("17:9", 260, 104, &Lato_Regular5pt7b);

    // Sensor Area

    // Pocket 6K all are Windowed except 6K
    // window.drawSmoothRoundRect(20, 120, 5, 3, 290, 40, TFT_DARKGREY); // Optionally draw a rectangle around this info.
    window.drawCentreString(currentRes == "6K" ? "FULL SENSOR" :"SENSOR WINDOWED", 165, 129);
    window.drawCentreString(currentRecordingFormat.frameWidthHeight_string().c_str(), 165, 148, &Lato_Regular5pt7b);
  }
  else if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::ProRes)
  {
    // Main label
    window.setTextColor(TFT_WHITE);
    window.drawString("ProRes RESOLUTION", 30, 9);

    String currentRes = currentRecordingFormat.frameDimensionsShort_string().c_str(); // "4K DCI", "4K UHD", "HD"

    // 4K
    String labelRes = "4K DCI";
    window.fillSmoothRoundRect(20, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("4K", 65, 36);
    window.drawCentreString("DCI", 65, 58, &Lato_Regular5pt7b);

    // 4K UHD
    labelRes = "4K UHD";
    window.fillSmoothRoundRect(115, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("UHD", 160, 41);

    // HD
    labelRes = "HD";
    window.fillSmoothRoundRect(210, 30, 100, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    window.drawCentreString("HD", 260, 41);

    // Sensor Area

    // 4K DCI is Scaled from 5.7K, Ultra HD is Scaled from Full or 5.7K, HD is Scaled from Full, 5.7K, or 2.8K (however we can't change the 5.7K or 2.8K)

    window.drawString(currentRecordingFormat.frameWidthHeight_string().c_str(), 30, 90, &Lato_Regular5pt7b);
    window.drawString("SCALED FROM SENSOR AREA", 30, 105);

    if(currentRes == "4K DCI")
    {
      // Full sensor area only]
      window.fillSmoothRoundRect(20, 120, 90, 40, 3, TFT_DARKGREEN);
      window.drawCentreString("FULL", 65, 131);
    }
    else if(currentRes == "4K UHD")
    {
      // Scaled from Full or 5.7K
      window.fillSmoothRoundRect(20, 120, 90, 40, 3, (!currentRecordingFormat.windowedModeEnabled ? TFT_DARKGREEN : TFT_DARKGREY));
      window.drawCentreString("FULL", 65, 131);

      // 5.7K
      window.fillSmoothRoundRect(115, 120, 90, 40, 3, (currentRecordingFormat.windowedModeEnabled ? TFT_DARKGREEN : TFT_DARKGREY));
      window.drawCentreString("5.7K", 160, 131);
    }
    else
    {
      // HD

      // Scaled from Full, 2.8K or 5.7K (however we can't tell if it's 2.8K or 5.7K)
      window.fillSmoothRoundRect(20, 120, 90, 40, 3, (!currentRecordingFormat.windowedModeEnabled ? TFT_DARKGREEN : TFT_DARKGREY));
      window.drawCentreString("FULL", 65, 131);

      // 2.8K 5.7K
      window.fillSmoothRoundRect(115, 120, 195, 40, 3, (currentRecordingFormat.windowedModeEnabled ? TFT_DARKGREEN : TFT_DARKGREY));
      window.drawCentreString("5.7K / 2.8K", 212, 126);
      window.drawCentreString("CHECK/SET ON CAMERA", 212, 146, &Lato_Regular5pt7b);
    }
  }
  else
    DEBUG_ERROR("Resolution Pocket 6K - Codec not catered for.");

  window.pushSprite(0, 0);
}

// Resolution Screen for URSA Mini Pro G2
void Screen_ResolutionURSAMiniProG2(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Resolution;

  auto camera = BMDControlSystem::getInstance()->getCamera();
  
  // TO DO
}

// Resolution Screen for URSA Mini Pro 12K
void Screen_ResolutionURSAMiniPro12K(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Resolution;

  auto camera = BMDControlSystem::getInstance()->getCamera();
  
  // TO DO
}

// Resolution screen - redirects to appropriate screen fro camera
void Screen_Resolution(bool forceRefresh = false)
{
  auto camera = BMDControlSystem::getInstance()->getCamera();

  if(camera->hasCodec())
  {
    if(camera->hasRecordingFormat())
    {
      if(camera->isPocket4K())
        Screen_Resolution4K(forceRefresh); // Pocket 4K
      else if(camera->isPocket6K())
        Screen_Resolution6K(forceRefresh); // Pocket 6K
      else if(camera->isURSAMiniProG2())
        Screen_ResolutionURSAMiniProG2(forceRefresh); // URSA Mini Pro G2
      else if(camera->isURSAMiniPro12K())
        Screen_ResolutionURSAMiniPro12K(forceRefresh); // URSA Mini Pro 12K
      else
        DEBUG_DEBUG("No Codec screen for this camera.");
    } 
    else
      Screen_Resolution4K(forceRefresh); // Handle no model name in 4K screen
  }
  else
    Screen_Resolution4K(forceRefresh); // If we don't have any codec info, we show the 4K screen that shows no codec
}

// Media screen for Pocket 4K
void Screen_Media4K6K(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Media;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // 3 Media Slots - CFAST, SD, USB

  // If we have a tap, we should determine if it is on anything
  bool tappedAction = false;
  if(tapped_x != -1 && camera->getMediaSlots().size() != 0)
  {
    if(tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 315 && tapped_y <= 160)
    {
      // Tapped within the area
      if(tapped_x >= 20 && tapped_y >= 30 && tapped_x <= 315 && tapped_y <= 70)
      {
        // CFAST
        if(camera->getMediaSlots()[0].status != CCUPacketTypes::MediaStatus::None && !camera->getMediaSlots()[0].active)
        {
          TransportInfo transportInfo = camera->getTransportMode();
          transportInfo.slots[0].active = true;
          transportInfo.slots[1].active = false;
          transportInfo.slots[2].active = false;
          PacketWriter::writeTransportInfo(transportInfo, &cameraConnection);

          tappedAction = true;
        }
      }
      else if(tapped_x >= 20 && tapped_y >= 75 && tapped_x <= 315 && tapped_y <= 115)
      {
          // Change to SD
        if(camera->getMediaSlots()[1].status != CCUPacketTypes::MediaStatus::None && !camera->getMediaSlots()[1].active)
        {
          TransportInfo transportInfo = camera->getTransportMode();
          transportInfo.slots[0].active = false;
          transportInfo.slots[1].active = true;
          transportInfo.slots[2].active = false;
          PacketWriter::writeTransportInfo(transportInfo, &cameraConnection);

          tappedAction = true;
        }
      }
      else if(tapped_x >= 20 && tapped_y >= 120 && tapped_x <= 315 && tapped_y <= 160)
      {
          // Change to USB
        if(camera->getMediaSlots()[2].status != CCUPacketTypes::MediaStatus::None && !camera->getMediaSlots()[2].active)
        {
          TransportInfo transportInfo = camera->getTransportMode();
          transportInfo.slots[0].active = false;
          transportInfo.slots[1].active = false;
          transportInfo.slots[2].active = true;
          PacketWriter::writeTransportInfo(transportInfo, &cameraConnection);

          tappedAction = true;
        }
      }
    }
  }

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen Media Pocket 4K/6K Refreshed.");

  window.fillSprite(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

    // Media label
  window.setTextColor(TFT_WHITE);
  window.drawString("MEDIA", 30, 9, &AgencyFB_Bold9pt7b);

  // CFAST
  BMDCamera::MediaSlot cfast = camera->getMediaSlots()[0];
  window.fillSmoothRoundRect(20, 30, 295, 40, 3, (cfast.active ? TFT_DARKGREEN : TFT_DARKGREY));
  if(cfast.StatusIsError()) window.drawRoundRect(20, 30, 295, 40, 3, (cfast.active ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawString("CFAST", 28, 47);
  if(cfast.status != CCUPacketTypes::MediaStatus::None) window.drawString(cfast.remainingRecordTimeString.c_str(), 155, 47);

  if(cfast.status != CCUPacketTypes::MediaStatus::None) window.drawString("REMAINING TIME", 155, 35, &Lato_Regular5pt7b);
  window.drawString("1", 300, 35, &Lato_Regular5pt7b);
  window.drawString(cfast.GetStatusString().c_str(), 28, 35, &Lato_Regular5pt7b);

  // SD
  BMDCamera::MediaSlot sd = camera->getMediaSlots()[1];
  window.fillSmoothRoundRect(20, 75, 295, 40, 3, (sd.active ? TFT_DARKGREEN : TFT_DARKGREY));
  if(sd.StatusIsError()) window.drawRoundRect(20, 75, 295, 40, 3, (sd.active ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawString("SD", 28, 92);
  if(sd.status != CCUPacketTypes::MediaStatus::None) window.drawString(sd.remainingRecordTimeString.c_str(), 155, 92);

  if(sd.status != CCUPacketTypes::MediaStatus::None) window.drawString("REMAINING TIME", 155, 80, &Lato_Regular5pt7b);
  window.drawString("2", 300, 80, &Lato_Regular5pt7b);
  window.drawString(sd.GetStatusString().c_str(), 28, 80, &Lato_Regular5pt7b);
  if(sd.StatusIsError()) window.setTextColor(TFT_WHITE);

  // USB
  BMDCamera::MediaSlot usb = camera->getMediaSlots()[2];
  window.fillSmoothRoundRect(20, 120, 295, 40, 3, (usb.active ? TFT_DARKGREEN : TFT_DARKGREY));
  if(usb.StatusIsError()) window.drawRoundRect(20, 120, 295, 40, 3, (usb.active ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawString("USB", 28, 137);
  if(usb.status != CCUPacketTypes::MediaStatus::None) window.drawString(usb.remainingRecordTimeString.c_str(), 155, 137);
  
  if(usb.status != CCUPacketTypes::MediaStatus::None) window.drawString("REMAINING TIME", 155, 125, &Lato_Regular5pt7b);
  window.drawString("3", 300, 125, &Lato_Regular5pt7b);
  window.drawString(usb.GetStatusString().c_str(), 28, 125, &Lato_Regular5pt7b);
  if(usb.StatusIsError()) window.setTextColor(TFT_WHITE);

  window.pushSprite(0, 0);
}

// Media Screen for URSA Mini Pro G2
void Screen_MediaURSAMiniProG2(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Media;

  auto camera = BMDControlSystem::getInstance()->getCamera();
  
  // TO DO
}

// Media Screen for URSA Mini Pro 12K
void Screen_MediaURSAMiniPro12K(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Media;

  auto camera = BMDControlSystem::getInstance()->getCamera();
  
  // TO DO
}


// Media screen - redirects to appropriate screen for camera
void Screen_Media(bool forceRefresh = false)
{
  auto camera = BMDControlSystem::getInstance()->getCamera();

  if(camera->getMediaSlots().size() != 0)
  {
    if(camera->hasModelName())
    {
      if(camera->isPocket4K6K())
        Screen_Media4K6K(forceRefresh); // Pocket camera
      else if(camera->isURSAMiniProG2())
        Screen_MediaURSAMiniProG2(forceRefresh); // URSA Mini Pro G2
      else if(camera->isURSAMiniPro12K())
        Screen_MediaURSAMiniPro12K(forceRefresh); // URSA Mini Pro 12K
      else
        DEBUG_DEBUG("No Media screen for this camera.");
    } 
    else
      Screen_Media4K6K(forceRefresh); // Handle no model name in 4K/6K screen
  }
  else
    Screen_Media4K6K(forceRefresh); // If we don't have any media info, we show the 4K/6K screen that shows no media

}

void Screen_Lens(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Lens;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // If we have a tap, we should determine if it is on anything
  bool tappedAction = false;
  if(tapped_x != -1 && camera->hasTransportMode())
  {
    if(tapped_x >= 200 && tapped_y <= 120)
    {
      // Focus button
      PacketWriter::writeAutoFocus(&cameraConnection);

      DEBUG_DEBUG("Instantaneous Autofocus");
      
      tappedAction = true;
    }
  }

    // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();

  DEBUG_DEBUG("Screen Lens Refreshed.");

  window.fillSprite(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // M5GFX, set font here rather than on each drawString line
  window.setFont(&Lato_Regular11pt7b);

  // Focus button
  window.drawCircle(257, 63, 57, TFT_BLUE); // Outer
  window.fillSmoothCircle(257, 63, 54, TFT_SKYBLUE); // Inner
  window.setTextColor(TFT_WHITE);
  window.drawCentreString("FOCUS", 257, 50);

  if(camera->hasFocalLengthMM())
  {
    auto focalLength = camera->getFocalLengthMM();
    std::string focalLengthMM = std::to_string(focalLength);
    std::string combined = focalLengthMM + "mm";

    window.drawString(combined.c_str(), 30, 25, &Lato_Regular12pt7b);
  }

  if(camera->hasApertureFStopString())
  {
      window.drawString(camera->getApertureFStopString().c_str(), 30, 50, &Lato_Regular12pt7b);
  }

  window.pushSprite(0, 0);
}


void setup() {

  // Allow a timeout of 20 seconds for time for the pass key entry.
  esp_task_wdt_init(20, true);

  // M5CoreS3 Demo
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
  }

  USBSerial.begin(15200);
  M5.begin();

  USBSerial.printf("M5CoreS3 User Demo, Version: %s\r\n", DEMO_VERSION);

  // BM8563 Init (clear INT)
  M5.In_I2C.writeRegister8(0x51, 0x00, 0x00, 100000L);
  M5.In_I2C.writeRegister8(0x51, 0x01, 0x00, 100000L);
  M5.In_I2C.writeRegister8(0x51, 0x0D, 0x00, 100000L);

  // AW9523 Control BOOST
  M5.In_I2C.bitOn(AW9523_ADDR, 0x03, 0b10000000, 100000L);  // BOOST_EN

  M5.Display.setBrightness(100);

  // MS
  M5.Display.setSwapBytes(true);

  // SET DEBUG LEVEL
  Debug.setDebugOutputStream(&USBSerial); // M5CoreS3 uses this serial output
  Debug.setDebugLevel(DBG_VERBOSE);
  Debug.timestampOn();

  window.createSprite(IWIDTH, IHEIGHT);

  // Load sprites
  spriteMPCSplash.createSprite(IWIDTH, IHEIGHT);
  spriteMPCSplash.setSwapBytes(true);
  spriteMPCSplash.pushImage(0, 0, IWIDTH, IHEIGHT, MPCSplash_M5Stack_CoreS3);

  // Images, store them in sprites ready to be used when we need them
  spriteBluetooth.createSprite(30, 46);
  spriteBluetooth.setSwapBytes(true);
  spriteBluetooth.pushImage(0, 0, 30, 46, Wikipedia_Bluetooth_30x46);

  spritePocket4k.createSprite(110, 61);
  spritePocket4k.setSwapBytes(true);
  spritePocket4k.pushImage(0, 0, 110, 61, blackmagic_pocket_4k_110x61);

  // White Balance images
  spriteWBBright.createSprite(30, 30);
  spriteWBBright.setSwapBytes(true);
  spriteWBBright.pushImage(0, 0, 30, 30, WBBright);

  spriteWBCloud.createSprite(30, 30);
  spriteWBCloud.setSwapBytes(true);
  spriteWBCloud.pushImage(0, 0, 30, 30, WBCloud);

  spriteWBFlourescent.createSprite(30, 30);
  spriteWBFlourescent.setSwapBytes(true);
  spriteWBFlourescent.pushImage(0, 0, 30, 30, WBFlourescent);

  spriteWBIncandescent.createSprite(30, 30);
  spriteWBIncandescent.setSwapBytes(true);
  spriteWBIncandescent.pushImage(0, 0, 30, 30, WBIncandescent);

  spriteWBMixedLight.createSprite(30, 30);
  spriteWBMixedLight.setSwapBytes(true);
  spriteWBMixedLight.pushImage(0, 0, 30, 30, WBMixedLight);

  spriteWBBrightBG.createSprite(30, 30);
  spriteWBBrightBG.setSwapBytes(true);
  spriteWBBrightBG.pushImage(0, 0, 30, 30, WBBrightBG);

  spriteWBCloudBG.createSprite(30, 30);
  spriteWBCloudBG.setSwapBytes(true);
  spriteWBCloudBG.pushImage(0, 0, 30, 30, WBCloudBG);

  spriteWBFlourescentBG.createSprite(30, 30);
  spriteWBFlourescentBG.setSwapBytes(true);
  spriteWBFlourescentBG.pushImage(0, 0, 30, 30, WBFlourescentBG);

  spriteWBIncandescentBG.createSprite(30, 30);
  spriteWBIncandescentBG.setSwapBytes(true);
  spriteWBIncandescentBG.pushImage(0, 0, 30, 30, WBIncandescentBG);

  spriteWBMixedLightBG.createSprite(30, 30);
  spriteWBMixedLightBG.setSwapBytes(true);
  spriteWBMixedLightBG.pushImage(0, 0, 30, 30, WBMixedLightBG);

  spritePassKey.createSprite(IWIDTH, IHEIGHT);

  // Splash screen
  M5.Display.pushImage(0,0,320,240,MPCSplash_M5Stack_CoreS3);

  // Set main font
  window.setFont(&Lato_Regular11pt7b);

  // Get and pass pointer to touch object specifically for passkey entry
  lgfx::v1::ITouch* touch = M5.Display.panel()->getTouch();

  // Prepare for Bluetooth connections and start scanning for cameras
  cameraConnection.initialise(touch, &window, &spritePassKey, IWIDTH, IHEIGHT); // Screen Pass Key entry
}

int memoryLoopCounter;

void loop() {

  memoryLoopCounter++;

  static unsigned long lastConnectedTime = 0;
  const unsigned long reconnectInterval = 5000;  // 5 seconds (milliseconds)

  unsigned long currentTime = millis();

  if (cameraConnection.status == BMDCameraConnection::ConnectionStatus::Disconnected && currentTime - lastConnectedTime >= reconnectInterval) {
    DEBUG_VERBOSE("Disconnected for too long, trying to reconnect");

    // Set the status to Scanning and then show the NoConnection screen to render the Scanning page before starting the scan (which blocks so it can't render the Scanning page before it finishes)
    cameraConnection.status = BMDCameraConnection::ConnectionStatus::Scanning;
    Screen_NoConnection();

    cameraConnection.scan();

    if(connectedScreenIndex != Screens::NoConnection) // Move to the No Connection screen if we're not on it
      Screen_NoConnection();
  }
  else if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::Connected)
  {
    auto camera = BMDControlSystem::getInstance()->getCamera();

    if(static_cast<byte>(connectedScreenIndex) >= 100)
    {
      // Check if the initial payload has been fully received and if it was after the camera's last modified time, update the camera's modified time
      if(cameraConnection.getInitialPayloadTime() != ULONG_MAX && cameraConnection.getInitialPayloadTime() > camera->getLastModified())
        camera->setLastModified();

      switch(connectedScreenIndex)
      {
        case Screens::Dashboard:
          Screen_Dashboard();
          break;
        case Screens::Recording:
          Screen_Recording();
          break;
        case Screens::ISO:
          Screen_ISO();
          break;
        case Screens::ShutterAngleSpeed:
          if(camera->shutterValueIsAngle)
            Screen_ShutterAngle();
          else
            Screen_ShutterSpeed();
          break;
        case Screens::WhiteBalanceTint:
          Screen_WBTint();
          break;
        case Screens::Codec:
          Screen_Codec();
          break;
        case Screens::Resolution:
          Screen_Resolution();
          break;
        case Screens::Media:
          Screen_Media();
          break;
        case Screens::Lens:
          Screen_Lens();
          break;
      }
    }
    else
      Screen_Dashboard(true); // Was on disconnected screen, now we're connected go to the dashboard

    lastConnectedTime = currentTime;
  }
  else if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::ScanningFound)
  {
    Screen_NoConnection();

    lastConnectedTime = currentTime;
  }
  else if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::ScanningNoneFound)
  {
    DEBUG_VERBOSE("Status Scanning NONE Found. Marking as Disconnected.");
    cameraConnection.status = BMDCameraConnection::Disconnected;
    lastConnectedTime = currentTime;

    Screen_NoConnection();
  }
  else if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::FailedPassKey)
  {
    // Pass Key failed, so show it and then delay for 2 seconds and reset status to disconnected.
    Screen_NoConnection();

    delay(2000);

    cameraConnection.status == BMDCameraConnection::ConnectionStatus::Disconnected;
  }

  // Reset tapped point
  tapped_x = -1;
  tapped_y = -1;

  // Touch
  lgfx::touch_point_t tp[3];
  int nums = M5.Display.getTouchRaw(tp, 3);

  // Is there a touch event available?
  if (nums)
  {
    for (int i = 0; i < nums; ++i)
    {
      // Save the tapped position for the screens to pick up and process
      tapped_x = tp[i].x;
      tapped_y = tp[i].y;

      // Uncomment these lines to show the coordinates of the tap on screen
      // M5.Display.setCursor(16, 16 + i * 24);
      // M5.Display.printf("Raw X:%03d  Y:%03d", tp[i].x, tp[i].y);

      break; // Taking the first finger only and taps (not gestures)
    }

    // If they have tapped on the bottom left button area, then take them to the dashboard
    if(tapped_x > 0 && tapped_x <= 100 && tapped_y > 220)
    {
        connectedScreenIndex = Screens::Dashboard;
        lastRefreshedScreen = 0; // Forces a refresh
    }
    else if(tapped_x > 100 && tapped_x <= 220 && tapped_y > 220) // Recording page
    {
        connectedScreenIndex = Screens::Recording;
        lastRefreshedScreen = 0; // Forces a refresh
    }
    else if(tapped_x > 220 && tapped_x <= 320 && tapped_y > 220) // Codec Page
    {
        connectedScreenIndex = Screens::Codec;
        lastRefreshedScreen = 0; // Forces a refresh
    }
  }

  delay(5);
}