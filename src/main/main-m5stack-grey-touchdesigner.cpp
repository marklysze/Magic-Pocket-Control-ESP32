// TouchDesigner specific code.
// How to use it:
// 1. Rename the existing main-m5stack-grey.cpp file to main-m5stack-grey-original.cpp and rename this file to "main-m5stack-grey.cpp"
// 2. TouchDesigner connects over serial and the buttons are emulated by sending a character to the device: 'A' = Button A, 'B' = Button B, 'C' = Button C
// 3. You'll need to have the devices paired first with Bluetooth (as entry of the pincode can't be done over serial in this code)
// Good luck!

// TouchDesigner instructions:
// Send a string through serial with a terminator value of "\n"
// E.g. to send "B" on serial1 the command is: op('serial1').send("B", terminator="\n")
// E.g. op('serial1').send("(ISO:2000)", terminator="\n")
// Commands must start with a "(", have a ":" to separate the command and the value, and to end with a ")"
// Commands are not case-sensitive (e.g. "ISO" will work and "iSo" will also work)
// 
// Examples:
// 
// (ISO:1600)
// (ISO:999)
// (CODEC:BRAW)
//
// Available commands are - don't forget to put them in () brackets:
// 
// BUTTON:A   // same as pressing the A button on the M5Stack
// BUTTON:B
// BUTTON:C
// RECORD:START
// RECORD:STOP
// ISO:100 up to ISO:25600
// SHUTTERANGLE:1 up to SHUTTERANGLE:360
// SHUTTERSPEED:24 up to SHUTTERSPEED:2000
// WHITEBALANCE:2500 up to WHITEBALANCE:10000
// TINT:-50 up to TINT:50
// BRAWQ:0    // Q0
// BRAWQ:3    // Q3
// BRAWQ:5    // Q5
// BRAWQ:8    // Q8
// BRAWB:3    // 3:1
// BRAWB:5    // 5:1
// BRAWB:8    // 8:1
// BRAWB:12   // 12:1
// FPS:23.98
// FPS:29.97
// FPS:59.94
// FPS:24 or 25 or 30 or 50 or 60 (note > 50 only available on the 6K if it's not in full 6K)
// IRIS:2.8 or any aperture value that the lens can take up to 16 (can't pass a value greater than 16)
//
// Want to create your own commands and actions - see the function "RunTouchDesignerCommand" in this file


#define USING_TFT_ESPI 0        // Not using the TFT_eSPI graphics library <-- must include this in every main file, 0 = not using, 1 = using
#define USING_M5GFX 0           // Using the M5GFX library with touch screen
#define USING_M5_BUTTONS 1   // Using the M5GFX library with the 3 buttons (buttons A, B, C)

#define OUTPUT_CAMERA_SETTINGS 1  // 1 = Outputs camera settings through serial (so other applications can read them)

// The output format is: >>[state]:[state value]
// Here are some examples:
/*
>>LensType:Canon EF-S 18-55mm f/3.5-5.6
>>LensIris:f3.5
>>FocalLengthMM:18mm
>>LensDistance:460mm to 590mm
>>ISO:3200
>>WhiteBalance:6500
>>Tint:10
>>Aperture:f3.5
>>ApertureNormalised:0
>>HasLens:Yes
>>FocalLengthMM:18
>>ModelName:Pocket Cinema Camera 6K
>>IsPocketCamera:Yes
>>FrameRate:25
>>FrameDims:4096 x 2160
>>FrameSize:4K DCI
>>ShutterAngle:6000
>>Codec:BRAW 12:1
*/

#include <ESP32-Chimera-Core.h>

#define tft M5.Lcd
static LGFX_Sprite *sprite;

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

// Include the watchdog library so we can stop it timing out while pass key entry.
#include "esp_task_wdt.h"

// TouchDesigner
std::string tdCommand = "";
bool tdCommandComplete = false;
int tdMaxLength = 50; // Accept up to 50 characters, including '(', ':', and ')'

// Lato font from Google Fonts
// Agency FB font is free for commercial use, copied from Windows fonts
// Rather than using font sizes, we use specific fonts for each size as it renders better on screen
#include "Fonts/Lato_Regular12pt7b.h" // Slightly larger version
#include "Fonts/Lato_Regular11pt7b.h" // Standard font
#include "Fonts/Lato_Regular6pt7b.h" // Small version for camera address
#include "Fonts/Lato_Regular5pt7b.h" // Smallest version
#include "Fonts/AgencyFB_Regular7pt7b.h" // Agency FB for tiny text
#include "Fonts/AgencyFB_Bold9pt7b.h" // Agency FB small for above buttons
#include "Fonts/AgencyFB_Regular9pt7b.h" // Agency FB small-medium for above buttons

// Screen width and height
#define IWIDTH 320
#define IHEIGHT 240

// Sprite width and height (limited by the lack of PSRAM on this device, Set to 8bpp, instead of 16bpp)
// We'll draw the side bar, the button labels, and the recording rectangle separately to the sprite
// Note: If you try to use a sprite that takes too much memory it may not show at all or Bluetooth will not connect properly
#define IWIDTH_SPRITE 320 // 300
#define IHEIGHT_SPRITE 240 // 208
#define BPP_SPRITE 8

// Sprites for Images
// Not using sprites, writing directly to M5.Display (aka "window")
// This causes flashing on the display, so we need to address this
/*
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
*/

// Images
#include "Images/MPCSplash-M5Stack-CoreS3.h"
#include "Images/ImageBluetooth.h"
#include "Images/ImagePocket4k.h"
#include "Images/WBBright.h"
#include "Images/WBCloud.h"
#include "Images/WBFlourescent.h"
#include "Images/WBIncandescent.h"
#include "Images/WBMixedLight.h"
#include "Images/WBBrightBG.h"
#include "Images/WBCloudBG.h"
#include "Images/WBFlourescentBG.h"
#include "Images/WBIncandescentBG.h"
#include "Images/WBMixedLightBG.h"

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
  WhiteBalanceTintWB = 104, // Editing White Balance
  WhiteBalanceTintT = 124, // Editing Tint
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
// 104 is WB / Tint - Edit WB
// 105 is Codec
// 106 is Framerate
// 107 is Resolution (one for each camera group, 4K, 6K/G2/Pro, Mini Pro G2, Mini Pro 12K)
// 108 is Media
// 109 is Lens
// 124 is WB / Tint - Edit Tint

// Keep track of the last camera modified time that we refreshed a screen so we don't keep refreshing a screen when the camera object remains unchanged.
static unsigned long lastRefreshedScreen = 0;

// Button pressed indications for use in each individual page
bool btnAPressed = false;
bool btnBPressed = false;

// Display elements on the screen common to all pages
void Screen_Common(int sideBarColour)
{
    // DEBUG_DEBUG("Screen_Common");

    // Sidebar colour
    sprite->fillRect(0, 0, 13, IHEIGHT, sideBarColour);
    sprite->fillRect(13, 0, 2, IHEIGHT, TFT_DARKGREY);

    if(BMDControlSystem::getInstance()->hasCamera())
    {
      auto camera = BMDControlSystem::getInstance()->getCamera();

      sprite->setTextColor(TFT_WHITE);

      if(connectedScreenIndex == Screens::Dashboard)
      {
        // Dashboard Bottom Buttons
        sprite->fillSmoothRoundRect(30, 210, 80, 40, 3, TFT_DARKCYAN);
        sprite->drawCenterString("TBD", 70, 217, &AgencyFB_Bold9pt7b);

        if(camera->isRecording)
          sprite->fillSmoothCircle(IWIDTH / 2, 240, 30, TFT_RED);
        else
        {
          // Two outlines to make it a bit thicker
          sprite->drawCircle(IWIDTH / 2, 240, 30, TFT_RED);
          sprite->drawCircle(IWIDTH / 2, 240, 29, TFT_RED);
        }
      }
      else if(connectedScreenIndex == Screens::Recording)
      {
        // Recording Screen Bottom Buttons
        sprite->fillSmoothRoundRect(30, 210, 80, 40, 3, TFT_DARKCYAN);
        sprite->drawCenterString("TBD", 70, 217, &AgencyFB_Bold9pt7b);

        if(camera->isRecording)
          sprite->fillSmoothCircle(IWIDTH / 2, 240, 30, TFT_RED);
        else
        {
          // Two outlines to make it a bit thicker
          sprite->drawCircle(IWIDTH / 2, 240, 30, TFT_RED);
          sprite->drawCircle(IWIDTH / 2, 240, 29, TFT_RED);
        }
      }
      else
      {
        // Other Screens Bottom Buttons

        switch(connectedScreenIndex)
        {
          case Screens::ISO:
          case Screens::ShutterAngleSpeed:
          case Screens::WhiteBalanceTintT:
          case Screens::Resolution:
          case Screens::Framerate:
          case Screens::Media:
            sprite->fillSmoothRoundRect(30, 210, 170, 40, 3, TFT_DARKCYAN);
            sprite->fillTriangle(60, 235, 70, 215, 50, 215, TFT_WHITE); // Up Arrow
            sprite->fillTriangle(150, 235, 170, 235, 160, 215, TFT_WHITE); // Down Arrow
            break;
          case Screens::Codec:
            // White Balance shows Presets or increment
            sprite->fillSmoothRoundRect(30, 210, 80, 40, 3, TFT_DARKCYAN);
            sprite->drawCenterString("CODEC", 70, 217, &AgencyFB_Bold9pt7b);

            sprite->fillSmoothRoundRect(120, 210, 80, 40, 3, TFT_DARKCYAN);
            sprite->drawCenterString("SETTING", 160, 217, &AgencyFB_Bold9pt7b);
            break;
          case Screens::WhiteBalanceTintWB:
            // White Balance shows Presets or increment
            sprite->fillSmoothRoundRect(30, 210, 80, 40, 3, TFT_DARKCYAN);
            sprite->drawCenterString("PRESET", 70, 217, &AgencyFB_Bold9pt7b);

            sprite->fillSmoothRoundRect(120, 210, 80, 40, 3, TFT_DARKCYAN);
            sprite->drawCenterString("+100", 160, 217, &AgencyFB_Bold9pt7b);
            break;
          case Screens::Lens:
            sprite->fillSmoothRoundRect(30, 210, 80, 40, 3, TFT_DARKCYAN);
            sprite->drawCenterString("FOCUS", 70, 217, &AgencyFB_Bold9pt7b);
            break;
        }
      }

      // Common Next Button
      sprite->fillSmoothRoundRect(215, 210, 80, 40, 3, TFT_DARKCYAN);
      sprite->drawCenterString("NEXT", 255, 217, &AgencyFB_Bold9pt7b);
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
      // Turn on recording box
      sprite->drawRect(15, 0, IWIDTH - 15, IHEIGHT, TFT_RED);
      sprite->drawRect(16, 1, IWIDTH - 13, IHEIGHT - 2, TFT_RED);
    }
  }
}

// Screen for when there's no connection, it's scanning, and it's trying to connect.
void Screen_NoConnection()
{
  DEBUG_DEBUG("Screen_NoConnection");

  if(!sprite->createSprite(IWIDTH_SPRITE, IHEIGHT_SPRITE)) return;

  // The camera to connect to.
  int connectToCameraIndex = -1;

  connectedScreenIndex = Screens::NoConnection;

  // Background on the sprite (overlay the part of the background that covers the sprite)
  sprite->pushImage(0, 0, IWIDTH, IHEIGHT, MPCSplash_M5Stack_CoreS3);

  // Black background for text and Bluetooth Logo
  sprite->fillRect(0, 3, IWIDTH, 51, TFT_BLACK);

  // Bluetooth Image
  sprite->pushImage(26, 6, 30, 46, Wikipedia_Bluetooth_30x46);

  switch(cameraConnection.status)
  {
    case BMDCameraConnection::Scanning:
      Screen_Common(TFT_BLUE); // Common elements
      sprite->drawString("Scanning...", 70, 20);
      break;
    case BMDCameraConnection::ScanningFound:
      Screen_Common(TFT_BLUE); // Common elements
      if(cameraConnection.cameraAddresses.size() == 1)
      {
        sprite->drawString("Found, connecting...", 70, 20);
        connectToCameraIndex = 0;
      }
      else
        sprite->drawString("Found cameras", 70, 20); // Multiple camera selection is below
      break;
    case BMDCameraConnection::ScanningNoneFound:
      Screen_Common(TFT_RED); // Common elements
      sprite->drawString("No camera found", 70, 20);
      break;
    case BMDCameraConnection::Connecting:
      Screen_Common(TFT_YELLOW); // Common elements
      sprite->drawString("Connecting...", 70, 20);
      break;
    case BMDCameraConnection::NeedPassKey:
      Screen_Common(TFT_PURPLE); // Common elements
      sprite->drawString("Need Pass Key", 70, 20);
      break;
    case BMDCameraConnection::FailedPassKey:
      Screen_Common(TFT_ORANGE); // Common elements
      sprite->drawString("Wrong Pass Key", 70, 20);
      break;
    case BMDCameraConnection::Disconnected:
      DEBUG_DEBUG("NoConnection - Disconnected");
      Screen_Common(TFT_RED); // Common elements
      sprite->drawString("Disconnected (wait)", 70, 20);
      break;
    case BMDCameraConnection::IncompatibleProtocol:
      // Note: This needs to be worked on as there's no incompatible protocol connections yet.
      Screen_Common(TFT_RED); // Common elements
      sprite->drawString("Incompatible Protocol", 70, 20);
      break;
    default:
      break;
  }

  // Show up to two cameras
  int cameras = cameraConnection.cameraAddresses.size();
  for(int count = 0; count < cameras && count < 2; count++)
  {
    // Cameras
    sprite->fillRoundRect(25 + (count * 125) + (count * 10), 60, 125, 100, 5, TFT_DARKGREY);

    // Highlight the camera to connect to
    if(connectToCameraIndex != -1 && connectToCameraIndex == count)
    {
      sprite->drawRoundRect(25 + (count * 125) + (count * 10), 60, 5, 2, TFT_GREEN);
    }

    sprite->pushImage(33 + (count * 125) + (count * 10), 69, 110, 61, blackmagic_pocket_4k_110x61);

    sprite->drawString(cameraConnection.cameraAddresses[count].toString().c_str(), 33 + (count * 125) + (count * 10), 144, &Lato_Regular6pt7b);
  }

  // If there's more than one camera, check for a tap to see if they have nominated one to connect to
  /*
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
  */

  if(connectToCameraIndex != -1)
  {
    cameraConnection.connect(cameraConnection.cameraAddresses[connectToCameraIndex]);
    connectToCameraIndex = -1;

    if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::FailedPassKey)
    {
      DEBUG_DEBUG("NoConnection - Failed Pass Key");
    }
  }

  sprite->pushSprite(0, 0);
}

// Default screen for connected state
void Screen_Dashboard(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Dashboard;

  auto camera = BMDControlSystem::getInstance()->getCamera();
  int xshift = 0;

  bool tappedAction = false;

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh)
  // if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  // DEBUG_DEBUG("Screen Dashboard Refreshing.");

  if(!sprite->createSprite(IWIDTH_SPRITE, IHEIGHT_SPRITE)) return;

  if(cameraConnection.getInitialPayloadTime() != ULONG_MAX)
    sprite->fillSprite(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // Set font here rather than on each drawString line
  sprite->setFont(&Lato_Regular11pt7b);

  // ISO
  if(camera->hasSensorGainISOValue())
  {
    sprite->fillSmoothRoundRect(20, 5, 75, 65, 3, TFT_DARKGREY);
    sprite->setTextColor(TFT_WHITE);

    sprite->drawCentreString(String(camera->getSensorGainISOValue()), 58, 23);

    sprite->drawCentreString("ISO", 58, 50, &AgencyFB_Regular7pt7b);
  }

  // Shutter
  xshift = 80;
  if(camera->hasShutterAngle() || camera->hasShutterSpeed())
  {
    sprite->fillSmoothRoundRect(20 + xshift, 5, 75, 65, 3, TFT_DARKGREY);
    sprite->setTextColor(TFT_WHITE);

    if(camera->shutterValueIsAngle && camera->hasShutterAngle())
    {
      // Shutter Angle
      int currentShutterAngle = camera->getShutterAngle();
      float ShutterAngleFloat = currentShutterAngle / 100.0;

      sprite->drawCentreString(String(ShutterAngleFloat, (currentShutterAngle % 100 == 0 ? 0 : 1)), 58 + xshift, 23);
    }
    else if(camera->hasShutterSpeed())
    {
      // Shutter Speed
      int currentShutterSpeed = camera->getShutterSpeed();

      sprite->drawCentreString("1/" + String(currentShutterSpeed), 58 + xshift, 23);
    }

    sprite->drawCentreString(camera->shutterValueIsAngle ? "DEGREES" : "SPEED", 58 + xshift, 50, &AgencyFB_Regular7pt7b); //  "SHUTTER"
  }

  // WhiteBalance and Tint
  xshift += 80;
  if(camera->hasWhiteBalance() || camera->hasTint())
  {
    sprite->fillSmoothRoundRect(20 + xshift, 5, 135, 65, 3, TFT_DARKGREY);
    sprite->setTextColor(TFT_WHITE);

    if(camera->hasWhiteBalance())
      sprite->drawCentreString(String(camera->getWhiteBalance()), 58 + xshift, 23);

    sprite->drawCentreString("WB", 58 + xshift, 50, &AgencyFB_Regular7pt7b);

    xshift += 66;

    if(camera->hasTint())
      sprite->drawCentreString(String(camera->getTint()), 58 + xshift, 23);

    sprite->drawCentreString("TINT", 58 + xshift, 50, &AgencyFB_Regular7pt7b);
  }

  // Codec
  if(camera->hasCodec())
  {
    sprite->fillSmoothRoundRect(20, 75, 155, 40, 3, TFT_DARKGREY);

    sprite->drawCentreString(camera->getCodec().to_string().c_str(), 97, 84);
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
      sprite->fillSmoothRoundRect(20, 120, 100, 40, 3, TFT_DARKGREY);

      sprite->drawCentreString(slotString.c_str(), 70, 130);

      // Show recording error
      if(camera->hasRecordError())
        sprite->drawRoundRect(20, 120, 100, 40, 3, TFT_RED);
    }
    else
    {
      // Show no Media
      sprite->fillSmoothRoundRect(20, 120, 100, 40, 3, TFT_DARKGREY);
      sprite->drawCentreString("NO MEDIA", 70, 135, &AgencyFB_Regular7pt7b);
    }
  }

  // Recording Format - Frame Rate and Resolution
  if(camera->hasRecordingFormat())
  {
    // Frame Rate
    sprite->fillSmoothRoundRect(180, 75, 135, 40, 3, TFT_DARKGREY);

    sprite->drawCentreString(camera->getRecordingFormat().frameRate_string().c_str(), 237, 84);

    sprite->drawCentreString("fps", 300, 89, &AgencyFB_Regular7pt7b);

    // Resolution
    sprite->fillSmoothRoundRect(125, 120, 190, 40, 3, TFT_DARKGREY);

    std::string resolution = camera->getRecordingFormat().frameDimensionsShort_string();
    sprite->drawCentreString(resolution.c_str(), 220, 130);
  }

  // Lens
  if(camera->hasFocalLengthMM() && camera->hasApertureFStopString())
  {
    // Lens
    sprite->fillSmoothRoundRect(20, 165, 295, 40, 3, TFT_DARKGREY);

    if(camera->hasFocalLengthMM() && camera->hasApertureFStopString())
    {
      auto focalLength = camera->getFocalLengthMM();
      std::string focalLengthMM = std::to_string(focalLength);
      std::string combined = focalLengthMM + "mm";

      sprite->drawString(combined.c_str(), 30, 174);
      sprite->drawString(camera->getApertureFStopString().c_str(), 100, 174);
    }
  }

  sprite->pushSprite(0, 0);
}

void Screen_Recording(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Recording;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // If we have a tap, we should determine if it is on anything
  /*
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
  */

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh)
  // if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();

  DEBUG_DEBUG("Screen Recording Refreshed.");

  sprite->fillSprite(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // M5GFX, set font here rather than on each drawString line
  sprite->setFont(&Lato_Regular11pt7b);

  // Record button
  if(camera->isRecording) sprite->fillSmoothCircle(257, 63, 58, TFT_RED); // Recording solid
  sprite->drawCircle(257, 63, 57, (camera->isRecording ? TFT_RED : TFT_LIGHTGREY)); // Outer
  sprite->fillSmoothCircle(257, 63, 38, camera->isRecording ? TFT_RED : TFT_LIGHTGREY); // Inner

  // Timecode
  sprite->setTextColor(camera->isRecording ? TFT_RED : TFT_WHITE);
  sprite->drawString(camera->getTimecodeString().c_str(), 30, 57);

  // Remaining time and any errors
  if(camera->getMediaSlots().size() != 0 && camera->hasActiveMediaSlot())
  {
    sprite->setTextColor(TFT_LIGHTGREY);
    sprite->drawString((camera->getActiveMediaSlot().GetMediumString() + " " + camera->getActiveMediaSlot().remainingRecordTimeString).c_str(), 30, 130);

    sprite->drawString("REMAINING TIME", 30, 153, &Lato_Regular5pt7b);

    // Show any media record errors
    if(camera->hasRecordError())
    {
      sprite->setTextColor(TFT_RED);
      sprite->drawString("RECORD ERROR", 30, 20);
    }
  }

  sprite->pushSprite(0, 0);
}

void Screen_ISO(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::ISO;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // If we have an up/down button press
  bool tappedAction = false;
  std::vector<int> Options_ISO = {200, 400, 640, 800, 1250, 3200, 8000, 12800};
  int Options_ISO_SelectingIndex = -1; // when using up/down buttons keeps track of what option you are navigating to
  if(btnAPressed || btnBPressed)
  {
    if(camera->hasSensorGainISOValue()) // Ensure we have the ISO value before allowing it to be changed
    {
      int currentValue = camera->getSensorGainISOValue();

      bool up = btnBPressed; // up = true, down = false

      // Find the closest ISO (particularly if it's not the standard set)
      int closestValue = Options_ISO[0];
      int minDifference = std::abs(currentValue - closestValue);

      for (int i = 1; i < Options_ISO.size(); ++i) {
          int difference = std::abs(currentValue - Options_ISO[i]);
          if (difference < minDifference) {
              minDifference = difference;
              closestValue = Options_ISO[i];
          }
      }

      auto iter = std::find(Options_ISO.begin(), Options_ISO.end(), closestValue);
      // Get the closest ISO
      if (iter != Options_ISO.end()) {
          Options_ISO_SelectingIndex = std::distance(Options_ISO.begin(), iter);
      }

      DEBUG_DEBUG("Options_ISO_SelectingIndex");
      DEBUG_DEBUG(std::to_string(Options_ISO_SelectingIndex).c_str());

      if(Options_ISO_SelectingIndex != -1)
      {
        Options_ISO_SelectingIndex = Options_ISO_SelectingIndex + (up ? 1 : -1); // Move the selected option

        if(Options_ISO_SelectingIndex < 0)
        {
          // Down button on the first option, go to the last option
          Options_ISO_SelectingIndex = Options_ISO.size() -1;
        }
        else if(Options_ISO_SelectingIndex > (Options_ISO.size() - 1))
        {
          // Up button on the last option, go to the first option
          Options_ISO_SelectingIndex = 0;
        }

        // ISO selected, send it to the camera
        PacketWriter::writeISO(Options_ISO[Options_ISO_SelectingIndex], &cameraConnection);

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

  sprite->fillScreen(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // M5GFX, set font here rather than on each drawString line
  sprite->setFont(&Lato_Regular11pt7b);

  // Get the current ISO value
  int currentISO = 0;
  if(camera->hasSensorGainISOValue())
    currentISO = camera->getSensorGainISOValue();

  // ISO label
  sprite->setTextColor(TFT_WHITE);
  sprite->drawString("ISO", 30, 9, &AgencyFB_Bold9pt7b);

  // sprite->textbgcolor = TFT_DARKGREY;

  // 200
  int labelISO = 200;
  sprite->fillSmoothRoundRect(20, 30, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(String(labelISO).c_str(), 65, 41);

  // 400
  labelISO = 400;
  sprite->fillSmoothRoundRect(115, 30, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(String(labelISO).c_str(), 160, 36);
  sprite->drawCentreString("NATIVE", 160, 59, &Lato_Regular5pt7b);

  // 8000
  labelISO = 8000;
  sprite->fillSmoothRoundRect(210, 30, 100, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(String(labelISO).c_str(), 260, 41);

  // 640
  labelISO = 640;
  sprite->fillSmoothRoundRect(20, 75, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(String(labelISO).c_str(), 65, 87);

  // 800
  labelISO = 800;
  sprite->fillSmoothRoundRect(115, 75, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(String(labelISO).c_str(), 160, 87);

  // 12800
  labelISO = 12800;
  sprite->fillSmoothRoundRect(210, 75, 100, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(String(labelISO).c_str(), 260, 87);

  // 1250
  labelISO = 1250;
  sprite->fillSmoothRoundRect(20, 120, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(String(labelISO).c_str(), 65, 131);

  // 3200
  labelISO = 3200;
  sprite->fillSmoothRoundRect(115, 120, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(String(labelISO).c_str(), 160, 126);
  sprite->drawCentreString("NATIVE", 160, 149, &Lato_Regular5pt7b);

  // Custom ISO - show if ISO is not one of the above values
  if(currentISO != 0)
  {
    // Only show the ISO value if it's not a standard one
    if(currentISO != 200 && currentISO != 400 && currentISO != 640 && currentISO != 800 && currentISO != 1250 && currentISO != 3200 && currentISO != 8000 && currentISO != 12800)
    {
      sprite->fillSmoothRoundRect(210, 120, 100, 40, 3, TFT_DARKGREEN);
      // sprite->textbgcolor = TFT_DARKGREEN;
      sprite->drawCentreString(String(currentISO).c_str(), 260, 126);
      sprite->drawCentreString("CUSTOM", 260, 149, &Lato_Regular5pt7b);
    }
  }

  sprite->pushSprite(0, 0);
}

void Screen_ShutterAngle(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::ShutterAngleSpeed;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // If we have an up/down button press
  bool tappedAction = false;
  std::vector<int> Options_ShutterAngle = {1500, 6000, 9000, 12000, 15000, 18000, 27000, 36000}; // Values are X100
  int Options_ShutterAngle_SelectingIndex = -1; // when using up/down buttons keeps track of what option you are navigating to
  if(btnAPressed || btnBPressed)
  {
    DEBUG_DEBUG("Shutter Angle Screen: Btn A/B pressed");

    if(camera->hasShutterAngle()) // Ensure we have a value before allowing it to be changed
    {
      // Get the current Shutter Angle (comes through X100, so 180 degrees = 18000)
      int currentValue = camera->getShutterAngle();

      bool up = btnBPressed; // up = true, down = false

      // If we don't have a selected value yet, set to the current one
      if(Options_ShutterAngle_SelectingIndex == -1)
      {
        auto iter = std::find(Options_ShutterAngle.begin(), Options_ShutterAngle.end(), currentValue);
        int index = (iter != Options_ShutterAngle.end()) ? std::distance(Options_ShutterAngle.begin(), iter) : -1;

        if(index != -1)
          Options_ShutterAngle_SelectingIndex = index;
        else // Custom Shutter Angle, use the latest one
        {
          int closestIndex = -1;
          int closestValue = std::numeric_limits<int>::min();  // Initialize with the lowest possible value

          for (int i = 0; i < Options_ShutterAngle.size(); ++i) {
              int optionValue = Options_ShutterAngle[i];

              if (optionValue <= currentValue && optionValue > closestValue) {
                  closestIndex = i;
                  closestValue = optionValue;
              }
          }

          if (closestIndex != -1)
          {
            if(up)
              Options_ShutterAngle_SelectingIndex = closestIndex; // Pressing up, closest one lower is right
            else
            {
              if(closestIndex < Options_ShutterAngle.size() - 1)
                Options_ShutterAngle_SelectingIndex = closestIndex + 1; // Pressing down, go up
              else
                Options_ShutterAngle_SelectingIndex = 0; // Already at the top, so go back to the start
            }
          }
          else // Lower than the lowest, so set to lowest in options
          {
            if(up)
              Options_ShutterAngle_SelectingIndex = Options_ShutterAngle.size() - 1; // Pressing up on a number lower than the lowest, put on the highest so we can go back around to the lowest in the set
            else
              Options_ShutterAngle_SelectingIndex = 0; // Otherwise pressing down and keep at the lowest and we'll be going to the highest
          }
        }
      }

      // Move to the next option and send it to the camera
      if(Options_ShutterAngle_SelectingIndex != -1)
      {
        Options_ShutterAngle_SelectingIndex = Options_ShutterAngle_SelectingIndex + (up ? 1 : -1); // Move the selected option

        if(Options_ShutterAngle_SelectingIndex < 0)
        {
          // Down button on the first option, go to the last option
          Options_ShutterAngle_SelectingIndex = Options_ShutterAngle.size() -1;
        }
        else if(Options_ShutterAngle_SelectingIndex > (Options_ShutterAngle.size() - 1))
        {
          // Up button on the last option, go to the first option
          Options_ShutterAngle_SelectingIndex = 0;
        }

        // Shutter Angle selected, send it to the camera
        PacketWriter::writeShutterAngle(Options_ShutterAngle[Options_ShutterAngle_SelectingIndex], &cameraConnection);

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

  sprite->fillScreen(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // Get the current Shutter Angle (comes through X100, so 180 degrees = 18000)
  int currentShutterAngle = 0;
  if(camera->hasShutterAngle())
    currentShutterAngle = camera->getShutterAngle();

  // Shutter Angle label
  sprite->setTextColor(TFT_WHITE);
  sprite->drawString("DEGREES", 265, 9, &AgencyFB_Regular7pt7b);
  sprite->drawString("SHUTTER ANGLE", 30, 9, &AgencyFB_Bold9pt7b);

  // 15
  int labelShutterAngle = 1500;
  sprite->fillSmoothRoundRect(20, 30, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ShutterAngle_SelectingIndex == 0) sprite->drawRoundRect(20, 30, 90, 40, 1, TFT_WHITE);
  sprite->drawCentreString(String(labelShutterAngle / 100).c_str(), 65, 41);

  // 60
  labelShutterAngle = 6000;
  sprite->fillSmoothRoundRect(115, 30, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ShutterAngle_SelectingIndex == 1) sprite->drawRoundRect(115, 30, 90, 40, 1, TFT_WHITE);
  sprite->drawCentreString(String(labelShutterAngle / 100).c_str(), 160, 41);

  // 90
  labelShutterAngle = 9000;
  sprite->fillSmoothRoundRect(210, 30, 100, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ShutterAngle_SelectingIndex == 2) sprite->drawRoundRect(210, 30, 100, 40, 1, TFT_WHITE);
  sprite->drawCentreString(String(labelShutterAngle / 100).c_str(), 260, 41);

  // 120
  labelShutterAngle = 12000;
  sprite->fillSmoothRoundRect(20, 75, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ShutterAngle_SelectingIndex == 3) sprite->drawRoundRect(20, 75, 90, 40, 1, TFT_WHITE);
  sprite->drawCentreString(String(labelShutterAngle / 100).c_str(), 65, 87);

  // 150
  labelShutterAngle = 15000;
  sprite->fillSmoothRoundRect(115, 75, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ShutterAngle_SelectingIndex == 4) sprite->drawRoundRect(115, 75, 90, 40, 1, TFT_WHITE);
  sprite->drawCentreString(String(labelShutterAngle / 100).c_str(), 160, 87);

  // 180 (with a border around it)
  labelShutterAngle = 18000;
  sprite->fillSmoothRoundRect(210, 75, 100, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawRoundRect(210, 75, 100, 40, 3, TFT_DARKGREEN);
  if(Options_ShutterAngle_SelectingIndex == 5) sprite->drawRoundRect(210, 75, 100, 40, 1, TFT_WHITE);
  sprite->drawCentreString(String(labelShutterAngle / 100).c_str(), 260, 87);

  // 270
  labelShutterAngle = 27000;
  sprite->fillSmoothRoundRect(20, 120, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ShutterAngle_SelectingIndex == 6) sprite->drawRoundRect(20, 120, 90, 40, 1, TFT_WHITE);
  sprite->drawCentreString(String(labelShutterAngle / 100).c_str(), 65, 131);

  // 360
  labelShutterAngle = 36000;
  sprite->fillSmoothRoundRect(115, 120, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ShutterAngle_SelectingIndex == 7) sprite->drawRoundRect(115, 120, 90, 40, 1, TFT_WHITE);
  sprite->drawCentreString(String(labelShutterAngle / 100).c_str(), 160, 131);

  // Custom Shutter Angle - show if not one of the above values
  if(currentShutterAngle != 0)
  {
    // Only show the Shutter angle value if it's not a standard one
    if(currentShutterAngle != 1500 && currentShutterAngle != 6000 && currentShutterAngle != 9000 && currentShutterAngle != 12000 && currentShutterAngle != 15000 && currentShutterAngle != 18000 && currentShutterAngle != 27000 && currentShutterAngle != 36000)
    {
      float customShutterAngle = currentShutterAngle / 100.0;

      sprite->fillSmoothRoundRect(210, 120, 100, 40, 3, TFT_DARKGREEN);
      sprite->drawCentreString(String(customShutterAngle, (currentShutterAngle % 100 == 0 ? 0 : 1)).c_str(), 260, 126);
      sprite->drawCentreString("CUSTOM", 260, 149, &Lato_Regular5pt7b);
    }
  }

  sprite->pushSprite(0, 0);
}

void Screen_ShutterSpeed(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::ShutterAngleSpeed;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // Shutter Speed Values: 1/30, 1/50, 1/60, 1/125, 1/200, 1/250, 1/500, 1/2000, CUSTOM
  // Note that the protocol takes the denominator as its parameter value. So for 1/60 we'll pass 60.

  // If we have an up/down button press
  bool tappedAction = false;
  std::vector<int> Options_ShutterSpeed = {30, 50, 60, 125, 200, 250, 500, 2000};
  int Options_ShutterSpeed_SelectingIndex = -1; // when using up/down buttons keeps track of what option you are navigating to
  if(btnAPressed || btnBPressed)
  {
    DEBUG_DEBUG("Shutter Speed Screen: Btn A/B pressed");

    if(camera->hasShutterSpeed()) // Ensure we have a value before allowing it to be changed
    {
      // Get the current Shutter Speed (denominator, e.g. 1/60 will return 60)
      int currentValue = camera->getShutterSpeed();

      bool up = btnBPressed; // up = true, down = false

      // If we don't have a selected value yet, set to the current one
      if(Options_ShutterSpeed_SelectingIndex == -1)
      {
        auto iter = std::find(Options_ShutterSpeed.begin(), Options_ShutterSpeed.end(), currentValue);
        int index = (iter != Options_ShutterSpeed.end()) ? std::distance(Options_ShutterSpeed.begin(), iter) : -1;

        if(index != -1)
          Options_ShutterSpeed_SelectingIndex = index;
        else // Custom Shutter Speed, use the latest one
        {
          int closestIndex = -1;
          int closestValue = std::numeric_limits<int>::min();  // Initialize with the lowest possible value

          for (int i = 0; i < Options_ShutterSpeed.size(); ++i) {
              int optionValue = Options_ShutterSpeed[i];

              if (optionValue <= currentValue && optionValue > closestValue) {
                  closestIndex = i;
                  closestValue = optionValue;
              }
          }

          if (closestIndex != -1)
          {
            if(up)
              Options_ShutterSpeed_SelectingIndex = closestIndex; // Pressing up, closest one lower is right
            else
            {
              if(closestIndex < Options_ShutterSpeed.size() - 1)
                Options_ShutterSpeed_SelectingIndex = closestIndex + 1; // Pressing down, go up
              else
                Options_ShutterSpeed_SelectingIndex = 0; // Already at the top, so go back to the start
            }
          }
          else // Lower than the lowest, so set to lowest in options
          {
            if(up)
              Options_ShutterSpeed_SelectingIndex = Options_ShutterSpeed.size() - 1; // Pressing up on a number lower than the lowest, put on the highest so we can go back around to the lowest in the set
            else
              Options_ShutterSpeed_SelectingIndex = 0; // Otherwise pressing down and keep at the lowest and we'll be going to the highest
          }
        }
      }

      // Move to the next option and send it to the camera
      if(Options_ShutterSpeed_SelectingIndex != -1)
      {
        Options_ShutterSpeed_SelectingIndex = Options_ShutterSpeed_SelectingIndex + (up ? 1 : -1); // Move the selected option

        if(Options_ShutterSpeed_SelectingIndex < 0)
        {
          // Down button on the first option, go to the last option
          Options_ShutterSpeed_SelectingIndex = Options_ShutterSpeed.size() -1;
        }
        else if(Options_ShutterSpeed_SelectingIndex > (Options_ShutterSpeed.size() - 1))
        {
          // Up button on the last option, go to the first option
          Options_ShutterSpeed_SelectingIndex = 0;
        }

        // Shutter Speed selected, send it to the camera
        PacketWriter::writeShutterSpeed(Options_ShutterSpeed[Options_ShutterSpeed_SelectingIndex], &cameraConnection);

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


  sprite->fillScreen(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // Get the current Shutter Speed
  int currentShutterSpeed = 0;
  if(camera->hasShutterSpeed())
    currentShutterSpeed = camera->getShutterSpeed();
  else
    DEBUG_DEBUG("DO NOT HAVE SHUTTER SPEED!");

  // Shutter Speed label
  sprite->setTextColor(TFT_WHITE);

  if(camera->hasRecordingFormat())
  {
    sprite->drawRightString(camera->getRecordingFormat().frameRate_string().c_str() + String(" fps"), 310, 9, &Lato_Regular5pt7b);
  }

  sprite->drawString("SHUTTER SPEED", 30, 9, &AgencyFB_Bold9pt7b);

  // 1/30
  int labelShutterSpeed = 30;
  sprite->fillSmoothRoundRect(20, 30, 90, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString("1/" + String(labelShutterSpeed), 65, 41);

  // 1/50
  labelShutterSpeed = 50;
  sprite->fillSmoothRoundRect(115, 30, 90, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString("1/" + String(labelShutterSpeed), 160, 41);

  // 1/60
  labelShutterSpeed = 60;
  sprite->fillSmoothRoundRect(210, 30, 100, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString("1/" + String(labelShutterSpeed), 260, 41);

  // 1/125
  labelShutterSpeed = 125;
  sprite->fillSmoothRoundRect(20, 75, 90, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString("1/" + String(labelShutterSpeed), 65, 87);

  // 1/200
  labelShutterSpeed = 200;
  sprite->fillSmoothRoundRect(115, 75, 90, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString("1/" + String(labelShutterSpeed), 160, 87);

  // 1/250
  labelShutterSpeed = 250;
  sprite->fillSmoothRoundRect(210, 75, 100, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString("1/" + String(labelShutterSpeed), 260, 87);

  // 1/500
  labelShutterSpeed = 500;
  sprite->fillSmoothRoundRect(20, 120, 90, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString("1/" + String(labelShutterSpeed), 65, 131);

  // 1/2000
  labelShutterSpeed = 2000;
  sprite->fillSmoothRoundRect(115, 120, 90, 40, 3, (currentShutterSpeed == labelShutterSpeed ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString("1/" + String(labelShutterSpeed), 160, 131);

  // Custom Shutter Speed - show if not one of the above values
  if(currentShutterSpeed != 0)
  {
    // Only show the Shutter Speed value if it's not a standard one
    if(currentShutterSpeed != 30 && currentShutterSpeed != 50 && currentShutterSpeed != 60 && currentShutterSpeed != 125 && currentShutterSpeed != 200 && currentShutterSpeed != 250 && currentShutterSpeed != 500 && currentShutterSpeed != 2000)
    {
      sprite->fillSmoothRoundRect(210, 120, 100, 40, 3, TFT_DARKGREEN);
      sprite->drawCentreString("1/" + String(currentShutterSpeed), 260, 126);
      sprite->drawCentreString("CUSTOM", 260, 149, &Lato_Regular5pt7b);
    }
  }

  sprite->pushSprite(0, 0);
}

void Screen_WBTint(bool editWB, bool forceRefresh = false) // editWB indicates editing White Balance when true, editing Tint when false
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = editWB ? Screens::WhiteBalanceTintWB : Screens::WhiteBalanceTintT;

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

  // If we have an up/down button press
  bool tappedAction = false;
  if((btnAPressed || btnBPressed) && camera->hasWhiteBalance() && camera->hasTint())
  {
    DEBUG_DEBUG("White Balance Screen: Btn A/B pressed");

    if(editWB) // White Balance Edit
    {
      if(btnAPressed)
      {
        // White Balance Presets

        std::vector<int> Options_WB = {5600, 3200, 4000, 4500, 6500};
        std::vector<int> Options_WB_Tint = {10, 0, 15, 15, 10}; // Corresponding Tints for WB presets
        int Options_WB_SelectingIndex = -1; // when using up/down buttons keeps track of what option you are navigating to

        // Get the current White Balance
        int currentValue = currentWB;

        bool up = true;

        // If we don't have a selected value yet, set to the current one
        if(Options_WB_SelectingIndex == -1)
        {
          auto iter = std::find(Options_WB.begin(), Options_WB.end(), currentValue);
          int index = (iter != Options_WB.end()) ? std::distance(Options_WB.begin(), iter) : -1;

          if(index != -1)
            Options_WB_SelectingIndex = index;
          else // Custom Shutter Speed, use the latest one
          {
            int closestIndex = -1;
            int closestValue = std::numeric_limits<int>::min();  // Initialize with the lowest possible value

            for (int i = 0; i < Options_WB.size(); ++i) {
                int optionValue = Options_WB[i];

                if (optionValue <= currentValue && optionValue > closestValue) {
                    closestIndex = i;
                    closestValue = optionValue;
                }
            }

            if (closestIndex != -1)
            {
              if(up)
                Options_WB_SelectingIndex = closestIndex; // Pressing up, closest one lower is right
              else
              {
                if(closestIndex < Options_WB.size() - 1)
                  Options_WB_SelectingIndex = closestIndex + 1; // Pressing down, go up
                else
                  Options_WB_SelectingIndex = 0; // Already at the top, so go back to the start
              }
            }
            else // Lower than the lowest, so set to lowest in options
            {
              if(up)
                Options_WB_SelectingIndex = Options_WB.size() - 1; // Pressing up on a number lower than the lowest, put on the highest so we can go back around to the lowest in the set
              else
                Options_WB_SelectingIndex = 0; // Otherwise pressing down and keep at the lowest and we'll be going to the highest
            }
          }
        }

        // Move to the next option and send it to the camera
        if(Options_WB_SelectingIndex != -1)
        {
          Options_WB_SelectingIndex = Options_WB_SelectingIndex + (up ? 1 : -1); // Move the selected option

          if(Options_WB_SelectingIndex < 0)
          {
            // Down button on the first option, go to the last option
            Options_WB_SelectingIndex = Options_WB.size() -1;
          }
          else if(Options_WB_SelectingIndex > (Options_WB.size() - 1))
          {
            // Up button on the last option, go to the first option
            Options_WB_SelectingIndex = 0;
          }

          // Shutter Speed selected, send it to the camera
          PacketWriter::writeWhiteBalance(Options_WB[Options_WB_SelectingIndex], Options_WB_Tint[Options_WB_SelectingIndex], &cameraConnection);

          tappedAction = true;
        }
      }
      else
      {
        // White Balance +100
        int newWB = currentWB + 100;

        if(newWB > 10000)
          newWB = 2500;

          // Shutter Speed selected, send it to the camera
          PacketWriter::writeWhiteBalance(newWB, currentTint, &cameraConnection);

          tappedAction = true;
      }
    }
    else if(!editWB && camera->hasTint())
    {
      std::vector<int> Options_Tint = {-50, -40, -30, -20, -15, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 30, 40, 50};
      int Options_Tint_SelectingIndex = -1; // when using up/down buttons keeps track of what option you are navigating to

      // Get the current Tint
      int currentValue = currentTint;

      bool up = btnBPressed; // up = true, down = false

      // If we don't have a selected value yet, set to the current one
      if(Options_Tint_SelectingIndex == -1)
      {
        auto iter = std::find(Options_Tint.begin(), Options_Tint.end(), currentValue);
        int index = (iter != Options_Tint.end()) ? std::distance(Options_Tint.begin(), iter) : -1;

        if(index != -1)
          Options_Tint_SelectingIndex = index;
        else // Custom Shutter Speed, use the latest one
        {
          int closestIndex = -1;
          int closestValue = std::numeric_limits<int>::min();  // Initialize with the lowest possible value

          for (int i = 0; i < Options_Tint.size(); ++i) {
              int optionValue = Options_Tint[i];

              if (optionValue <= currentValue && optionValue > closestValue) {
                  closestIndex = i;
                  closestValue = optionValue;
              }
          }

          if (closestIndex != -1)
          {
            if(up)
              Options_Tint_SelectingIndex = closestIndex; // Pressing up, closest one lower is right
            else
            {
              if(closestIndex < Options_Tint.size() - 1)
                Options_Tint_SelectingIndex = closestIndex + 1; // Pressing down, go up
              else
                Options_Tint_SelectingIndex = 0; // Already at the top, so go back to the start
            }
          }
          else // Lower than the lowest, so set to lowest in options
          {
            if(up)
              Options_Tint_SelectingIndex = Options_Tint.size() - 1; // Pressing up on a number lower than the lowest, put on the highest so we can go back around to the lowest in the set
            else
              Options_Tint_SelectingIndex = 0; // Otherwise pressing down and keep at the lowest and we'll be going to the highest
          }
        }
      }

      // Move to the next option and send it to the camera
      if(Options_Tint_SelectingIndex != -1)
      {
        Options_Tint_SelectingIndex = Options_Tint_SelectingIndex + (up ? 1 : -1); // Move the selected option

        if(Options_Tint_SelectingIndex < 0)
        {
          // Down button on the first option, go to the last option
          Options_Tint_SelectingIndex = Options_Tint.size() -1;
        }
        else if(Options_Tint_SelectingIndex > (Options_Tint.size() - 1))
        {
          // Up button on the last option, go to the first option
          Options_Tint_SelectingIndex = 0;
        }

        // Shutter Speed selected, send it to the camera
        PacketWriter::writeWhiteBalance(currentWB, Options_Tint[Options_Tint_SelectingIndex], &cameraConnection);

        tappedAction = true;
      }
    }
  }

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen WB Tint Refreshed.");

  sprite->fillScreen(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // ISO label
  sprite->setTextColor(TFT_WHITE);
  sprite->drawString(editWB ? "WHITE BALANCE" : "TINT", 30, 9, &AgencyFB_Bold9pt7b);
  sprite->drawCentreString("TINT", 54, 132, &AgencyFB_Bold9pt7b);

  // Bright, 5600K
  int lblWBKelvin = 5600;
  int lblTint = 10;
  sprite->fillSmoothRoundRect(20, 30, 70, 40, 3, (currentWB == lblWBKelvin && currentTint == lblTint ? TFT_DARKGREEN : TFT_DARKGREY));
  if(currentWB == lblWBKelvin && currentTint == lblTint)
    sprite->pushImage(40, 35, 30, 30, WBBrightBG);
  else
    sprite->pushImage(40, 35, 30, 30, WBBright);

  // Incandescent, 3200K
  lblWBKelvin = 3200;
  lblTint = 0;
  sprite->fillSmoothRoundRect(95, 30, 70, 40, 3, (currentWB == lblWBKelvin && currentTint == lblTint ? TFT_DARKGREEN : TFT_DARKGREY));
  if(currentWB == lblWBKelvin && currentTint == lblTint)
    sprite->pushImage(115, 35, 30, 30, WBIncandescentBG);
  else
    sprite->pushImage(115, 35, 30, 30, WBIncandescent);

  // Fluorescent, 4000K
  lblWBKelvin = 4000;
  lblTint = 15;
  sprite->fillSmoothRoundRect(170, 30, 70, 40, 3, (currentWB == lblWBKelvin && currentTint == lblTint ? TFT_DARKGREEN : TFT_DARKGREY));
  if(currentWB == lblWBKelvin && currentTint == lblTint)
    sprite->pushImage(190, 35, 30, 30, WBFlourescentBG);
  else
    sprite->pushImage(190, 35, 30, 30, WBFlourescent);

  // Mixed Light, 4500K
  lblWBKelvin = 4500;
  lblTint = 15;
  sprite->fillSmoothRoundRect(245, 30, 70, 40, 3, (currentWB == lblWBKelvin && currentTint == lblTint ? TFT_DARKGREEN : TFT_DARKGREY));
  if(currentWB == lblWBKelvin && currentTint == lblTint)
    sprite->pushImage(265, 35, 30, 30, WBMixedLightBG);
  else
    sprite->pushImage(265, 35, 30, 30, WBMixedLight);

  // Cloud, 6500K
  lblWBKelvin = 6500;
  lblTint = 10;
  sprite->fillSmoothRoundRect(20, 75, 70, 40, 3, (currentWB == lblWBKelvin && currentTint == lblTint ? TFT_DARKGREEN : TFT_DARKGREY));
  if(currentWB == lblWBKelvin && currentTint == lblTint)
    sprite->pushImage(40, 80, 30, 30, WBCloudBG);
  else
    sprite->pushImage(40, 80, 30, 30, WBCloud);

  // Current White Balance Kelvin
  sprite->fillSmoothRoundRect(160, 75, 90, 40, 3, editWB ? TFT_DARKGREEN : TFT_DARKGREY);
  sprite->drawCentreString(String(currentWB), 205, 80);
  sprite->drawCentreString("KELVIN", 205, 103, &Lato_Regular5pt7b);

  if(editWB)
  {
    // WB Adjust Left <
    sprite->fillSmoothRoundRect(95, 75, 60, 40, 3, TFT_DARKGREY);
    sprite->drawCentreString("<", 125, 87);

    // WB Adjust Right >
    sprite->fillSmoothRoundRect(255, 75, 60, 40, 3, TFT_DARKGREY);
    sprite->drawCentreString(">", 284, 87);
  }

  // Current Tint
  sprite->fillSmoothRoundRect(160, 120, 90, 40, 3, !editWB ? TFT_DARKGREEN : TFT_DARKGREY);
  sprite->drawCentreString(String(currentTint), 205, 130);

  if(!editWB)
  {
    // Tint Adjust Left <
    sprite->fillSmoothRoundRect(95, 120, 60, 40, 3, TFT_DARKGREY);
    sprite->drawCentreString("<", 125, 132);

    // Tint Adjust Right >
    sprite->fillSmoothRoundRect(255, 120, 60, 40, 3, TFT_DARKGREY);
    sprite->drawCentreString(">", 284, 132);
  }

  sprite->pushSprite(0, 0);
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

  // If we have an up/down button press
  bool tappedAction = false;
  if(btnAPressed || btnBPressed)
  {
    DEBUG_DEBUG("Codec 4K/6K: Btn A/B pressed");

    // Switching between BRAW and ProRes
    if(btnAPressed)
    {
      if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW)
      {
        // Switch to ProRes
        PacketWriter::writeCodec(camera->lastKnownProRes, &cameraConnection);
      }
      else
      {
        // Switch to BRAW
        PacketWriter::writeCodec(camera->lastKnownBRAWIsBitrate ? camera->lastKnownBRAWBitrate : camera->lastKnownBRAWQuality, &cameraConnection);
      }

      tappedAction = true;
    }
    else if(btnBPressed)
    {
        // Current setting
        std::string currentCodecString = currentCodec.to_string();

      // Change the setting on the current Codec
      if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW)
      {
          if(currentCodecString == "BRAW 3:1")
          {
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAW5_1), &cameraConnection);
          }
          else if(currentCodecString == "BRAW 5:1")
          {
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAW8_1), &cameraConnection);
          }
          else if(currentCodecString == "BRAW 8:1")
          {
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAW12_1), &cameraConnection);
          }
          else if(currentCodecString == "BRAW 12:1")
          {
            // Switch across to Constant Quality
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAWQ0), &cameraConnection);
          }
          else if(currentCodecString == "BRAW Q0")
          {
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAWQ1), &cameraConnection);
          }
          else if(currentCodecString == "BRAW Q1")
          {
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAWQ3), &cameraConnection);
          }
          else if(currentCodecString == "BRAW Q3")
          {
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAWQ5), &cameraConnection);
          }
          else if(currentCodecString == "BRAW Q5")
          {
            // Switch across to Constant Bitrate
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAW3_1), &cameraConnection);
          }
      }
      else
      {
          if(currentCodecString == "ProRes HQ")
          {
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::ProRes, CCUPacketTypes::CodecVariants::kProRes422), &cameraConnection);
          }
          else if(currentCodecString == "ProRes 422")
          {
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::ProRes, CCUPacketTypes::CodecVariants::kProResLT), &cameraConnection);
          }
          else if(currentCodecString == "ProRes LT")
          {
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::ProRes, CCUPacketTypes::CodecVariants::kProResProxy), &cameraConnection);
          }
          else if(currentCodecString == "ProRes PXY")
          {
            PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::ProRes, CCUPacketTypes::CodecVariants::kProResHQ), &cameraConnection);
          }
      }

      tappedAction = true;
    }
  }

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen Codec 4K/6K Refreshed.");

  sprite->fillScreen(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // We need to have the Codec information to show the screen
  if(!camera->hasCodec())
  {
    sprite->setTextColor(TFT_WHITE);
    sprite->drawString("NO CODEC INFO.", 30, 9);

    return;
  }

  // Codec label
  sprite->setTextColor(TFT_WHITE);
  sprite->drawString("CODEC", 30, 9, &AgencyFB_Bold9pt7b);

  // BRAW and ProRes selector buttons

  // BRAW
  sprite->fillSmoothRoundRect(20, 30, 145, 40, 5, (currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawRoundRect(20, 30, 145, 40, 3, (currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString("BRAW", 93, 41);

  // ProRes
  sprite->fillSmoothRoundRect(170, 30, 145, 40, 5, (currentCodec.basicCodec == CCUPacketTypes::BasicCodec::ProRes ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawRoundRect(170, 30, 145, 40, 3, (currentCodec.basicCodec == CCUPacketTypes::BasicCodec::ProRes ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString("ProRes", 242, 41);

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
    sprite->fillSmoothRoundRect(20, 75, 145, 40, 3, (isConstantBitrate ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("BITRATE", 93, 80);
    sprite->drawCentreString("CONSTANT", 93, 102, &Lato_Regular5pt7b);

    // Constant Quality
    sprite->fillSmoothRoundRect(170, 75, 145, 40, 3, (!isConstantBitrate ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("QUALITY", 242, 80);
    sprite->drawCentreString("CONSTANT", 242, 102, &Lato_Regular5pt7b);

    // Setting 1 of 4
    std::string optionString = (isConstantBitrate ? "3:1" : "Q0");
    sprite->fillSmoothRoundRect(20, 120, 70, 40, 3, (optionString == qualityBitrateSetting ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString(optionString.c_str(), 55, 131);

    // Setting 2 of 4
    optionString = (isConstantBitrate ? "5:1" : "Q1");
    sprite->fillSmoothRoundRect(95, 120, 70, 40, 3, (optionString == qualityBitrateSetting ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString(optionString.c_str(), 130, 131);

  //   Setting 3 of 4
    optionString = (isConstantBitrate ? "8:1" : "Q3");
    sprite->fillSmoothRoundRect(170, 120, 70, 40, 3, (optionString == qualityBitrateSetting ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString(optionString.c_str(), 205, 131);

    // Setting 4 of 4
    optionString = (isConstantBitrate ? "12:1" : "Q5");
    sprite->fillSmoothRoundRect(245, 120, 70, 40, 3, (optionString == qualityBitrateSetting ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString(optionString.c_str(), 280, 131);
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
    sprite->fillSmoothRoundRect(20, 75, 145, 40, 3, (currentProResSetting == proResLabel ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString(proResLabel.c_str(), 93, 87);

    // 422
    proResLabel = "422";
    sprite->fillSmoothRoundRect(170, 75, 145, 40, 3, (currentProResSetting == proResLabel ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString(proResLabel.c_str(), 242, 87);

    // LT
    proResLabel = "LT";
    sprite->fillSmoothRoundRect(20, 120, 145, 40, 3, (currentProResSetting == proResLabel ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString(proResLabel.c_str(), 93, 131);

    // PXY
    proResLabel = "PXY";
    sprite->fillSmoothRoundRect(170, 120, 145, 40, 3, (currentProResSetting == proResLabel ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString(proResLabel.c_str(), 242, 131);
  }

  sprite->pushSprite(0, 0);
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

  bool tappedAction = false;
  if(btnAPressed || btnBPressed)
  {
    DEBUG_DEBUG("Resolution4K: Btn A/B pressed");

    String currentRes = currentRecordingFormat.frameDimensionsShort_string().c_str(); // "HD", "2.6K 16:9", "4K DCI", "4K UHD", "HD"

    if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW)
    {
      int width = 0;
      int height = 0;
      bool window = false;

      if(currentRes == "4K DCI")
      {
        if(btnAPressed)
        {
          // 4K 2.4:1
          width = 4096; height = 1720;
          window = true;
        }
        else
        {
          // HD
          width = 1920; height = 1080;
          window = true;
        }
      }
      else if(currentRes == "4K 2.4:1")
      {
        if(btnAPressed)
        {
          // 4K UHD
          width = 3840; height = 2160;
          window = true;
        }
        else
        {
          // 4K DCI
          width = 4096; height = 2160;
        }
      }
      else if(currentRes == "4K UHD")
      {
        if(btnAPressed)
        {
          // 2.8K Ana
          width = 2880; height = 2160;
          window = true;
        }
        else
        {
          // 4K 2.4:1
          width = 4096; height = 1720;
          window = true;
        }
      }
      else if(currentRes == "2.8K Ana")
      {
        if(btnAPressed)
        {
          // 2.6K 16:9
          width = 2688; height = 1512;
          window = true;
        }
        else
        {
          // 4K UHD
          width = 3840; height = 2160;
          window = true;
        }
      }
      else if(currentRes == "2.6K 16:9")
      {
        if(btnAPressed)
        {
          // HD
          width = 1920; height = 1080;
          window = true;
        }
        else
        {
          // 2.8K Ana
          width = 2880; height = 2160;
          window = true;
        }
      }
      else // HD
      {
        if(btnAPressed)
        {
          // 4K DCI
          width = 4096; height = 2160;
        }
        else
        {
          // 2.6K 16:9
          width = 2688; height = 1512;
          window = true;
        }
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

      DEBUG_DEBUG("Before change:");
      DEBUG_DEBUG(currentRecordingFormat.frameDimensionsShort_string().c_str());
      DEBUG_DEBUG(currentRecordingFormat.frameRate_string().c_str());
      DEBUG_DEBUG("Frame Rate: %hi", currentRecordingFormat.frameRate);
      DEBUG_DEBUG("Off Speed Frame Rate: %hi", currentRecordingFormat.offSpeedFrameRate);
      DEBUG_DEBUG("Width: %i", currentRecordingFormat.width);
      DEBUG_DEBUG("Height: %i", currentRecordingFormat.height);
      DEBUG_DEBUG("mRateEnabled: %s", currentRecordingFormat.mRateEnabled ? "Yes" : "No");
      DEBUG_DEBUG("offSpeedEnabled: %s", currentRecordingFormat.offSpeedEnabled ? "Yes" : "No");
      DEBUG_DEBUG("interlacedEnabled: %s", currentRecordingFormat.interlacedEnabled ? "Yes" : "No");
      DEBUG_DEBUG("windowedModeEnabled: %s", currentRecordingFormat.windowedModeEnabled ? "Yes" : "No");
      DEBUG_DEBUG("sensorMRateEnabled: %s", currentRecordingFormat.sensorMRateEnabled ? "Yes" : "No");

      if(currentRes == "4K DCI")
      {
        if(btnAPressed)
        {
          // 4K UHD
          width = 3840; height = 2160;
          window = true;
        }
        else
        {
          // HD
          width = 1920; height = 1080;
          window = true;
        }
      }
      else if(currentRes == "4K UHD")
      {
        if(btnAPressed)
        {
          // HD
          width = 1920; height = 1080;
          window = true;
        }
        else
        {
          // 4K DCI
          width = 4096; height = 2160;
          window = false;
        }
      }
      else // UHD
      {
        if(btnAPressed)
        {
          // 4K DCI
          width = 4096; height = 2160;
          window = false;
        }
        else
        {
          // 4K UHD
          width = 3840; height = 2160;
          window = true;
        }
      }

      if(width != 0)
      {
        // Resolution or Sensor Area selected, write to camera
        CCUPacketTypes::RecordingFormatData newRecordingFormat = currentRecordingFormat;
        newRecordingFormat.width = width;
        newRecordingFormat.height = height;
        newRecordingFormat.windowedModeEnabled = window;

        DEBUG_DEBUG("Attempting to change to:");
        DEBUG_DEBUG(newRecordingFormat.frameDimensionsShort_string().c_str());
        DEBUG_DEBUG(newRecordingFormat.frameRate_string().c_str());
        DEBUG_DEBUG("Frame Rate: %hi", newRecordingFormat.frameRate);
        DEBUG_DEBUG("Off Speed Frame Rate: %hi", newRecordingFormat.offSpeedFrameRate);
        DEBUG_DEBUG("Width: %i", newRecordingFormat.width);
        DEBUG_DEBUG("Height: %i", newRecordingFormat.height);
        DEBUG_DEBUG("mRateEnabled: %s", newRecordingFormat.mRateEnabled ? "Yes" : "No");
        DEBUG_DEBUG("offSpeedEnabled: %s", newRecordingFormat.offSpeedEnabled ? "Yes" : "No");
        DEBUG_DEBUG("interlacedEnabled: %s", newRecordingFormat.interlacedEnabled ? "Yes" : "No");
        DEBUG_DEBUG("windowedModeEnabled: %s", newRecordingFormat.windowedModeEnabled ? "Yes" : "No");
        DEBUG_DEBUG("sensorMRateEnabled: %s", newRecordingFormat.sensorMRateEnabled ? "Yes" : "No");

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

  sprite->fillScreen(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW)
  {
    // Main label
    sprite->setTextColor(TFT_WHITE);
    sprite->drawString("BRAW RESOLUTION", 30, 9, &AgencyFB_Bold9pt7b);

    String currentRes = currentRecordingFormat.frameDimensionsShort_string().c_str(); // "4K DCI", "4K 2.4:1", "4K UHD", "2.8K Ana", "2.6K 16:9", "HD"

    // 4K DCI
    String labelRes = "4K DCI";
    sprite->fillSmoothRoundRect(20, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("4K", 65, 35);
    sprite->drawCentreString("DCI", 65, 58, &Lato_Regular5pt7b);

    // 4K 2.4:1
    labelRes = "4K 2.4:1";
    sprite->fillSmoothRoundRect(115, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("4K", 160, 35);
    sprite->drawCentreString("2.4:1", 160, 58, &Lato_Regular5pt7b);

    // 4K UHD
    labelRes = "4K UHD";
    sprite->fillSmoothRoundRect(210, 30, 100, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("4K", 260, 35);
    sprite->drawCentreString("UHD", 260, 58, &Lato_Regular5pt7b);

    // 2.8K Ana
    labelRes = "2.8K Ana";
    sprite->fillSmoothRoundRect(20, 75, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("2.8K", 65, 81);
    sprite->drawCentreString("ANAMORPHIC", 65, 104, &Lato_Regular5pt7b);

    // 2.6K 16:9
    labelRes = "2.6K 16:9";
    sprite->fillSmoothRoundRect(115, 75, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("2.6K", 160, 81);
    sprite->drawCentreString("16:9", 160, 104, &Lato_Regular5pt7b);

    // HD
    labelRes = "HD";
    sprite->fillSmoothRoundRect(210, 75, 100, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("HD", 260, 87);

    // Sensor Area

    // Pocket 4K all are Windowed except 4K
    // sprite->drawSmoothRoundRect(20, 120, 5, 3, 290, 40, TFT_DARKGREY); // Optionally draw a rectangle around this info.
    sprite->drawCentreString(currentRes == "4K" ? "FULL SENSOR" :"SENSOR WINDOWED", 165, 129);
    sprite->drawCentreString(currentRecordingFormat.frameWidthHeight_string().c_str(), 165, 150, &Lato_Regular5pt7b);
  }
  else if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::ProRes)
  {
    // Main label
    sprite->setTextColor(TFT_WHITE);
    sprite->drawString("ProRes RESOLUTION", 30, 9, &AgencyFB_Bold9pt7b);

    String currentRes = currentRecordingFormat.frameDimensionsShort_string().c_str(); // "4K DCI", "4K UHD", "HD"

    // 4K
    String labelRes = "4K DCI";
    sprite->fillSmoothRoundRect(20, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("4K", 65, 36);
    sprite->drawCentreString("DCI", 65, 58, &Lato_Regular5pt7b);

    // 4K UHD
    labelRes = "4K UHD";
    sprite->fillSmoothRoundRect(115, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("UHD", 160, 41);

    // HD
    labelRes = "HD";
    sprite->fillSmoothRoundRect(210, 30, 100, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("HD", 260, 41);

    // Sensor Area

    // 4K DCI is Scaled from 5.7K, Ultra HD is Scaled from Full or 5.7K, HD is Scaled from Full, 5.7K, or 2.8K (however we can't change the 5.7K or 2.8K)

    sprite->drawString(currentRecordingFormat.frameWidthHeight_string().c_str(), 30, 90, &Lato_Regular5pt7b);
    sprite->drawString("SCALED / SENSOR AREA", 30, 105);

    if(currentRes == "4K DCI")
    {
      // Full sensor area only]
      sprite->fillSmoothRoundRect(20, 135, 90, 40, 3, TFT_DARKGREEN);
      sprite->drawCentreString("FULL", 65, 145);
    }
    else if(currentRes == "4K UHD")
    {
      // Full sensor area only]
      sprite->fillSmoothRoundRect(20, 135, 120, 40, 3, TFT_DARKGREEN);
      sprite->drawCentreString("WINDOW", 80, 145);
    }
    else
    {
      // HD

      // Scaled from Full, 2.6K or Windowed (however we can't tell which)
      sprite->fillSmoothRoundRect(20, 135, 290, 40, 3, TFT_DARKGREEN);
      sprite->drawCentreString("FULL / 2.6K / WINDOW", 165, 140);
      sprite->drawCentreString("CHECK/SET ON CAMERA", 165, 161, &Lato_Regular5pt7b);
    }
  }
  else
    DEBUG_ERROR("Resolution Pocket 4K - Codec not catered for.");

  sprite->pushSprite(0, 0);
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

  bool tappedAction = false;
  if(btnAPressed || btnBPressed)
  {
    DEBUG_DEBUG("Resolution6K: Btn A/B pressed");

    String currentRes = currentRecordingFormat.frameDimensionsShort_string().c_str();

    if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW)
    {
      int width = 0;
      int height = 0;
      bool window = false;

      if(currentRes == "6K")
      {
        if(btnAPressed)
        {
          // 6K 2.4:1
          width = 6144; height = 2560;
          window = true;
        }
        else
        {
          // 2.8K 17:9 - Note V8.1 width is 2880 not 2868
          width = 2880; height = 1512;
          window = true;
        }
      }
      else if(currentRes == "6K 2.4:1")
      {
        if(btnAPressed)
        {
          // 5.7K 17:9
          width = 5744; height = 3024;
          window = true;
        }
        else
        {
          // 6K
          width = 6144; height = 3456;
        }
      }
      else if(currentRes == "5.7K 17:9")
      {
        if(btnAPressed)
        {
          // 4K DCI
          width = 4096; height = 2160;
          window = true;
        }
        else
        {
          // 6K 2.4:1
          width = 6144; height = 2560;
          window = true;
        }
      }
      else if(currentRes == "4K DCI")
      {
        if(btnAPressed)
        {
          // 3.7K Anamorphic
          width = 3728; height = 3104;
          window = true;
        }
        else
        {
          // 5.7K 17:9
          width = 5744; height = 3024;
          window = true;
        }
      }
      else if(currentRes == "3.7K 6:5A")
      {
        if(btnAPressed)
        {
          // 2.8K 17:9 - Note V8.1 width is 2880 not 2868
          width = 2880; height = 1512;
          window = true;
        }
        else
        {
          // 4K DCI
          width = 4096; height = 2160;
          window = true;
        }
      }
      else // 2.8K 17:9
      {
        if(btnAPressed)
        {
          // 6K
          width = 6144; height = 3456;
        }
        else
        {
          // 3.7K Anamorphic
          width = 3728; height = 3104;
          window = true;
        }
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

      DEBUG_DEBUG("Before change:");
      DEBUG_DEBUG(currentRecordingFormat.frameDimensionsShort_string().c_str());
      DEBUG_DEBUG(currentRecordingFormat.frameRate_string().c_str());
      DEBUG_DEBUG("Frame Rate: %hi", currentRecordingFormat.frameRate);
      DEBUG_DEBUG("Off Speed Frame Rate: %hi", currentRecordingFormat.offSpeedFrameRate);
      DEBUG_DEBUG("Width: %i", currentRecordingFormat.width);
      DEBUG_DEBUG("Height: %i", currentRecordingFormat.height);
      DEBUG_DEBUG("mRateEnabled: %s", currentRecordingFormat.mRateEnabled ? "Yes" : "No");
      DEBUG_DEBUG("offSpeedEnabled: %s", currentRecordingFormat.offSpeedEnabled ? "Yes" : "No");
      DEBUG_DEBUG("interlacedEnabled: %s", currentRecordingFormat.interlacedEnabled ? "Yes" : "No");
      DEBUG_DEBUG("windowedModeEnabled: %s", currentRecordingFormat.windowedModeEnabled ? "Yes" : "No");
      DEBUG_DEBUG("sensorMRateEnabled: %s", currentRecordingFormat.sensorMRateEnabled ? "Yes" : "No");

      if(currentRes == "4K DCI")
      {
        if(btnAPressed)
        {
          // 4K UHD
          width = 3840; height = 2160;
          window = false;
        }
        else
        {
          // HD
          width = 1920; height = 1080;
          window = false;
        }
      }
      else if(currentRes == "4K UHD")
      {
        if(btnAPressed)
        {
          // HD
          width = 1920; height = 1080;
          window = false;
        }
        else
        {
          // 4K DCI
          width = 4096; height = 2160;
          window = true;
        }
      }
      else // UHD
      {
        if(btnAPressed)
        {
          // 4K DCI
          width = 4096; height = 2160;
          window = true;
        }
        else
        {
          // 4K UHD
          width = 3840; height = 2160;
          window = false;
        }
      }

      if(width != 0)
      {
        // Resolution or Sensor Area selected, write to camera
        CCUPacketTypes::RecordingFormatData newRecordingFormat = currentRecordingFormat;
        newRecordingFormat.width = width;
        newRecordingFormat.height = height;
        newRecordingFormat.windowedModeEnabled = window;

        DEBUG_DEBUG("Attempting to change to:");
        DEBUG_DEBUG(newRecordingFormat.frameDimensionsShort_string().c_str());
        DEBUG_DEBUG(newRecordingFormat.frameRate_string().c_str());
        DEBUG_DEBUG("Frame Rate: %hi", newRecordingFormat.frameRate);
        DEBUG_DEBUG("Off Speed Frame Rate: %hi", newRecordingFormat.offSpeedFrameRate);
        DEBUG_DEBUG("Width: %i", newRecordingFormat.width);
        DEBUG_DEBUG("Height: %i", newRecordingFormat.height);
        DEBUG_DEBUG("mRateEnabled: %s", newRecordingFormat.mRateEnabled ? "Yes" : "No");
        DEBUG_DEBUG("offSpeedEnabled: %s", newRecordingFormat.offSpeedEnabled ? "Yes" : "No");
        DEBUG_DEBUG("interlacedEnabled: %s", newRecordingFormat.interlacedEnabled ? "Yes" : "No");
        DEBUG_DEBUG("windowedModeEnabled: %s", newRecordingFormat.windowedModeEnabled ? "Yes" : "No");
        DEBUG_DEBUG("sensorMRateEnabled: %s", newRecordingFormat.sensorMRateEnabled ? "Yes" : "No");

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

  sprite->fillScreen(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW)
  {
    // Main label
    sprite->setTextColor(TFT_WHITE);
    sprite->drawString("BRAW RESOLUTION", 30, 9);

    String currentRes = currentRecordingFormat.frameDimensionsShort_string().c_str(); // "6K", "6K 2.4:1", "5.7K 17:9", "4K DCI", "3.7K 6:5A", "2.8K 17:9"

    // 6K
    String labelRes = "6K";
    sprite->fillSmoothRoundRect(20, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString(String(labelRes).c_str(), 65, 41);

    // 6K, 2.4:1
    labelRes = "6K 2.4:1";
    sprite->fillSmoothRoundRect(115, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("6K", 160, 36);
    sprite->drawCentreString("2.4:1", 160, 58, &Lato_Regular5pt7b);
  
    // 5.7K, 17:9
    labelRes = "5.7K 17:9";
    sprite->fillSmoothRoundRect(210, 30, 100, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("5.7K", 260, 36);
    sprite->drawCentreString("17:9", 260, 58, &Lato_Regular5pt7b);

    // 4K DCI
    labelRes = "4K DCI";
    sprite->fillSmoothRoundRect(20, 75, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("4K", 65, 82);
    sprite->drawCentreString("DCI", 65, 104, &Lato_Regular5pt7b);

    // 3.7K 6:5A
    labelRes = "3.7K 6:5A";
    sprite->fillSmoothRoundRect(115, 75, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("3.7K", 160, 82);
    sprite->drawCentreString("6:5 ANA", 160, 104, &Lato_Regular5pt7b);

    // 2.8K 17:9
    labelRes = "2.8K 17:9";
    sprite->fillSmoothRoundRect(210, 75, 100, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("2.8K", 260, 82);
    sprite->drawCentreString("17:9", 260, 104, &Lato_Regular5pt7b);

    // Sensor Area

    // Pocket 6K all are Windowed except 6K
    // sprite->drawSmoothRoundRect(20, 120, 5, 3, 290, 40, TFT_DARKGREY); // Optionally draw a rectangle around this info.
    sprite->drawCentreString(currentRes == "6K" ? "FULL SENSOR" :"SENSOR WINDOWED", 165, 129);
    sprite->drawCentreString(currentRecordingFormat.frameWidthHeight_string().c_str(), 165, 148, &Lato_Regular5pt7b);
  }
  else if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::ProRes)
  {
    // Main label
    sprite->setTextColor(TFT_WHITE);
    sprite->drawString("ProRes RESOLUTION", 30, 9);

    String currentRes = currentRecordingFormat.frameDimensionsShort_string().c_str(); // "4K DCI", "4K UHD", "HD"

    // 4K
    String labelRes = "4K DCI";
    sprite->fillSmoothRoundRect(20, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("4K", 65, 36);
    sprite->drawCentreString("DCI", 65, 58, &Lato_Regular5pt7b);

    // 4K UHD
    labelRes = "4K UHD";
    sprite->fillSmoothRoundRect(115, 30, 90, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("UHD", 160, 41);

    // HD
    labelRes = "HD";
    sprite->fillSmoothRoundRect(210, 30, 100, 40, 3, (currentRes == labelRes ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString("HD", 260, 41);

    // Sensor Area

    // 4K DCI is Scaled from 5.7K, Ultra HD is Scaled from Full or 5.7K, HD is Scaled from Full, 5.7K, or 2.8K (however we can't change the 5.7K or 2.8K)

    sprite->drawString(currentRecordingFormat.frameWidthHeight_string().c_str(), 30, 90, &Lato_Regular5pt7b);
    sprite->drawString("SCALED FROM SENSOR AREA", 30, 105);

    if(currentRes == "4K DCI")
    {
      // Full sensor area only]
      sprite->fillSmoothRoundRect(20, 130, 90, 40, 3, TFT_DARKGREEN);
      sprite->drawCentreString("FULL", 65, 141);
    }
    else if(currentRes == "4K UHD")
    {
      // Scaled from Full or 5.7K
      sprite->fillSmoothRoundRect(20, 130, 90, 40, 3, (!currentRecordingFormat.windowedModeEnabled ? TFT_DARKGREEN : TFT_DARKGREY));
      sprite->drawCentreString("FULL", 65, 141);

      // 5.7K
      sprite->fillSmoothRoundRect(115, 130, 90, 40, 3, (currentRecordingFormat.windowedModeEnabled ? TFT_DARKGREEN : TFT_DARKGREY));
      sprite->drawCentreString("5.3K", 160, 141);
    }
    else
    {
      // HD

      // Scaled from Full, 2.8K or 5.7K (however we can't tell if it's 2.8K or 5.7K)
      sprite->fillSmoothRoundRect(20, 130, 90, 40, 3, (!currentRecordingFormat.windowedModeEnabled ? TFT_DARKGREEN : TFT_DARKGREY));
      sprite->drawCentreString("FULL", 65, 141);

      // 2.8K 5.7K
      sprite->fillSmoothRoundRect(115, 130, 195, 40, 3, (currentRecordingFormat.windowedModeEnabled ? TFT_DARKGREEN : TFT_DARKGREY));
      sprite->drawCentreString("5.3K / 2.8K", 212, 136);
      sprite->drawCentreString("CHECK/SET ON CAMERA", 212, 156, &Lato_Regular5pt7b);
    }
  }
  else
    DEBUG_ERROR("Resolution Pocket 6K - Codec not catered for.");

  sprite->pushSprite(0, 0);
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

// Resolution screen - redirects to appropriate screen for camera
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

// Frame Rate screen for Pocket 4K
void Screen_Framerate4K(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Framerate;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // Pocket 4K Frame Rate

  // Get the current Resolution and Codec
  CCUPacketTypes::RecordingFormatData currentRecordingFormat;
  if(camera->hasRecordingFormat())
    currentRecordingFormat = camera->getRecordingFormat();

  // Get the current Codec values
  CodecInfo currentCodec = camera->getCodec();

  std::string currentFrameRate = currentRecordingFormat.frameRate_string();

  bool tappedAction = false;
  if(btnAPressed || btnBPressed)
  {
    DEBUG_DEBUG("FrameRate4K: Btn A/B pressed");

    short frameRate = 0;
    bool mRateEnabled = false;

    if(currentFrameRate == "23.98")
    {
      if(btnBPressed)
      {
        // 24
        frameRate = 24;
      }
      else
      {
        // 60
        frameRate = 60;
      }
    }
    else if(currentFrameRate == "24")
    {
      if(btnBPressed)
      {
        // 25
        frameRate = 25;
      }
      else
      {
        // 23.98
        frameRate = 24;
        mRateEnabled = true;
      }
    }
    else if(currentFrameRate == "25")
    {
      if(btnBPressed)
      {
        // 29.97
        frameRate = 30;
        mRateEnabled = true;
      }
      else
      {
        // 24
        frameRate = 24;
      }
    }
    else if(currentFrameRate == "29.97")
    {
      if(btnBPressed)
      {
        // 30
        frameRate = 30;
      }
      else
      {
        // 25
        frameRate = 25;
      }
    }
    else if(currentFrameRate == "30")
    {
      if(btnBPressed)
      {
        // 50
        frameRate = 50;
      }
      else
      {
        // 30
        frameRate = 30;
        mRateEnabled = true;
      }
    }
    else if(currentFrameRate == "50")
    {
      if(btnBPressed)
      {
        // 59.94
        frameRate = 60;
        mRateEnabled = true;
      }
      else
      {
        // 30
        frameRate = 30;
      }
    }
    else if(currentFrameRate == "59.94")
    {
      if(btnBPressed)
      {
        // 60
        frameRate = 60;
      }
      else
      {
        // 50
        frameRate = 50;
      }
    }
    else if(currentFrameRate == "60")
    {
      if(btnBPressed)
      {
        // 23.98
        frameRate = 24;
        mRateEnabled = true;
      }
      else
      {
        // 59.94
        frameRate = 60;
        mRateEnabled = true;
      }
    }

    if(frameRate != 0)
    {
      // Frame rate selected, write to camera
      CCUPacketTypes::RecordingFormatData newRecordingFormat = currentRecordingFormat;
      newRecordingFormat.frameRate = frameRate;
      newRecordingFormat.mRateEnabled = mRateEnabled;
      PacketWriter::writeRecordingFormat(newRecordingFormat, &cameraConnection);

      tappedAction = true;
    }
  }

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Frame Rate Pocket 4K Refreshed.");

  sprite->fillScreen(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // Main label
  sprite->setTextColor(TFT_WHITE);
  sprite->drawString("FRAME RATE", 30, 9, &AgencyFB_Bold9pt7b);

  // Output the current Codec and Resolution
  String codecRes = currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW ? "BRAW | " :"ProRes | ";
  codecRes.concat(currentRecordingFormat.frameDimensionsShort_string().c_str());
  sprite->drawString(codecRes, 30, 167);

  sprite->drawString(currentRecordingFormat.frameWidthHeight_string().c_str(), 30, 189, &Lato_Regular5pt7b);

  // 23.98
  std::string labelFR = "23.98";
  sprite->fillSmoothRoundRect(20, 30, 90, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(labelFR.c_str(), 65, 41);

  // 24
  labelFR = "24";
  sprite->fillSmoothRoundRect(115, 30, 90, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(labelFR.c_str(), 160, 41);

  // 25
  labelFR = "25";
  sprite->fillSmoothRoundRect(210, 30, 100, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(labelFR.c_str(), 260, 41);

  // 29.97
  labelFR = "29.97";
  sprite->fillSmoothRoundRect(20, 75, 90, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(labelFR.c_str(), 65, 87);

  // 30
  labelFR = "30";
  sprite->fillSmoothRoundRect(115, 75, 90, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(labelFR.c_str(), 160, 87);

  // 50
  labelFR = "50";
  sprite->fillSmoothRoundRect(210, 75, 100, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(labelFR.c_str(), 260, 87);

  // 59.94
  labelFR = "59.94";
  sprite->fillSmoothRoundRect(20, 120, 90, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(labelFR.c_str(), 65, 131);

  // 60
  labelFR = "60";
  sprite->fillSmoothRoundRect(115, 120, 90, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(labelFR.c_str(), 160, 131);

  sprite->pushSprite(0, 0);
}

// Frame Rate Screen for Pocket 6K
void Screen_Framerate6K(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Framerate;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // Pocket 6K Frame Rate

  // Get the current Resolution and Codec
  CCUPacketTypes::RecordingFormatData currentRecordingFormat;
  if(camera->hasRecordingFormat())
    currentRecordingFormat = camera->getRecordingFormat();

  // Get the current Codec values
  CodecInfo currentCodec = camera->getCodec();

  std::string currentFrameRate = currentRecordingFormat.frameRate_string();

  bool isFull6K = currentRecordingFormat.width == 6144 && currentRecordingFormat.height == 3456; // Are we in full 6K (only goes up to 50fps)

  bool tappedAction = false;
  if(btnAPressed || btnBPressed)
  {
    DEBUG_DEBUG("FrameRate6K: Btn A/B pressed");

    short frameRate = 0;
    bool mRateEnabled = false;

    if(currentFrameRate == "23.98")
    {
      if(btnBPressed)
      {
        // 24
        frameRate = 24;
      }
      else
      {
        if(isFull6K)
          frameRate = 50;
        else
          frameRate = 60;
      }
    }
    else if(currentFrameRate == "24")
    {
      if(btnBPressed)
      {
        // 25
        frameRate = 25;
      }
      else
      {
        // 23.98
        frameRate = 24;
        mRateEnabled = true;
      }
    }
    else if(currentFrameRate == "25")
    {
      if(btnBPressed)
      {
        // 29.97
        frameRate = 30;
        mRateEnabled = true;
      }
      else
      {
        // 24
        frameRate = 24;
      }
    }
    else if(currentFrameRate == "29.97")
    {
      if(btnBPressed)
      {
        // 30
        frameRate = 30;
      }
      else
      {
        // 25
        frameRate = 25;
      }
    }
    else if(currentFrameRate == "30")
    {
      if(btnBPressed)
      {
        // 50
        frameRate = 50;
      }
      else
      {
        // 30
        frameRate = 30;
        mRateEnabled = true;
      }
    }
    else if(currentFrameRate == "50")
    {
      if(btnBPressed)
      {
        if(isFull6K)
        {
          // 23.98
          frameRate = 24;
          mRateEnabled = true;
        }
        else
        {
          // 59.94
          frameRate = 60;
          mRateEnabled = true;
        }
      }
      else
      {
        // 30
        frameRate = 30;
      }
    }
    else if(currentFrameRate == "59.94")
    {
      if(btnBPressed)
      {
        // 60
        frameRate = 60;
      }
      else
      {
        // 50
        frameRate = 50;
      }
    }
    else if(currentFrameRate == "60")
    {
      if(btnBPressed)
      {
        // 23.98
        frameRate = 24;
        mRateEnabled = true;
      }
      else
      {
        // 59.94
        frameRate = 60;
        mRateEnabled = true;
      }
    }

    if(frameRate != 0)
    {
      // Frame rate selected, write to camera
      CCUPacketTypes::RecordingFormatData newRecordingFormat = currentRecordingFormat;
      newRecordingFormat.frameRate = frameRate;
      newRecordingFormat.mRateEnabled = mRateEnabled;
      PacketWriter::writeRecordingFormat(newRecordingFormat, &cameraConnection);

      tappedAction = true;
    }
  }

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Frame Rate Pocket 4K Refreshed.");

  sprite->fillScreen(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // Main label
  sprite->setTextColor(TFT_WHITE);
  sprite->drawString("FRAME RATE", 30, 9, &AgencyFB_Bold9pt7b);

  // Output the current Codec and Resolution
  String codecRes = currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW ? "BRAW | " :"ProRes | ";
  codecRes.concat(currentRecordingFormat.frameDimensionsShort_string().c_str());
  sprite->drawString(codecRes, 30, 167);

  sprite->drawString(currentRecordingFormat.frameWidthHeight_string().c_str(), 30, 189, &Lato_Regular5pt7b);

  // 23.98
  std::string labelFR = "23.98";
  sprite->fillSmoothRoundRect(20, 30, 90, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(labelFR.c_str(), 65, 41);

  // 24
  labelFR = "24";
  sprite->fillSmoothRoundRect(115, 30, 90, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(labelFR.c_str(), 160, 41);

  // 25
  labelFR = "25";
  sprite->fillSmoothRoundRect(210, 30, 100, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(labelFR.c_str(), 260, 41);

  // 29.97
  labelFR = "29.97";
  sprite->fillSmoothRoundRect(20, 75, 90, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(labelFR.c_str(), 65, 87);

  // 30
  labelFR = "30";
  sprite->fillSmoothRoundRect(115, 75, 90, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(labelFR.c_str(), 160, 87);

  // 50
  labelFR = "50";
  sprite->fillSmoothRoundRect(210, 75, 100, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawCentreString(labelFR.c_str(), 260, 87);

  if(!isFull6K)
  {
    // 59.94
    labelFR = "59.94";
    sprite->fillSmoothRoundRect(20, 120, 90, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString(labelFR.c_str(), 65, 131);

    // 60
    labelFR = "60";
    sprite->fillSmoothRoundRect(115, 120, 90, 40, 3, (currentFrameRate == labelFR ? TFT_DARKGREEN : TFT_DARKGREY));
    sprite->drawCentreString(labelFR.c_str(), 160, 131);
  }

  sprite->pushSprite(0, 0);}

// Frame Rate Screen for URSA Mini Pro G2
void Screen_FramerateURSAMiniProG2(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Resolution;

  auto camera = BMDControlSystem::getInstance()->getCamera();
  
  // TO DO
}

// Frame Rate Screen for URSA Mini Pro 12K
void Screen_FramerateURSAMiniPro12K(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Resolution;

  auto camera = BMDControlSystem::getInstance()->getCamera();
  
  // TO DO
}

// Frame Rate screen - redirects to appropriate screen for camera
void Screen_Framerate(bool forceRefresh = false)
{
  auto camera = BMDControlSystem::getInstance()->getCamera();

  if(camera->hasCodec())
  {
    if(camera->hasRecordingFormat())
    {
      if(camera->isPocket4K())
        Screen_Framerate4K(forceRefresh); // Pocket 4K
      else if(camera->isPocket6K())
        Screen_Framerate6K(forceRefresh); // Pocket 6K
      else if(camera->isURSAMiniProG2())
        Screen_FramerateURSAMiniProG2(forceRefresh); // URSA Mini Pro G2
      else if(camera->isURSAMiniPro12K())
        Screen_FramerateURSAMiniPro12K(forceRefresh); // URSA Mini Pro 12K
      else
        DEBUG_DEBUG("No Codec screen for this camera.");
    } 
    else
      Screen_Framerate4K(forceRefresh); // Handle no model name in 4K screen
  }
  else
    Screen_Framerate4K(forceRefresh); // If we don't have any codec info, we show the 4K screen that shows no codec
}


// Media screen for Pocket 4K
void Screen_Media4K6K(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Media;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // 3 Media Slots - CFAST, SD, USB

  bool tappedAction = false;
  if(btnAPressed || btnBPressed)
  {
    DEBUG_DEBUG("Media Pocket 4K/6K: Btn A/B pressed");

    // Make sure we have some media
    bool hasMedia = camera->getMediaSlots()[0].status != CCUPacketTypes::MediaStatus::None || camera->getMediaSlots()[1].status != CCUPacketTypes::MediaStatus::None || camera->getMediaSlots()[2].status != CCUPacketTypes::MediaStatus::None;

    if(hasMedia)
    {
      TransportInfo transportInfo = camera->getTransportMode();

      bool slotAvail1 = camera->getMediaSlots()[0].status != CCUPacketTypes::MediaStatus::None;
      bool slotAvail2 = camera->getMediaSlots()[1].status != CCUPacketTypes::MediaStatus::None;
      bool slotAvail3 = camera->getMediaSlots()[2].status != CCUPacketTypes::MediaStatus::None;

      // Make sure we have more than one available slot
      if(slotAvail1 + slotAvail2 + slotAvail3 > 1)
      {
        short changeToSlot = -1;
        short slotActive = 0;
        if(slotAvail1 && transportInfo.slots[0].active)
          slotActive = 1;
        else if(slotAvail2 && transportInfo.slots[1].active)
          slotActive = 2;
        else
          slotActive = 3;

        if(btnAPressed)
        {
          // Down to the next
          switch(slotActive)
          {
            case 1:
              changeToSlot = slotAvail2 ? 2 : 3;
              break;
            case 2:
              changeToSlot = slotAvail3 ? 3 : 1;
              break;
            case 3:
              changeToSlot = slotAvail1 ? 1 : 2;
              break;
          }
        }
        else
        {
          // Up to the previous
          switch(slotActive)
          {
            case 1:
              changeToSlot = slotAvail3 ? 3 : 2;
              break;
            case 2:
              changeToSlot = slotAvail1 ? 1 : 3;
              break;
            case 3:
              changeToSlot = slotAvail2 ? 2 : 1;
              break;
          }
        }

        // Change the media slot
        if(changeToSlot != -1)
        {
          transportInfo.slots[0].active = changeToSlot == 1 ? true : false;
          transportInfo.slots[1].active = changeToSlot == 2 ? true : false;
          transportInfo.slots[2].active = changeToSlot == 3 ? true : false;
          PacketWriter::writeTransportInfo(transportInfo, &cameraConnection);

          tappedAction = true;
        }
      }
      else
        DEBUG_DEBUG("Only 1 media available, can't change active media.");
    }
    else
      DEBUG_DEBUG("No media, can't change active media.");
  }

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen Media Pocket 4K/6K Refreshed.");

  sprite->fillScreen(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

    // Media label
  sprite->setTextColor(TFT_WHITE);
  sprite->drawString("MEDIA", 30, 9, &AgencyFB_Bold9pt7b);

  // CFAST
  BMDCamera::MediaSlot cfast = camera->getMediaSlots()[0];
  sprite->fillSmoothRoundRect(20, 30, 295, 40, 3, (cfast.active ? TFT_DARKGREEN : TFT_DARKGREY));
  if(cfast.StatusIsError()) sprite->drawRoundRect(20, 30, 295, 40, 3, (cfast.active ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawString("CFAST", 28, 47);
  if(cfast.status != CCUPacketTypes::MediaStatus::None) sprite->drawString(cfast.remainingRecordTimeString.c_str(), 155, 47);

  if(cfast.status != CCUPacketTypes::MediaStatus::None) sprite->drawString("REMAINING TIME", 155, 35, &Lato_Regular5pt7b);
  sprite->drawString("1", 300, 35, &Lato_Regular5pt7b);
  sprite->drawString(cfast.GetStatusString().c_str(), 28, 35, &Lato_Regular5pt7b);

  // SD
  BMDCamera::MediaSlot sd = camera->getMediaSlots()[1];
  sprite->fillSmoothRoundRect(20, 75, 295, 40, 3, (sd.active ? TFT_DARKGREEN : TFT_DARKGREY));
  if(sd.StatusIsError()) sprite->drawRoundRect(20, 75, 295, 40, 3, (sd.active ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawString("SD", 28, 92);
  if(sd.status != CCUPacketTypes::MediaStatus::None) sprite->drawString(sd.remainingRecordTimeString.c_str(), 155, 92);

  if(sd.status != CCUPacketTypes::MediaStatus::None) sprite->drawString("REMAINING TIME", 155, 80, &Lato_Regular5pt7b);
  sprite->drawString("2", 300, 80, &Lato_Regular5pt7b);
  sprite->drawString(sd.GetStatusString().c_str(), 28, 80, &Lato_Regular5pt7b);
  if(sd.StatusIsError()) sprite->setTextColor(TFT_WHITE);

  // USB
  BMDCamera::MediaSlot usb = camera->getMediaSlots()[2];
  sprite->fillSmoothRoundRect(20, 120, 295, 40, 3, (usb.active ? TFT_DARKGREEN : TFT_DARKGREY));
  if(usb.StatusIsError()) sprite->drawRoundRect(20, 120, 295, 40, 3, (usb.active ? TFT_DARKGREEN : TFT_DARKGREY));
  sprite->drawString("USB", 28, 137);
  if(usb.status != CCUPacketTypes::MediaStatus::None) sprite->drawString(usb.remainingRecordTimeString.c_str(), 155, 137);
  
  if(usb.status != CCUPacketTypes::MediaStatus::None) sprite->drawString("REMAINING TIME", 155, 125, &Lato_Regular5pt7b);
  sprite->drawString("3", 300, 125, &Lato_Regular5pt7b);
  sprite->drawString(usb.GetStatusString().c_str(), 28, 125, &Lato_Regular5pt7b);
  if(usb.StatusIsError()) sprite->setTextColor(TFT_WHITE);

  sprite->pushSprite(0, 0);
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

  bool tappedAction = false;
  if(btnAPressed || btnBPressed)
  {
    DEBUG_DEBUG("Lens: Btn A/B pressed");

    if(btnAPressed)
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

  sprite->fillScreen(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

    // Media label
  sprite->setTextColor(TFT_WHITE);
  sprite->drawString("LENS", 30, 9, &AgencyFB_Bold9pt7b);

  // M5GFX, set font here rather than on each drawString line
  sprite->setFont(&Lato_Regular11pt7b);

  sprite->drawString("LENS TYPE", 30, 53, &Lato_Regular5pt7b);
  if(camera->hasLensType())
  {
    sprite->setTextColor(TFT_LIGHTGREY);
    sprite->drawString(camera->getLensType().c_str(), 30, 35, &Lato_Regular6pt7b);

  }
  
  sprite->drawString("FOCAL LENGTH", 30, 98, &Lato_Regular5pt7b);
  if(camera->hasFocalLengthMM() || camera->hasLensFocalLength())
  {
    if(camera->hasFocalLengthMM())
    {
      auto focalLength = camera->getFocalLengthMM();
      std::string focalLengthMM = std::to_string(focalLength);
      std::string combined = focalLengthMM + "mm";

      sprite->drawString(combined.c_str(), 30, 75, &Lato_Regular12pt7b);
    }
    else
      sprite->drawString(camera->getLensFocalLength().c_str(), 30, 80, &Lato_Regular12pt7b);
  }

  sprite->drawString("LENS DISTANCE", 30, 143, &Lato_Regular5pt7b);
  if(camera->hasLensDistance())
  {
    sprite->drawString(camera->getLensDistance().c_str(), 30, 120, &Lato_Regular12pt7b);
  }

  sprite->drawString("APERTURE", 30, 188, &Lato_Regular5pt7b);
  if(camera->hasApertureFStopString())
  {
    sprite->drawString(camera->getApertureFStopString().c_str(), 30, 165, &Lato_Regular12pt7b);
  }

  sprite->pushSprite(0, 0);
}

// Start of TouchDesigner functions

// Splits the TouchDesigner string into the command and the value parts
std::vector<std::string> splitTDCommand(const std::string& command) {
  std::vector<std::string> parts;

  // Find the index of the colon character
  size_t colonIndex = command.find(':');

  if (colonIndex != std::string::npos) {
    // Split the command into two parts
    std::string commandPart = command.substr(0, colonIndex);
    std::string valuePart = command.substr(colonIndex + 1);

    // Add the parts to the vector
    parts.push_back(commandPart);
    parts.push_back(valuePart);
  }

  return parts;
}

// Upper case a string
std::string capitaliseString(const std::string& str) {
  std::string result = str;

  for (char& c : result) {
    c = std::toupper(c);
  }

  return result;
}

// Removes Null characters from the TouchDesigner strings (as it can send them through)
std::string removeNullCharacter(const std::string& str) {
    std::string result;
    
    for (char c : str) {
        if (c != '\0') {
            result += c;
        }
    }

    return result;
}

// Check if a string is an integer (whole number)
bool isInteger(const std::string& str) {
  if (str.empty()) {
    return false;
  }

  for (char c : str) {

    if (!std::isdigit(c)) {
      
      // Convert the character to its integer value (character code)
      int charCode = static_cast<int>(c);

      // Print the non-digit character and its character code
      Serial.print("[IsInteger NOT A DIGIT] character: ");
      Serial.print(c);
      Serial.print(" (Character code: ");
      Serial.print(charCode);
      Serial.println(")");

      return false;
    }
  }

  return true;
}


// Convert a string to an integer
int convertToInt(const std::string& str) {
  try {
    return std::stoi(str);
  } catch (const std::exception& e) {
    return -1; // Default value
  }
}

// Processes and runs TouchDesigner commands sent over Serial
void RunTouchDesignerCommand(std::string commandPart, std::string valuePart)
{
  commandPart = capitaliseString(commandPart);
  bool haveCamera = BMDControlSystem::getInstance()->hasCamera();

  Serial.println(String("TouchDesigner Command Received, ") + commandPart.c_str() + ":" + valuePart.c_str());

  if(commandPart == "RECORD" && haveCamera)
  {
    Serial.println("[Command RECORD]: Processing RECORD");

    auto camera = BMDControlSystem::getInstance()->getCamera();

    if(camera->hasTransportMode())
    {
      Serial.println("[Command RECORD]: Getting Transport Mode");

      auto transportInfo = camera->getTransportMode();

      Serial.println("[Command RECORD]: Have Transport Mode");

      valuePart = capitaliseString(valuePart);

      Serial.println("[Command RECORD]: Value capitalised:");
      Serial.println(valuePart.c_str());

      if(valuePart == "START")
      {
        Serial.println("[Command RECORD]: Value part is START.");

        if(!camera->isRecording)
        {
          transportInfo.mode = CCUPacketTypes::MediaTransportMode::Record;
          PacketWriter::writeTransportInfo(transportInfo, &cameraConnection);
          Serial.println("[Command RECORD]: Start Recording sent to camera");
        }
        else
          Serial.println("[Command RECORD]: Camera already recording, can't send.");
      }
      else if(valuePart == "STOP")
      {
        Serial.println("[Command RECORD]: Value part is STOP.");

        if(camera->isRecording)
        {
          transportInfo.mode = CCUPacketTypes::MediaTransportMode::Preview;
          PacketWriter::writeTransportInfo(transportInfo, &cameraConnection);
          Serial.println("[Command RECORD]: Stop Recording sent to camera");
        }
        else
          Serial.println("[Command RECORD]: Camera was not recording, can't stop.");
      }
      else
        Serial.println("[Command RECORD]: Transport Mode has not been received, can't send.");
    }
    else
      Serial.println("[Command RECORD]: Transport Mode has not been received, can't send.");
  }
  else if(commandPart == "SHUTTERANGLE" && haveCamera)
  {
    int value = std::stoi(valuePart);

    if(value >= 1 and value <= 360)
    {
      PacketWriter::writeShutterAngle(value * 100, &cameraConnection); // Shutter angle is times 100
      Serial.println("Shutter Angle sent to camera");
    }
  }
  else if(commandPart == "SHUTTERSPEED" && haveCamera)
  {
    int value = std::stoi(valuePart);

    if(value >= 24 and value <= 2000)
    {
      PacketWriter::writeShutterSpeed(value, &cameraConnection); // Shutter speed command
      Serial.println("Shutter Speed sent to camera");
    }
  }
  else if(commandPart == "ISO" && haveCamera)
  {
    Serial.println("[Command ISO]: Processing ISO");

    if(isInteger(valuePart)) // Ensure we have a whole number
    {
      int value = std::stoi(valuePart);

      // Accepting values between 
      if(value >= 100 and value <= 25600)
      {
        PacketWriter::writeISO(value, &cameraConnection); // ISO command
        Serial.println("[Command ISO]: ISO sent to camera");
      }
      else
        Serial.println("[Command ISO]: Value was not between 100 and 25600, can't send.");
    }
    else
      Serial.println("[Command ISO]: Value was not an integer number.");
  }
  else if((commandPart == "TINT" || commandPart == "WHITEBALANCE") && haveCamera)
  {
    auto camera = BMDControlSystem::getInstance()->getCamera();

    // Get the current White Balance value
    int currentWB = 0;
    if(camera->hasWhiteBalance())
      currentWB = camera->getWhiteBalance();

    // Get the current Tint value
    int currentTint = 0;
    if(camera->hasTint())
      currentTint = camera->getTint();

    if(commandPart == "WHITEBALANCE" && haveCamera)
    {
      int value = std::stoi(valuePart);

      if(value >= 2500 and value <= 10000)
      {
        PacketWriter::writeWhiteBalance(value, currentTint, &cameraConnection); // White Balance command
        Serial.println("White Balance sent to camera");
      }
    }
    else if(commandPart == "TINT" && haveCamera)
    {
      int value = std::stoi(valuePart);

      if(value >= -50 and value <= 50)
      {
        PacketWriter::writeWhiteBalance(currentWB, value, &cameraConnection); // Tint command
        Serial.println("Tint sent to camera");
      }
    }
  }
  else if(commandPart == "BRAWQ" && haveCamera)
  {
    auto camera = BMDControlSystem::getInstance()->getCamera();
    CodecInfo currentCodec = camera->getCodec();

    if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW)
    {
      int value = std::stoi(valuePart);

      switch(value)
      {
        case 0: // Q0
          PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAWQ0), &cameraConnection);
          Serial.println("BRAW Q0 sent to camera");
          break;
        case 1: // Q1
          PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAWQ1), &cameraConnection);
          Serial.println("BRAW Q1 sent to camera");
          break;
        case 3: // Q3
          PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAWQ3), &cameraConnection);
          Serial.println("BRAW Q3 sent to camera");
          break;
        case 5: // Q5
          PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAWQ5), &cameraConnection);
          Serial.println("BRAW Q5 sent to camera");
          break;
        default:
          Serial.println("Unknown value for BRAW Quality");
      }
    }
    else
    Serial.println("Camera must be set to BRAW to change BRAW Quality setting");
  }
  else if(commandPart == "BRAWB" && haveCamera)
  {
    auto camera = BMDControlSystem::getInstance()->getCamera();
    CodecInfo currentCodec = camera->getCodec();

    if(currentCodec.basicCodec == CCUPacketTypes::BasicCodec::BRAW)
    {
      int value = std::stoi(valuePart);

      switch(value)
      {
        case 3: // 3:1
          PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAW3_1), &cameraConnection);
          Serial.println("BRAW 3:1 sent to camera");
          break;
        case 5: // 5:1
          PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAW5_1), &cameraConnection);
          Serial.println("BRAW 5:1 sent to camera");
          break;
        case 8: // 8:1
          PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAW8_1), &cameraConnection);
          Serial.println("BRAW 8:1 sent to camera");
          break;
        case 12: // 12:1
          PacketWriter::writeCodec(CodecInfo(CCUPacketTypes::BasicCodec::BRAW, CCUPacketTypes::CodecVariants::kBRAW12_1), &cameraConnection);
          Serial.println("BRAW 12:1 sent to camera");
          break;
        default:
          Serial.println("Unknown value for BRAW Bitrate");
      }
    }
    else
      Serial.println("Camera must be set to BRAW to change BRAW Quality setting");
  }
  else if(commandPart == "FPS" && haveCamera)
  {
    auto camera = BMDControlSystem::getInstance()->getCamera();

    // Get the current Resolution and Codec
    CCUPacketTypes::RecordingFormatData currentRecordingFormat;
    if(camera->hasRecordingFormat())
    {
      currentRecordingFormat = camera->getRecordingFormat();

      short frameRate = 0;
      bool mRateEnabled = false;

      if(valuePart == "23.98" || valuePart == "29.97" || valuePart == "59.94") // 23.98, 59.94
      {
        mRateEnabled = true;

        if(valuePart == "23.98")
          frameRate = 24;
        else if(valuePart == "29.97")
          frameRate = 30;
        else
          frameRate = 60;
      }
      else
      {
        frameRate = std::stoi(valuePart); // Integer based frame rate, 24, 25, 30, 50, 60
      }

      if(frameRate != 0)
      {
        // Frame rate selected, write to camera
        CCUPacketTypes::RecordingFormatData newRecordingFormat = currentRecordingFormat;
        newRecordingFormat.frameRate = frameRate;
        newRecordingFormat.mRateEnabled = mRateEnabled;
        PacketWriter::writeRecordingFormat(newRecordingFormat, &cameraConnection);
      }
    }
  }
  else if(commandPart == "IRIS" && haveCamera)
  {
    try {
        short irisApertureValue = CCUEncodingFunctions::ConvertFStopToCCUAperture(std::stof(valuePart));

        if(irisApertureValue != 0)
          PacketWriter::writeIris(irisApertureValue, &cameraConnection);
        else
          DEBUG_ERROR("<TD Not able to calculate aperture value from provided string>");
    } catch (const std::invalid_argument& e) {
        DEBUG_ERROR("<TD Not a valid IRIS value>");
    }
  }
  else
    Serial.println("[UNKNOWN TOUCHDESIGNER COMMAND");
}

void setup() {

  M5.begin();

  tft.setColorDepth(16);
  tft.setSwapBytes(true);

  tft.setTextDatum(TL_DATUM);
  tft.setTextPadding(tft.width());
  tft.setTextColor( TFT_WHITE);
  tft.setTextSize(1);
  tft.setFont(&Lato_Regular11pt7b);

  sprite = new LGFX_Sprite(&tft);
  sprite->setPsram(false);
  sprite->setColorDepth(BPP_SPRITE);
  sprite->setSwapBytes(true);
  sprite->setTextSize(1);
  sprite->setFont(&Lato_Regular11pt7b);


  /* To use Deep Sleep and use button B to wake up, the following code can be used.
      However, as the device won't maintain a connection while asleep, there isn't much point. Power button on the side can be used to shutdown.
      Power off: Quickly double-click the red power button on the left
      If you do want to go to deep sleep in your code, you can run the following code: esp_deep_sleep_start();

  pinMode(GPIO_NUM_38, INPUT_PULLUP); // Button B on M5Stack is GPIO 38, that's the one to wake up from sleep

  // Check if the ESP32 woke up from a deep sleep
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.begin(115200);
    Serial.println("Woke up from sleep!");
    // Additional actions can be performed here after waking up
  } else {
    Serial.begin(115200);
    Serial.println("First boot!");
    // Additional setup can be performed here on the first boot
  }
  // Configure wake-up source
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_38, LOW);
  */

  // SET DEBUG LEVEL
  Debug.setDebugLevel(DBG_VERBOSE);
  Debug.timestampOn();

  // Allow a timeout of 30 seconds for time for the pass key entry. It's slower with buttons
  esp_task_wdt_init(35, true);

  // Splash screen
  tft.pushImage(0, 0, IWIDTH, IHEIGHT, MPCSplash_M5Stack_CoreS3);

  // Prepare for Bluetooth connections and start scanning for cameras
  cameraConnection.initialise(&tft, IWIDTH, IHEIGHT); // Screen Pass Key entry
}

int memoryLoopCounter;
bool hitRecord = false; // Let's just hit record once.

void loop() {

  static unsigned long lastConnectedTime = 0;
  const unsigned long reconnectInterval = 5000;  // 5 seconds (milliseconds)

  unsigned long currentTime = millis();

  if ((cameraConnection.status == BMDCameraConnection::ConnectionStatus::Disconnected || cameraConnection.status == BMDCameraConnection::ConnectionStatus::FailedPassKey) && currentTime - lastConnectedTime >= reconnectInterval) {
    
    if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::Disconnected)
      DEBUG_VERBOSE("Disconnected for too long, trying to reconnect");
    else
      DEBUG_VERBOSE("Failed Pass Key, trying to reconnect");

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
        case Screens::WhiteBalanceTintWB:
        case Screens::WhiteBalanceTintT:
          Screen_WBTint(connectedScreenIndex == Screens::WhiteBalanceTintWB);
          break;
        case Screens::Codec:
          Screen_Codec();
          break;
        case Screens::Resolution:
          Screen_Resolution();
          break;
        case Screens::Framerate:
          Screen_Framerate();
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
    DEBUG_DEBUG("Cameras found!");

    cameraConnection.connect(cameraConnection.cameraAddresses[0]);

    if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::FailedPassKey)
      DEBUG_DEBUG("Loop - Failed Pass Key");

    // Clear the screen so we can show the dashboard cleanly
    tft.fillScreen(TFT_BLACK);

    lastConnectedTime = currentTime;
  }
  else if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::ScanningNoneFound)
  {
    DEBUG_VERBOSE("Status Scanning NONE Found. Marking as Disconnected.");
    cameraConnection.status = BMDCameraConnection::Disconnected;
    lastConnectedTime = currentTime;

    Screen_NoConnection();
  }

  // Buttons
  btnAPressed = false;
  btnBPressed = false;
  M5.update();

  // Check Serial for TouchDesigner input
  // 'A' = Button A press, 'B' = Button B press, 'C' = Button C press
  char TouchDesignerPress = ' ';

  if(tdCommandComplete)
  {
    std::vector<std::string> commandParts = splitTDCommand(tdCommand);

    std::string commandPart = commandParts[0];
    std::string valuePart = commandParts[1];

    // Process button presses
    if(commandPart == "BUTTON")
    {
      if(valuePart == "A")
        TouchDesignerPress = 'A';
      else if(valuePart == "B")
        TouchDesignerPress = 'B';
      else if(valuePart == "C")
        TouchDesignerPress = 'C';
    }
    else
    {
      // It's not a Button command, so let's process it in a separate function
      RunTouchDesignerCommand(commandPart, removeNullCharacter(valuePart));
    }

    // Reset ready for the next command
    tdCommandComplete = false;
  }

  if(M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed() || TouchDesignerPress != ' ')
  {
    // Left button function changes on context
    // Only handle dashboard here
    if(M5.BtnA.wasPressed() || TouchDesignerPress == 'A')
    {
      DEBUG_DEBUG("Button A");

      switch(connectedScreenIndex)
      {
        case Screens::Dashboard:
        case Screens::Recording:
        case Screens::ISO:
        case Screens::ShutterAngleSpeed:
        case Screens::WhiteBalanceTintWB:
        case Screens::WhiteBalanceTintT:
        case Screens::Codec:
        case Screens::Resolution:
        case Screens::Framerate:
        case Screens::Media:
        case Screens::Lens:
          // Indicate to the other screens the first button has been pressed
          btnAPressed = true;
          break;
      }
    }
    else if(M5.BtnB.wasPressed() || TouchDesignerPress == 'B')
    {
      DEBUG_DEBUG("Button B");

      switch(connectedScreenIndex)
      {
        case Screens::Dashboard:
        case Screens::Recording:

          // RECORD START/STOP FOR DASHBOARD AND RECORDING SCREEN
          if(connectedScreenIndex == Screens::Dashboard || connectedScreenIndex == Screens::Recording)
          {
            // Do we have a camera instance created (happens when connected)
            if(BMDControlSystem::getInstance()->hasCamera())
            {
                // Get the camera instance so we can check the state of it
                auto camera = BMDControlSystem::getInstance()->getCamera();

                // Only hit record if we have the Transport Mode info (knowing if it's recording) and we're not already recording.
                if(camera->hasTransportMode())
                {
                    // Record button
                    DEBUG_VERBOSE("Record Start/Stop");

                    auto transportInfo = camera->getTransportMode();

                    if(!camera->isRecording)
                      transportInfo.mode = CCUPacketTypes::MediaTransportMode::Record;
                    else
                      transportInfo.mode = CCUPacketTypes::MediaTransportMode::Preview;

                    // Send the packet to the camera to start recording
                    PacketWriter::writeTransportInfo(transportInfo, &cameraConnection);
                }
            }
          }
          break;
        case Screens::ISO:
        case Screens::ShutterAngleSpeed:
        case Screens::WhiteBalanceTintWB:
        case Screens::WhiteBalanceTintT:
        case Screens::Codec:
        case Screens::Resolution:
        case Screens::Framerate:
        case Screens::Media:
        case Screens::Lens:
          // Indicate to the other screens the second button has been pressed
          btnBPressed = true;
          break;
      }
    }
    else if(M5.BtnC.wasPressed() || TouchDesignerPress == 'C')
    {
      DEBUG_DEBUG("Button C > NEXT SCREEN");

      switch(connectedScreenIndex)
      {
        case Screens::Dashboard:
          connectedScreenIndex = Screens::Recording;
          break;
        case Screens::Recording:
          connectedScreenIndex = Screens::ISO;
          break;
        case Screens::ISO:
          connectedScreenIndex = Screens::ShutterAngleSpeed;
          break;
        case Screens::ShutterAngleSpeed:
          connectedScreenIndex = Screens::WhiteBalanceTintWB;
          break;
        case Screens::WhiteBalanceTintWB:
          connectedScreenIndex = Screens::WhiteBalanceTintT;
          break;
        case Screens::WhiteBalanceTintT:
          connectedScreenIndex = Screens::Codec;
          break;
        case Screens::Codec:
          connectedScreenIndex = Screens::Resolution;
          break;
        case Screens::Resolution:
          connectedScreenIndex = Screens::Framerate;
          break;
        case Screens::Framerate:
          connectedScreenIndex = Screens::Media;
          break;
        case Screens::Media:
          connectedScreenIndex = Screens::Lens;
          break;
        case Screens::Lens:
          connectedScreenIndex = Screens::Dashboard;
          break;
      }

      lastRefreshedScreen = 0; // Forces a refresh
    }
  }

  delay(5);
}

void serialEvent()
{
  while (Serial.available()) {
    char incomingChar = (char)Serial.read();

    // Check if the incoming character is the start of the string
    if (incomingChar == '(') {
      tdCommand = ""; // Clear the previous string
    }
    // Check if the incoming character is the end of the string
    else if (incomingChar == ')') {

      // Ensure that we have a colon in the string
      if(tdCommand.find(':') != std::string::npos)
      {
        tdCommandComplete = true; // We're good to go to process this command
      }
      else
      {
        // No colon, so we'll clear the command and try and start again
        tdCommand == "";
        tdCommandComplete = false;
      }
    }
    // Append the character to the input string
    else {
      if (tdCommand.length() < tdMaxLength) {
        tdCommand += incomingChar;
      }
    }
  }
}