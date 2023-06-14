
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


// Screen width and height
#define IWIDTH 320
#define IHEIGHT 240

const float buttonFontSizeLarge = 2;
const float buttonFontSizeNormal = 1;
const float buttonFontSizeSmall = 0.5;

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
      window.setTextSize(buttonFontSizeSmall);
      window.setTextColor(TFT_WHITE);
      window.drawCenterString("DASHBOARD", 70, 220);

      if(camera->isRecording)
        window.fillSmoothCircle(IWIDTH / 2, 240, 30, TFT_RED);
      else
        window.drawCircle(IWIDTH / 2, 240, 30, TFT_RED);

      window.fillSmoothRoundRect(210, 210, 80, 40, 3, TFT_DARKCYAN);
      window.drawCenterString("CODEC", 250, 220);

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

  // spriteMPCSplash.pushToSprite(&window, 0, 0);
  // M5.Display.pushImage(0,0,320,240,MPCSplash_M5Stack_CoreS3);

  // window.createSprite(IWIDTH, IHEIGHT);

  spriteMPCSplash.pushSprite(&window, 0, 0);

  // Black background for text and Bluetooth Logo
  // window.fillRect(0, 3, IWIDTH, 51, TFT_BLACK);
  // M5.Display.fillRect(0, 3, IWIDTH, 51, TFT_BLACK);
  window.fillRect(0, 3, IWIDTH, 51, TFT_BLACK);

  // Bluetooth Image
  // spriteBluetooth.pushToSprite(&window, 26, 6);
  // M5.Display.pushImage(26, 6, 30, 46, Wikipedia_Bluetooth_30x46);
  spriteBluetooth.pushSprite(&window, 26, 6);

  // M5.Display.setTextSize(2);
  window.setTextSize(buttonFontSizeNormal);

  switch(cameraConnection.status)
  {
    case BMDCameraConnection::Scanning:
      Screen_Common(TFT_BLUE); // Common elements
      // M5.Display.drawString("Scanning...", 70, 20);
      window.drawString("Scanning...", 70, 20);
      break;
    case BMDCameraConnection::ScanningFound:
      Screen_Common(TFT_BLUE); // Common elements
      if(cameraConnection.cameraAddresses.size() == 1)
      {
        // M5.Display.drawString("Found, connecting...", 70, 20);
        window.drawString("Found, connecting...", 70, 20);
        connectToCameraIndex = 0;
      }
      else
        // M5.Display.drawString("Found cameras", 70, 20); // Multiple camera selection is below
        window.drawString("Found cameras", 70, 20); // Multiple camera selection is below
      break;
    case BMDCameraConnection::ScanningNoneFound:
      Screen_Common(TFT_RED); // Common elements
      // M5.Display.drawString("No camera found", 70, 20);
      window.drawString("No camera found", 70, 20);
      break;
    case BMDCameraConnection::Connecting:
      Screen_Common(TFT_YELLOW); // Common elements
      // M5.Display.drawString("Connecting...", 70, 20);
      window.drawString("Connecting...", 70, 20);
      break;
    case BMDCameraConnection::NeedPassKey:
      Screen_Common(TFT_PURPLE); // Common elements
      // M5.Display.drawString("Need Pass Key", 70, 20);
      window.drawString("Need Pass Key", 70, 20);
      break;
    case BMDCameraConnection::FailedPassKey:
      Screen_Common(TFT_ORANGE); // Common elements
      // M5.Display.drawString("Wrong Pass Key", 70, 20);
      window.drawString("Wrong Pass Key", 70, 20);
      break;
    case BMDCameraConnection::Disconnected:
      Screen_Common(TFT_RED); // Common elements
      // M5.Display.drawString("Disconnected (wait)", 70, 20);
      window.drawString("Disconnected (wait)", 70, 20);
      break;
    case BMDCameraConnection::IncompatibleProtocol:
      // Note: This needs to be worked on as there's no incompatible protocol connections yet.
      Screen_Common(TFT_RED); // Common elements
      // M5.Display.drawString("Incompatible Protocol", 70, 20);
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
    // M5.Display.fillRoundRect(25 + (count * 125) + (count * 10), 60, 125, 100, 5, TFT_DARKGREY);
    window.fillRoundRect(25 + (count * 125) + (count * 10), 60, 125, 100, 5, TFT_DARKGREY);

    // Highlight the camera to connect to
    if(connectToCameraIndex != -1 && connectToCameraIndex == count)
    {
      // M5.Display.drawRoundRect(25 + (count * 125) + (count * 10), 60, 5, 2, TFT_GREEN);
      window.drawRoundRect(25 + (count * 125) + (count * 10), 60, 5, 2, TFT_GREEN);
    }

    // spritePocket4k.pushToSprite(&window, 33 + (count * 125) + (count * 10), 69);
    // M5.Display.pushImage(33 + (count * 125) + (count * 10), 69, 110, 61, blackmagic_pocket_4k_110x61);
    spritePocket4k.pushSprite(&window, 33 + (count * 125) + (count * 10), 69);

    // M5.Display.setTextSize(1);
    window.setTextSize(buttonFontSizeNormal);
    // window.textbgcolor = TFT_DARKGREY;
    // M5.Display.drawString(cameraConnection.cameraAddresses[count].toString().c_str(), 33 + (count * 125) + (count * 10), 144);
    window.setTextSize(buttonFontSizeSmall);
    window.drawString(cameraConnection.cameraAddresses[count].toString().c_str(), 33 + (count * 125) + (count * 10), 144);
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

    if(tapped_x >= 20 && tapped_y >= 5 && tapped_x <= 315 && tapped_y <= 160)
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
  window.setFont(&fonts::FreeMono12pt7b);

  // ISO
  if(camera->hasSensorGainISOValue())
  {
    window.fillSmoothRoundRect(20, 5, 75, 65, 3, TFT_DARKGREY);
    window.setTextSize(buttonFontSizeNormal);
    window.setTextColor(TFT_WHITE);
    // window.textbgcolor = TFT_DARKGREY;

    window.drawCentreString(String(camera->getSensorGainISOValue()), 58, 28);

    window.setTextSize(buttonFontSizeSmall);
    window.drawCentreString("ISO", 58, 59);
  }

  // Shutter
  xshift = 80;
  if(camera->hasShutterAngle())
  {
    DEBUG_DEBUG("Dashboard: Shutter Angle");

    window.fillSmoothRoundRect(20 + xshift, 5, 75, 65, 3, TFT_DARKGREY);
    window.setTextSize(buttonFontSizeNormal);
    window.setTextColor(TFT_WHITE);
    // window.textbgcolor = TFT_DARKGREY;

    if(camera->shutterValueIsAngle)
    {
      // Shutter Angle
      int currentShutterAngle = camera->getShutterAngle();
      float ShutterAngleFloat = currentShutterAngle / 100.0;

      window.drawCentreString(String(ShutterAngleFloat, (currentShutterAngle % 100 == 0 ? 0 : 1)), 58 + xshift, 28);
    }
    else
    {
      DEBUG_DEBUG("Dashboard: Shutter Speed");

      // Shutter Speed
      int currentShutterSpeed = camera->getShutterSpeed();

      window.drawCentreString("1/" + String(currentShutterSpeed), 58 + xshift, 28);
    }

    window.setTextSize(buttonFontSizeSmall);
    window.drawCentreString(camera->shutterValueIsAngle ? "DEGREES" : "SPEED", 58 + xshift, 59); //  "SHUTTER"
  }

  // WhiteBalance and Tint
  xshift += 80;
  if(camera->hasWhiteBalance() || camera->hasTint())
  {
    DEBUG_DEBUG("Dashboard: White Balance");

    window.fillSmoothRoundRect(20 + xshift, 5, 135, 65, 3, TFT_DARKGREY);
    window.setTextSize(buttonFontSizeNormal);
    window.setTextColor(TFT_WHITE);
    // window.textbgcolor = TFT_DARKGREY;

    if(camera->hasWhiteBalance())
      window.drawCentreString(String(camera->getWhiteBalance()), 58 + xshift, 28);

    window.setTextSize(buttonFontSizeSmall);
    window.drawCentreString("WB", 58 + xshift, 59);

    xshift += 66;

    window.setTextSize(buttonFontSizeNormal);
    if(camera->hasTint())
      window.drawCentreString(String(camera->getTint()), 58 + xshift, 28);

    window.setTextSize(buttonFontSizeSmall);
    window.drawCentreString("TINT", 58 + xshift, 59);
  }

  // Codec
  if(camera->hasCodec())
  {
    window.fillSmoothRoundRect(20, 75, 155, 40, 3, TFT_DARKGREY);

    window.setTextSize(buttonFontSizeNormal);
    window.drawCentreString(camera->getCodec().to_string().c_str(), 97, 87);
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

      window.setTextSize(buttonFontSizeNormal);
      window.drawCentreString(slotString.c_str(), 70, 133);

      // Show recording error
      if(camera->hasRecordError())
        window.drawRoundRect(20, 120, 100, 40, 3, TFT_RED);
    }
    else
    {
      // Show no Media
      window.fillSmoothRoundRect(20, 120, 100, 40, 3, TFT_DARKGREY);
      window.setTextSize(buttonFontSizeSmall);
      window.drawCentreString("NO MEDIA", 70, 135);
    }
  }

  // Recording Format - Frame Rate and Resolution
  if(camera->hasRecordingFormat())
  {
    // Frame Rate
    window.fillSmoothRoundRect(180, 75, 135, 40, 3, TFT_DARKGREY);
    window.setTextSize(buttonFontSizeNormal);

    window.drawCentreString(camera->getRecordingFormat().frameRate_string().c_str(), 237, 87);

    window.setTextSize(buttonFontSizeSmall);
    window.drawCentreString("fps", 285, 97);

    // Resolution
    window.fillSmoothRoundRect(125, 120, 190, 40, 3, TFT_DARKGREY);

    std::string resolution = camera->getRecordingFormat().frameDimensionsShort_string();
    window.setTextSize(buttonFontSizeNormal);
    window.drawCentreString(resolution.c_str(), 220, 133);
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
  window.setFont(&fonts::FreeMono12pt7b);

  // Record button
  if(camera->isRecording) window.fillSmoothCircle(257, 63, 58, Constants::DARK_RED); // Recording solid
  window.drawCircle(257, 63, 57, (camera->isRecording ? TFT_RED : TFT_LIGHTGREY)); // Outer
  window.fillSmoothCircle(257, 63, 38, camera->isRecording ? TFT_RED : TFT_LIGHTGREY); // Inner

  // Timecode
  window.setTextSize(buttonFontSizeNormal);
  window.setTextColor(camera->isRecording ? TFT_RED : TFT_WHITE);
  // window.textbgcolor = TFT_BLACK;
  window.drawString(camera->getTimecodeString().c_str(), 30, 57);

  // Remaining time and any errors
  if(camera->getMediaSlots().size() != 0 && camera->hasActiveMediaSlot())
  {
    window.setTextColor(TFT_LIGHTGREY);
    window.drawString((camera->getActiveMediaSlot().GetMediumString() + " " + camera->getActiveMediaSlot().remainingRecordTimeString).c_str(), 30, 130);

    window.setTextSize(buttonFontSizeSmall);
    window.drawString("REMAINING TIME", 30, 153);

    // Show any media record errors
    if(camera->hasRecordError())
    {
      window.setTextSize(buttonFontSizeNormal);
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
  window.setFont(&fonts::FreeMono12pt7b);

  // Get the current ISO value
  int currentISO = 0;
  if(camera->hasSensorGainISOValue())
    currentISO = camera->getSensorGainISOValue();

  // ISO label
  window.setTextSize(buttonFontSizeNormal);
  window.setTextColor(TFT_WHITE);
  // window.textbgcolor = TFT_BLACK;
  window.drawString("ISO", 30, 9);

  // window.textbgcolor = TFT_DARKGREY;

  // 200
  int labelISO = 200;
  window.setTextSize(buttonFontSizeNormal);
  window.fillSmoothRoundRect(20, 30, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  // if(currentISO == labelISO) window.textbgcolor = TFT_DARKGREEN; else window.textbgcolor = TFT_DARKGREY;
  window.drawCentreString(String(labelISO).c_str(), 65, 41);

  // 400
  labelISO = 400;
  window.fillSmoothRoundRect(115, 30, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  // if(currentISO == labelISO) window.textbgcolor = TFT_DARKGREEN; else window.textbgcolor = TFT_DARKGREY;
  window.drawCentreString(String(labelISO).c_str(), 160, 36);
  window.setTextSize(buttonFontSizeSmall);
  window.drawCentreString("NATIVE", 160, 59);
  window.setTextSize(buttonFontSizeNormal);

  // 8000
  labelISO = 8000;
  window.fillSmoothRoundRect(210, 30, 100, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  // if(currentISO == labelISO) window.textbgcolor = TFT_DARKGREEN; else window.textbgcolor = TFT_DARKGREY;
  window.drawCentreString(String(labelISO).c_str(), 260, 41);

  // 640
  labelISO = 640;
  window.fillSmoothRoundRect(20, 75, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  // if(currentISO == labelISO) window.textbgcolor = TFT_DARKGREEN; else window.textbgcolor = TFT_DARKGREY;
  window.drawCentreString(String(labelISO).c_str(), 65, 87);

  // 800
  labelISO = 800;
  window.fillSmoothRoundRect(115, 75, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  // if(currentISO == labelISO) window.textbgcolor = TFT_DARKGREEN; else window.textbgcolor = TFT_DARKGREY;
  window.drawCentreString(String(labelISO).c_str(), 160, 87);

  // 12800
  labelISO = 12800;
  window.fillSmoothRoundRect(210, 75, 100, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  // if(currentISO == labelISO) window.textbgcolor = TFT_DARKGREEN; else window.textbgcolor = TFT_DARKGREY;
  window.drawCentreString(String(labelISO).c_str(), 260, 87);

  // 1250
  labelISO = 1250;
  window.fillSmoothRoundRect(20, 120, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  // if(currentISO == labelISO) window.textbgcolor = TFT_DARKGREEN; else window.textbgcolor = TFT_DARKGREY;
  window.drawCentreString(String(labelISO).c_str(), 65, 131);

  // 3200
  labelISO = 3200;
  window.fillSmoothRoundRect(115, 120, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  // if(currentISO == labelISO) window.textbgcolor = TFT_DARKGREEN; else window.textbgcolor = TFT_DARKGREY;
  window.drawCentreString(String(labelISO).c_str(), 160, 126);
  window.setTextSize(buttonFontSizeSmall);
  window.drawCentreString("NATIVE", 160, 149);
  window.setTextSize(buttonFontSizeNormal);

  // Custom ISO - show if ISO is not one of the above values
  if(currentISO != 0)
  {
    // Only show the ISO value if it's not a standard one
    if(currentISO != 200 && currentISO != 400 && currentISO != 640 && currentISO != 800 && currentISO != 1250 && currentISO != 3200 && currentISO != 8000 && currentISO != 12800)
    {
      window.fillSmoothRoundRect(210, 120, 100, 40, 3, TFT_DARKGREEN);
      // window.textbgcolor = TFT_DARKGREEN;
      window.drawCentreString(String(currentISO).c_str(), 260, 126);
      window.setTextSize(buttonFontSizeSmall);
      window.drawCentreString("CUSTOM", 260, 149);
    }
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
  window.setFont(&fonts::FreeMono12pt7b);

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
          /*
          if(camera->shutterValueIsAngle)
            Screen_ShutterAngle();
          else
            Screen_ShutterSpeed();
          */
          break;
        case Screens::WhiteBalanceTint:
          // Screen_WBTint();
          break;
        case Screens::Codec:
          // Screen_Codec();
          break;
        case Screens::Resolution:
          // Screen_Resolution();
          break;
        case Screens::Media:
          // Screen_Media();
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

  /*Change the active screen's background color*/
  // lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x6C3483), LV_PART_MAIN);

  /*Create a white label, set its text and align it to the center*/
  // lv_obj_t * label = lv_label_create(lv_scr_act());
  // lv_label_set_text(label, "Hello world");
  // lv_obj_set_style_text_color(lv_scr_act(), lv_color_hex(0xffffff), LV_PART_MAIN);
  // lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

  /*
  M5.Display.drawString("Hello there.", 100, 100);

  M5.Display.setColor(0xFF0000U);                        // 描画色に赤色を指定
  M5.Display.fillCircle ( 40, 80, 20    );               // 赤色で円の塗り
  M5.Display.fillEllipse( 80, 40, 10, 20);               // 赤色で楕円の塗り
  M5.Display.fillArc    ( 80, 80, 20, 10, 0, 90);        // 赤色で円弧の塗り
  M5.Display.fillTriangle(80, 80, 60, 80, 80, 60);       // 赤色で三角の塗り
  M5.Display.setColor(0x0000FFU);                        // 描画色に青色を指定
  M5.Display.drawCircle ( 40, 80, 20    );               // 青色で円の外周
  M5.Display.drawEllipse( 80, 40, 10, 20);               // 青色で楕円の外周
  M5.Display.drawArc    ( 80, 80, 20, 10, 0, 90);        // 青色で円弧の外周
  M5.Display.drawTriangle(60, 80, 80, 80, 80, 60);       // 青色で三角の外周
  M5.Display.setColor(0x00FF00U);                        // 描画色に緑色を指定
  M5.Display.drawBezier( 60, 80, 80, 80, 80, 60);        // 緑色で二次ベジエ曲線
  M5.Display.drawBezier( 60, 80, 80, 20, 20, 80, 80, 60);// 緑色で三次ベジエ曲線

// グラデーションの線を描画するdrawGradientLine は色の指定を省略できません。
  M5.Display.drawGradientLine( 0, 80, 80, 0, 0xFF0000U, 0x0000FFU);// 赤から青へのグラデーション直線
*/




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

      DEBUG_DEBUG("Main Loop - Screen Tapped");

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
  }

  /*
  if(memoryLoopCounter % 100 == 0)
    USBSerial.println("->>>");
  */

  delay(10);
}