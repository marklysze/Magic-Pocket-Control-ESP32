// Magic Pocket Control, by Mark Sze
// For viewing and changing basic settings on Blackmagic Design Pocket Cinema Cameras and URSA 4.6K G2 and 12K.

#define USING_TFT_ESPI 0  // Using the TFT_eSPI graphics library <-- must include this in every main file, 0 = not using, 1 = using
#define USING_M5GFX = 0   // Using the M5GFX graphics library <-- must include this in every main file, 0 = not using, 1 = using

// Note that the M5Stack libraries have their own modified TFT_eSPI libraries

#include <M5StickCPlus.h>

// M5Stack include a "min" macro in their In_eSPI.h file, we don't want to use that, so undefine it otherwise it won't compile
#undef min

// Main BMD Libraries
#include "Camera/PacketWriter.h"
#include "Camera/BMDCameraConnection.h"

// Bluetooth for bonding support
#include <BLEUtils.h>
#include <BLEServer.h>

// Images
#include "Images/MPCSplash-M5StickC-Plus.h"
#include "Images/ImageBluetooth.h"



// Core variables and control system
BMDCameraConnection cameraConnection;
std::shared_ptr<BMDControlSystem> BMDControlSystem::instance = nullptr; // Required for Singleton pattern and the constructor for BMDControlSystem
BMDCameraConnection* BMDCameraConnection::instancePtr = &cameraConnection; // Required for the scan function to run non-blocking and call the object back with the result

// TFT_eSPI Window using the M5.Lcd object, which derives from TFT_eSPI
TFT_eSprite window = TFT_eSprite(&M5.Lcd);

// Images
TFT_eSprite spriteMPCSplash = TFT_eSprite(&M5.Lcd);


#define IWIDTH 240
#define IHEIGHT 135

enum class Screens : byte
{
  // PassKey = 9,
  NoConnection = 10,
  Dashboard = 100,
  Recording = 101,
};

Screens connectedScreenIndex = Screens::NoConnection; // The index of the screen we're on:
// 9 is Pass Key
// 10 is No Connection
// 100 is Dashboard
// 101 is Recording

// Keep track of the last camera modified time that we refreshed a screen so we don't keep refreshing a screen when the camera object remains unchanged.
static unsigned long lastRefreshedScreen = 0;

// Display elements on the screen common to all pages
void Screen_Common(int sideBarColour)
{
    // Sidebar colour
    window.fillRect(0, 0, 13, IHEIGHT, sideBarColour);
    window.fillRect(13, 0, 2, IHEIGHT, TFT_DARKGREY);

  if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::Connected && BMDControlSystem::getInstance()->hasCamera())
  {
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

  window.pushImage(0, 0, IWIDTH, IHEIGHT, MPCSplash_M5StickC_Plus);

  // Black background for text and Bluetooth Logo
  window.fillRect(0, 3, IWIDTH, 51, TFT_BLACK);

  // Bluetooth Image
  window.pushImage(20, 6, 30, 46, Wikipedia_Bluetooth_30x46);
  
  window.setTextSize(2);
  window.textbgcolor = TFT_BLACK;
  window.textcolor = TFT_WHITE;
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
        window.drawString("Connecting...", 70, 20);
        connectToCameraIndex = 0;
      }
      else
        window.drawString("Found cameras", 70, 20);
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
      DEBUG_DEBUG("Need Pass Key");
      Screen_Common(TFT_PURPLE); // Common elements
      break;
    case BMDCameraConnection::FailedPassKey:
      Screen_Common(TFT_ORANGE); // Common elements
      window.drawString("Wrong Key", 70, 20);
      break;
    case BMDCameraConnection::Disconnected:
      Screen_Common(TFT_RED); // Common elements
      window.drawString("Disconnected", 70, 20);
      break;
    case BMDCameraConnection::IncompatibleProtocol:
      // Note: This needs to be worked on as there's no incompatible protocol connections yet.
      Screen_Common(TFT_RED); // Common elements
      window.drawString("Incompatible Protocol", 70, 20);
      break;
    default:
      break;
  }


  // Although we could support more than one camera, we'll just connect to the first.
  if(connectToCameraIndex != -1)
  {
    // Check if we have a Bluetooth bonding to the camera already
    if(BMDCameraConnection::isCameraBonded(cameraConnection.cameraAddresses[connectToCameraIndex]))
    {
      // We have previously bonded so chances are we won't see the pass key entry
      window.pushSprite(0, 0);
    }
    else
    {
      // Expect the pass key entry, so display a message
        window.fillRoundRect(30, 65, 200, 60, 3, TFT_YELLOW);
        window.textbgcolor = TFT_YELLOW;
        window.setTextColor(TFT_BLACK);
        window.drawCentreString("PASS KEY", 130, 75, M5.Lcd.textfont);
        window.drawCentreString("SERIAL TERMINAL", 130, 100, M5.Lcd.textfont);
    }

    window.pushSprite(0, 0);

    cameraConnection.connect(cameraConnection.cameraAddresses[connectToCameraIndex]);
    connectToCameraIndex = -1;
  }
  else
    window.pushSprite(0, 0);
}

// Default screen for connected state
void Screen_Dashboard(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Dashboard;

  auto camera = BMDControlSystem::getInstance()->getCamera();
  int xshift = 0;

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();
  
  DEBUG_DEBUG("Screen Dashboard Refreshed.");

  window.fillSprite(TFT_BLACK);

  Screen_Common(TFT_GREEN); // Common elements

  // ISO
  if(camera->hasSensorGainISOValue())
  {
    window.fillRoundRect(20, 5, 70, 45, 3, TFT_DARKGREY);
    window.setTextSize(2);
    window.textcolor = TFT_WHITE;
    window.textbgcolor = TFT_DARKGREY;

    window.drawCentreString(String(camera->getSensorGainISOValue()), 56, 15, M5.Lcd.textfont);

    window.setTextSize(1);
    window.drawCentreString("ISO", 56, 37, M5.Lcd.textfont);
  }

  // Shutter
  xshift = 75;
  if(camera->hasShutterAngle())
  {
    window.fillRoundRect(20 + xshift, 5, 70, 45, 3, TFT_DARKGREY);
    window.setTextSize(2);
    window.textcolor = TFT_WHITE;
    window.textbgcolor = TFT_DARKGREY;

    if(camera->shutterValueIsAngle)
    {
      // Shutter Angle
      int currentShutterAngle = camera->getShutterAngle();
      float ShutterAngleFloat = currentShutterAngle / 100.0;

      window.drawCentreString(String(ShutterAngleFloat, (currentShutterAngle % 100 == 0 ? 0 : 1)), 56 + xshift, 15, M5.Lcd.textfont);
    }
    else
    {
      // Shutter Speed
      int currentShutterSpeed = camera->getShutterSpeed();

      window.drawCentreString("1/" + String(currentShutterSpeed), 56 + xshift, 15, M5.Lcd.textfont);
    }

    window.setTextSize(1);
    window.drawCentreString(camera->shutterValueIsAngle ? "DEGREES" : "SPEED", 56 + xshift, 37, M5.Lcd.textfont); //  "SHUTTER"
  }

  // WhiteBalance and Tint
  xshift += 75;
  if(camera->hasWhiteBalance() || camera->hasTint())
  {
    window.fillRoundRect(20 + xshift, 5, 70, 65, 3, TFT_DARKGREY);
    window.setTextSize(2);
    window.textcolor = TFT_WHITE;
    window.textbgcolor = TFT_DARKGREY;

    if(camera->hasWhiteBalance())
      window.drawCentreString(String(camera->getWhiteBalance()), 58 + xshift, 15, M5.Lcd.textfont);

    window.setTextSize(1);
    window.drawCentreString("WB/TINT", 58 + xshift, 57, M5.Lcd.textfont);

    window.setTextSize(2);
    if(camera->hasTint())
      window.drawCentreString(String(camera->getTint()), 58 + xshift, 35, M5.Lcd.textfont);
  }

  // Codec
  if(camera->hasCodec())
  {
    window.fillRoundRect(20, 55, 145, 30, 3, TFT_DARKGREY);

    window.setTextSize(2);
    window.drawCentreString(camera->getCodec().to_string().c_str(), 93, 62, M5.Lcd.textfont);
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
      window.fillRoundRect(20, 90, 70, 42, 3, TFT_DARKGREY);

      window.setTextSize(2);
      window.drawCentreString(slotString.c_str(), 55, 105, M5.Lcd.textfont);

      // Show recording error
      if(camera->hasRecordError())
        window.drawRoundRect(20, 90, 70, 42, 3, TFT_RED);
    }
    else
    {
      // Show no Media
      window.fillRoundRect(20, 90, 70, 42, 3, TFT_DARKGREY);
      window.setTextSize(1);
      window.drawCentreString("NO MEDIA", 55, 105, M5.Lcd.textfont);
    }
  }

  // Recording Format - Frame Rate and Resolution
  if(camera->hasRecordingFormat())
  {
    // Frame Rate
    window.fillRoundRect(95, 90, 70, 42, 3, TFT_DARKGREY);
    window.setTextSize(2);

    window.drawCentreString(camera->getRecordingFormat().frameRate_string().c_str(), 130, 98, M5.Lcd.textfont);
    window.setTextSize(1);
    window.drawCentreString("fps", 130, 120, M5.Lcd.textfont);

    // Resolution
    window.fillRoundRect(170, 75, 70, 57, 3, TFT_DARKGREY);

    std::string resolution = camera->getRecordingFormat().frameDimensionsShort_string();
    window.setTextSize(2);

    // Split over two lines if necessary
    size_t spaceFound = resolution.find(' ');

    if(spaceFound != std::string::npos)
    {
        // Split the string into the first part, big text, and the second part, small text
        window.drawCentreString(resolution.substr(0, spaceFound).c_str(), 205, 85, M5.Lcd.textfont);
        window.drawCentreString(resolution.substr(spaceFound + 1).c_str(), 205, 110, M5.Lcd.textfont);
    }
    else    // No space
        window.drawCentreString(resolution.c_str(), 205, 95, M5.Lcd.textfont);
  }

  window.pushSprite(0, 0);
}

void Screen_Recording(bool forceRefresh = false)
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = Screens::Recording;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // If the screen hasn't changed, there were no touch events and we don't have to refresh, return.
  if(lastRefreshedScreen == camera->getLastModified() && !forceRefresh)
    return;
  else
    lastRefreshedScreen = camera->getLastModified();

  DEBUG_DEBUG("Screen Recording Refreshed.");

  window.fillSprite(TFT_BLACK);

  Screen_Common(TFT_GREEN); // Common elements

  // Record button
  if(camera->isRecording) window.fillCircle(242, 63, 58, Constants::DARK_RED); // Recording solid
  window.drawCircle(242, 63, 57, (camera->isRecording ? TFT_RED : TFT_LIGHTGREY)); // Outer
  window.fillCircle(242, 63, 38, TFT_RED); // Inner

  // Timecode
  window.setTextSize(2);
  window.textcolor = (camera->isRecording ? TFT_RED : TFT_WHITE);
  window.textbgcolor = TFT_BLACK;
  window.drawString(camera->getTimecodeString().c_str(), 30, 100);

  // Remaining time and any errors
  if(camera->getMediaSlots().size() != 0)
  {
    window.textcolor = TFT_LIGHTGREY;
    window.drawString(camera->getActiveMediaSlot().GetMediumString().c_str(), 30, 40);
    window.drawString(camera->getActiveMediaSlot().remainingRecordTimeString.c_str(), 30, 60);

    window.setTextSize(1);
    window.drawString("REMAINING TIME", 30, 80);

    // Show any media record errors
    if(camera->hasRecordError())
    {
      window.setTextSize(2);
      window.textcolor = TFT_RED;
      window.drawString("RECORD ERROR", 30, 18);
    }
  }

  window.pushSprite(0, 0);
}

void setup() {

    Serial.begin(115200);

    // SET DEBUG LEVEL
    Debug.setDebugLevel(DBG_VERBOSE);
    Debug.timestampOn();

    M5.begin();

    // Initialise the screen
    M5.Lcd.setRotation(3);

    // Main sprite for drawing to screen in one go
    window.createSprite(IWIDTH, IHEIGHT);

    // Load the splash screen image into the sprite
    window.pushImage(0, 0, IWIDTH, IHEIGHT, MPCSplash_M5StickC_Plus);

    // And draw it on screen!
    window.pushSprite(0, 0);

    // Prepare for Bluetooth connections and start scanning for cameras
    cameraConnection.initialise(); // For Serial-based pass key entry

}

void loop() {

  static unsigned long lastConnectedTime = 0;
  const unsigned long reconnectInterval = 5000;  // 5 seconds (milliseconds)

  unsigned long currentTime = millis();

  if (cameraConnection.status == BMDCameraConnection::ConnectionStatus::Disconnected && currentTime - lastConnectedTime >= reconnectInterval) {
    DEBUG_VERBOSE("Disconnected for too long, trying to reconnect");

    // For testing, this removes BLE bondings so the pass key needs to be entered. Remove comment to force pass key entry.
    // BMDCameraConnection::clearBondedDevices();

    // Set the status to Scanning and then show the NoConnection screen to render the Scanning page before starting the scan (which blocks so it can't render the Scanning page before it finishes)
    cameraConnection.status = BMDCameraConnection::ConnectionStatus::Scanning;
    Screen_NoConnection();

    cameraConnection.scan();

    if(connectedScreenIndex != Screens::NoConnection) // Move to the No Connection screen if we're not on it
      Screen_NoConnection();
  }
  else if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::Connected)
  {
    if(static_cast<byte>(connectedScreenIndex) >= 100)
    {
      auto camera = BMDControlSystem::getInstance()->getCamera();

      switch(connectedScreenIndex)
      {
        case Screens::Dashboard:
          Screen_Dashboard();
          break;
        case Screens::Recording:
          Screen_Recording();
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

  // Read the state of the buttons
  M5.update();

  if(M5.BtnA.wasReleased())
  {
    // Record button
    auto camera = BMDControlSystem::getInstance()->getCamera();
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

  }
  else if(M5.BtnB.wasReleased())
  {
    // Screen switcher
    switch(connectedScreenIndex)
    {
      case Screens::Dashboard:
        Screen_Recording(true);
        break;
      case Screens::Recording:
        Screen_Dashboard(true);
        break;
    }
  }

  delay(25);
}