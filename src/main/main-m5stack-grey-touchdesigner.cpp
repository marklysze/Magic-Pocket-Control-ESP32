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
// Available commands are:
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
//
// Want to create your own commands and actions - see the function "RunTouchDesignerCommand" in this file


#define USING_TFT_ESPI 0        // Not using the TFT_eSPI graphics library <-- must include this in every main file, 0 = not using, 1 = using
#define USING_M5GFX 0           // Using the M5GFX library with touch screen
#define USING_M5_BUTTONS 1   // Using the M5GFX library with the 3 buttons (buttons A, B, C)

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

// M5
#include "M5GFX.h"
#include "M5Unified.h"
// static M5GFX display;
// M5Canvas window(&display);
M5GFX& window = M5.Display; // We'll directly use the display for writing rather than a sprite/M5Canvas, we'll use the alias window so code can remain similar between other devices
// static M5Canvas spritePassKey(&display);

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
    DEBUG_DEBUG("Screen_Common");

    // Sidebar colour
    window.fillRect(0, 0, 13, IHEIGHT, sideBarColour);
    window.fillRect(13, 0, 2, IHEIGHT, TFT_DARKGREY);

    if(BMDControlSystem::getInstance()->hasCamera())
    {
      auto camera = BMDControlSystem::getInstance()->getCamera();

      window.setTextColor(TFT_WHITE);

      if(connectedScreenIndex == Screens::Dashboard)
      {
        // Dashboard Bottom Buttons
        window.fillSmoothRoundRect(30, 210, 80, 40, 3, TFT_DARKCYAN);
        window.drawCenterString("TBD", 70, 217, &AgencyFB_Bold9pt7b);

        if(camera->isRecording)
          window.fillSmoothCircle(IWIDTH / 2, 240, 30, TFT_RED);
        else
        {
          // Two outlines to make it a bit thicker
          window.drawCircle(IWIDTH / 2, 240, 30, TFT_RED);
          window.drawCircle(IWIDTH / 2, 240, 29, TFT_RED);
        }
      }
      else if(connectedScreenIndex == Screens::Recording)
      {
        // Recording Screen Bottom Buttons
        window.fillSmoothRoundRect(30, 210, 80, 40, 3, TFT_DARKCYAN);
        window.drawCenterString("TBD", 70, 217, &AgencyFB_Bold9pt7b);

        if(camera->isRecording)
          window.fillSmoothCircle(IWIDTH / 2, 240, 30, TFT_RED);
        else
        {
          // Two outlines to make it a bit thicker
          window.drawCircle(IWIDTH / 2, 240, 30, TFT_RED);
          window.drawCircle(IWIDTH / 2, 240, 29, TFT_RED);
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
          case Screens::Codec:
          case Screens::Resolution:
          case Screens::Media:
          case Screens::Lens:
            window.fillSmoothRoundRect(30, 210, 170, 40, 3, TFT_DARKCYAN);
            window.fillTriangle(60, 235, 70, 215, 50, 215, TFT_WHITE); // Up Arrow
            window.fillTriangle(150, 235, 170, 235, 160, 215, TFT_WHITE); // Down Arrow
            break;
          case Screens::WhiteBalanceTintWB:
            // White Balance shows Presets or increment
            window.fillSmoothRoundRect(30, 210, 80, 40, 3, TFT_DARKCYAN);
            window.drawCenterString("PRESET", 70, 217, &AgencyFB_Bold9pt7b);

            window.fillSmoothRoundRect(120, 210, 80, 40, 3, TFT_DARKCYAN);
            window.drawCenterString("+100", 160, 217, &AgencyFB_Bold9pt7b);
            break;
        }
      }

      // Common Next Button
      window.fillSmoothRoundRect(220, 210, 70, 40, 3, TFT_DARKCYAN);
      window.drawCenterString("NEXT", 255, 217, &AgencyFB_Bold9pt7b);
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
  DEBUG_DEBUG("Screen_NoConnection");

  // The camera to connect to.
  int connectToCameraIndex = -1;

  connectedScreenIndex = Screens::NoConnection;

  // Background 
  window.pushImage(0, 0, IWIDTH, IHEIGHT, MPCSplash_M5Stack_CoreS3);

  // Black background for text and Bluetooth Logo
  window.fillRect(0, 3, IWIDTH, 51, TFT_BLACK);

  // Bluetooth Image
  window.pushImage(26, 6, 30, 46, Wikipedia_Bluetooth_30x46);

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
      DEBUG_DEBUG("NoConnection - Disconnected");
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

    window.pushImage(33 + (count * 125) + (count * 10), 69, 110, 61, blackmagic_pocket_4k_110x61);

    window.drawString(cameraConnection.cameraAddresses[count].toString().c_str(), 33 + (count * 125) + (count * 10), 144, &Lato_Regular6pt7b);
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
  std::vector<int> Options_ISO = {200, 400, 640, 800, 1250, 3200, 8000, 12800};
  int Options_ISO_SelectingIndex = -1; // when using up/down buttons keeps track of what option you are navigating to
  if(btnAPressed || btnBPressed)
  {
  }
  // If we have a tap, we should determine if it is on anything
  /*
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
  */

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh)
  // if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen Dashboard Refreshing.");

  if(cameraConnection.getInitialPayloadTime() != ULONG_MAX)
    window.fillScreen(TFT_BLACK);

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
  if(camera->hasShutterAngle() || camera->hasShutterSpeed())
  {
    window.fillSmoothRoundRect(20 + xshift, 5, 75, 65, 3, TFT_DARKGREY);
    window.setTextColor(TFT_WHITE);

    if(camera->shutterValueIsAngle && camera->hasShutterAngle())
    {
      // Shutter Angle
      int currentShutterAngle = camera->getShutterAngle();
      float ShutterAngleFloat = currentShutterAngle / 100.0;

      window.drawCentreString(String(ShutterAngleFloat, (currentShutterAngle % 100 == 0 ? 0 : 1)), 58 + xshift, 23);
    }
    else if(camera->hasShutterSpeed())
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

  window.fillScreen(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // M5GFX, set font here rather than on each drawString line
  window.setFont(&Lato_Regular11pt7b);

  // Record button
  if(camera->isRecording) window.fillSmoothCircle(257, 63, 58, TFT_RED); // Recording solid
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

      // If we don't have a selected value yet, set to the current one
      if(Options_ISO_SelectingIndex == -1)
      {
        auto iter = std::find(Options_ISO.begin(), Options_ISO.end(), currentValue);
        int index = (iter != Options_ISO.end()) ? std::distance(Options_ISO.begin(), iter) : -1;

        if(index != -1)
          Options_ISO_SelectingIndex = index;
      }

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

  window.fillScreen(TFT_BLACK);

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
  if(Options_ISO_SelectingIndex == 0) window.drawRoundRect(20, 30, 90, 40, 1, TFT_WHITE);
  window.drawCentreString(String(labelISO).c_str(), 65, 41);

  // 400
  labelISO = 400;
  window.fillSmoothRoundRect(115, 30, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ISO_SelectingIndex == 1) window.drawRoundRect(115, 30, 90, 40, 1, TFT_WHITE);
  window.drawCentreString(String(labelISO).c_str(), 160, 36);
  window.drawCentreString("NATIVE", 160, 59, &Lato_Regular5pt7b);

  // 8000
  labelISO = 8000;
  window.fillSmoothRoundRect(210, 30, 100, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ISO_SelectingIndex == 6) window.drawRoundRect(210, 30, 100, 40, 1, TFT_WHITE);
  window.drawCentreString(String(labelISO).c_str(), 260, 41);

  // 640
  labelISO = 640;
  window.fillSmoothRoundRect(20, 75, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ISO_SelectingIndex == 2) window.drawRoundRect(20, 75, 90, 40, 1, TFT_WHITE);
  window.drawCentreString(String(labelISO).c_str(), 65, 87);

  // 800
  labelISO = 800;
  window.fillSmoothRoundRect(115, 75, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ISO_SelectingIndex == 3) window.drawRoundRect(115, 75, 90, 40, 1, TFT_WHITE);
  window.drawCentreString(String(labelISO).c_str(), 160, 87);

  // 12800
  labelISO = 12800;
  window.fillSmoothRoundRect(210, 75, 100, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ISO_SelectingIndex == 7) window.drawRoundRect(210, 75, 100, 40, 1, TFT_WHITE);
  window.drawCentreString(String(labelISO).c_str(), 260, 87);

  // 1250
  labelISO = 1250;
  window.fillSmoothRoundRect(20, 120, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ISO_SelectingIndex == 4) window.drawRoundRect(20, 120, 90, 40, 1, TFT_WHITE);
  window.drawCentreString(String(labelISO).c_str(), 65, 131);

  // 3200
  labelISO = 3200;
  window.fillSmoothRoundRect(115, 120, 90, 40, 3, (currentISO == labelISO ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ISO_SelectingIndex == 5) window.drawRoundRect(115, 120, 90, 40, 1, TFT_WHITE);
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

  window.fillScreen(TFT_BLACK);

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
  if(Options_ShutterAngle_SelectingIndex == 0) window.drawRoundRect(20, 30, 90, 40, 1, TFT_WHITE);
  window.drawCentreString(String(labelShutterAngle / 100).c_str(), 65, 41);

  // 60
  labelShutterAngle = 6000;
  window.fillSmoothRoundRect(115, 30, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ShutterAngle_SelectingIndex == 1) window.drawRoundRect(115, 30, 90, 40, 1, TFT_WHITE);
  window.drawCentreString(String(labelShutterAngle / 100).c_str(), 160, 41);

  // 90
  labelShutterAngle = 9000;
  window.fillSmoothRoundRect(210, 30, 100, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ShutterAngle_SelectingIndex == 2) window.drawRoundRect(210, 30, 100, 40, 1, TFT_WHITE);
  window.drawCentreString(String(labelShutterAngle / 100).c_str(), 260, 41);

  // 120
  labelShutterAngle = 12000;
  window.fillSmoothRoundRect(20, 75, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ShutterAngle_SelectingIndex == 3) window.drawRoundRect(20, 75, 90, 40, 1, TFT_WHITE);
  window.drawCentreString(String(labelShutterAngle / 100).c_str(), 65, 87);

  // 150
  labelShutterAngle = 15000;
  window.fillSmoothRoundRect(115, 75, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ShutterAngle_SelectingIndex == 4) window.drawRoundRect(115, 75, 90, 40, 1, TFT_WHITE);
  window.drawCentreString(String(labelShutterAngle / 100).c_str(), 160, 87);

  // 180 (with a border around it)
  labelShutterAngle = 18000;
  window.fillSmoothRoundRect(210, 75, 100, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  window.drawRoundRect(210, 75, 100, 40, 3, TFT_DARKGREEN);
  if(Options_ShutterAngle_SelectingIndex == 5) window.drawRoundRect(210, 75, 100, 40, 1, TFT_WHITE);
  window.drawCentreString(String(labelShutterAngle / 100).c_str(), 260, 87);

  // 270
  labelShutterAngle = 27000;
  window.fillSmoothRoundRect(20, 120, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ShutterAngle_SelectingIndex == 6) window.drawRoundRect(20, 120, 90, 40, 1, TFT_WHITE);
  window.drawCentreString(String(labelShutterAngle / 100).c_str(), 65, 131);

  // 360
  labelShutterAngle = 36000;
  window.fillSmoothRoundRect(115, 120, 90, 40, 3, (currentShutterAngle == labelShutterAngle ? TFT_DARKGREEN : TFT_DARKGREY));
  if(Options_ShutterAngle_SelectingIndex == 7) window.drawRoundRect(115, 120, 90, 40, 1, TFT_WHITE);
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


  window.fillScreen(TFT_BLACK);

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

  window.drawString("SHUTTER SPEED", 30, 9, &AgencyFB_Bold9pt7b);

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

  window.fillScreen(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // ISO label
  window.setTextColor(TFT_WHITE);
  window.drawString(editWB ? "WHITE BALANCE" : "TINT", 30, 9, &AgencyFB_Bold9pt7b);
  window.drawCentreString("TINT", 54, 132, &AgencyFB_Bold9pt7b);

  // Bright, 5600K
  int lblWBKelvin = 5600;
  int lblTint = 10;
  window.fillSmoothRoundRect(20, 30, 70, 40, 3, (currentWB == lblWBKelvin && currentTint == lblTint ? TFT_DARKGREEN : TFT_DARKGREY));
  if(currentWB == lblWBKelvin && currentTint == lblTint)
    window.pushImage(40, 35, 30, 30, WBBrightBG);
  else
    window.pushImage(40, 35, 30, 30, WBBright);

  // Incandescent, 3200K
  lblWBKelvin = 3200;
  lblTint = 0;
  window.fillSmoothRoundRect(95, 30, 70, 40, 3, (currentWB == lblWBKelvin && currentTint == lblTint ? TFT_DARKGREEN : TFT_DARKGREY));
  if(currentWB == lblWBKelvin && currentTint == lblTint)
    window.pushImage(115, 35, 30, 30, WBIncandescentBG);
  else
    window.pushImage(115, 35, 30, 30, WBIncandescent);

  // Fluorescent, 4000K
  lblWBKelvin = 4000;
  lblTint = 15;
  window.fillSmoothRoundRect(170, 30, 70, 40, 3, (currentWB == lblWBKelvin && currentTint == lblTint ? TFT_DARKGREEN : TFT_DARKGREY));
  if(currentWB == lblWBKelvin && currentTint == lblTint)
    window.pushImage(190, 35, 30, 30, WBFlourescentBG);
  else
    window.pushImage(190, 35, 30, 30, WBFlourescent);

  // Mixed Light, 4500K
  lblWBKelvin = 4500;
  lblTint = 15;
  window.fillSmoothRoundRect(245, 30, 70, 40, 3, (currentWB == lblWBKelvin && currentTint == lblTint ? TFT_DARKGREEN : TFT_DARKGREY));
  if(currentWB == lblWBKelvin && currentTint == lblTint)
    window.pushImage(265, 35, 30, 30, WBMixedLightBG);
  else
    window.pushImage(265, 35, 30, 30, WBMixedLight);

  // Cloud, 6500K
  lblWBKelvin = 6500;
  lblTint = 10;
  window.fillSmoothRoundRect(20, 75, 70, 40, 3, (currentWB == lblWBKelvin && currentTint == lblTint ? TFT_DARKGREEN : TFT_DARKGREY));
  if(currentWB == lblWBKelvin && currentTint == lblTint)
    window.pushImage(40, 80, 30, 30, WBCloudBG);
  else
    window.pushImage(40, 80, 30, 30, WBCloud);

  // Current White Balance Kelvin
  window.fillSmoothRoundRect(160, 75, 90, 40, 3, TFT_DARKGREEN);
  window.drawCentreString(String(currentWB), 205, 80);
  window.drawCentreString("KELVIN", 205, 103, &Lato_Regular5pt7b);

  if(editWB)
  {
    // WB Adjust Left <
    window.fillSmoothRoundRect(95, 75, 60, 40, 3, TFT_DARKGREY);
    window.drawCentreString("<", 125, 87);

    // WB Adjust Right >
    window.fillSmoothRoundRect(255, 75, 60, 40, 3, TFT_DARKGREY);
    window.drawCentreString(">", 284, 87);
  }

  // Current Tint
  window.fillSmoothRoundRect(160, 120, 90, 40, 3, TFT_DARKGREEN);
  window.drawCentreString(String(currentTint), 205, 132);

  if(!editWB)
  {
    // Tint Adjust Left <
    window.fillSmoothRoundRect(95, 120, 60, 40, 3, TFT_DARKGREY);
    window.drawCentreString("<", 125, 132);

    // Tint Adjust Right >
    window.fillSmoothRoundRect(255, 120, 60, 40, 3, TFT_DARKGREY);
    window.drawCentreString(">", 284, 132);
  }
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
  /*
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
  */

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh)
  // if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen Codec 4K/6K Refreshed.");

  window.fillScreen(TFT_BLACK);

  Screen_Common_Connected(); // Common elements

  // We need to have the Codec information to show the screen
  if(!camera->hasCodec())
  {
    window.setTextColor(TFT_WHITE);
    window.drawString("NO CODEC INFO.", 30, 9);

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
  /*
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
      // else if(currentRes == "HD" && tapped_x >= 20 && tapped_y >= 120 && tapped_x <= 310 && tapped_y <= 160)
      // {
        // HD - Scaled from Full sensor, 2.6K, or Windowed, can't distinguish
        // width = currentRecordingFormat.width; height = currentRecordingFormat.height;
        // window = true;
      // }

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
  */

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh)
  // if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen Resolution Pocket 4K Refreshed.");

  window.fillScreen(TFT_BLACK);

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
  /*
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
  */

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh)
  // if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen Resolution Pocket 6K Refreshed.");

  window.fillScreen(TFT_BLACK);

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
  /*
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
  */

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh)
  // if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen Media Pocket 4K/6K Refreshed.");

  window.fillScreen(TFT_BLACK);

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
  /*
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
  */

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh)
  // if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh && !tappedAction)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();

  DEBUG_DEBUG("Screen Lens Refreshed.");

  window.fillScreen(TFT_BLACK);

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
}

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

// Check if a string is an integer (whole number)
bool isInteger(const std::string& str) {
  if (str.empty()) {
    return false;
  }

  for (char c : str) {
    if (!std::isdigit(c)) {
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

  Serial.println(String("TouchDesigner Command Received: ") + commandPart.c_str() + ":" + valuePart.c_str());

  if(commandPart == "RECORD" && haveCamera)
  {
    auto camera = BMDControlSystem::getInstance()->getCamera();

    if(camera->hasTransportMode())
    {
      auto transportInfo = camera->getTransportMode();
      valuePart = capitaliseString(valuePart);

      if(valuePart == "START" && !camera->isRecording)
      {
        transportInfo.mode = CCUPacketTypes::MediaTransportMode::Record;
        PacketWriter::writeTransportInfo(transportInfo, &cameraConnection);
        Serial.println("Start Recording sent to camera");
      }
      else if(valuePart == "STOP" && camera->isRecording)
      {
        transportInfo.mode = CCUPacketTypes::MediaTransportMode::Preview;
        PacketWriter::writeTransportInfo(transportInfo, &cameraConnection);
        Serial.println("Stop Recording sent to camera");
      }
    }
  }
  else if(commandPart == "ISO" && haveCamera)
  {
    if(isInteger(valuePart)) // Ensure we have a whole number
    {
      int value = std::stoi(valuePart);

      // Accepting values between 
      if(value >= 100 and value <= 25600)
      {
        PacketWriter::writeISO(value, &cameraConnection); // ISO command
        Serial.println("ISO sent to camera");
      }
    }
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
}

void setup() {

  M5.begin();
  M5.Power.begin();

  Serial.begin(115200);

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

  M5.Display.setSwapBytes(true);

  // Standand Font
  M5.Display.setTextSize(1);
  M5.Display.setFont(&Lato_Regular11pt7b);

  // Splash screen
  M5.Display.pushImage(0, 0, 320, 170, MPCSplash_M5Stack_CoreS3);

  // From this point forward we'll use "window" instead of M5.Display (window is just a reference to M5.Display)

  // Prepare for Bluetooth connections and start scanning for cameras
  cameraConnection.initialise(&M5.Display, IWIDTH, IHEIGHT); // Screen Pass Key entry
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
    window.fillScreen(BLACK);

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
      RunTouchDesignerCommand(commandPart, valuePart);
    }

    // Reset ready for the next command
    tdCommandComplete = false;
  }

  /*
  if (Serial.available()) {
      int ch = Serial.read();

      switch(ch)
      {
        case 65:
          TouchDesignerPress = 'A';
          break;
        case 66:
          TouchDesignerPress = 'B';
          break;
        case 67:
          TouchDesignerPress = 'C';
          break;
        case 77: // M
        if(BMDControlSystem::getInstance()->hasCamera())
            PacketWriter::writeISO(400, &cameraConnection);
          break;
        case 78: // N
        if(BMDControlSystem::getInstance()->hasCamera())
            PacketWriter::writeISO(800, &cameraConnection);
          break;
        case 79: // O
        if(BMDControlSystem::getInstance()->hasCamera())
            PacketWriter::writeISO(1600, &cameraConnection);
          break;
        case 80: // P
        if(BMDControlSystem::getInstance()->hasCamera())
            PacketWriter::writeISO(3200, &cameraConnection);
          break;
      }
  }
  */

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