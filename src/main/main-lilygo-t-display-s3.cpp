// Magic Pocket Control, by Mark Sze
// For viewing and changing basic settings on Blackmagic Design Pocket Cinema Cameras and URSA 4.6K G2 and 12K.

#define USING_TFT_ESPI 1  // Using the TFT_eSPI graphics library <-- must include this in every main file, 0 = not using, 1 = using

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

#include "Arduino_DebugUtils.h" // Debugging to Serial - https://github.com/arduino-libraries/Arduino_DebugUtils

#include <TFT_eSPI.h> // Master copy here: https://github.com/Bodmer/TFT_eSPI

// https://github.com/fbiego/CST816S
// Touch with Gesture for the touch controller on the T-Display-S3
// Also integrated code from here to read touch points properly from the entire screen: // https://github.com/fbiego/CST816S/issues/1
#include "ESP32\CST816S\CST816S.h"
#include "ESP32\PinConfig.h"

// Main BMD Libraries
#include "Camera\ConstantsTypes.h"
#include "Camera\PacketWriter.h"
#include "CCU\CCUUtility.h"
#include "CCU\CCUPacketTypes.h"
#include "CCU\CCUValidationFunctions.h"
#include "Camera\BMDCameraConnection.h"
#include "Camera\BMDCamera.h"
#include "BMDControlSystem.h"

// Images
#include "Images\MPCSplash.h"
#include "Images\ImageBluetooth.h"
#include "Images\ImagePocket4k.h"
// PNG png;

// Include the watchdog library so we can hold stop it timing out while pass key entry.
#include "esp_task_wdt.h"

// https://github.com/fbiego/CST816S
// Touch with Gesture for the touch controller on the T-Display-S3
CST816S touch(PIN_IIC_SDA, PIN_IIC_SCL, PIN_TOUCH_RES, PIN_TOUCH_INT);

BMDCameraConnection cameraConnection;
std::shared_ptr<BMDControlSystem> BMDControlSystem::instance = nullptr; // Required for Singleton pattern and the constructor for BMDControlSystem
BMDCameraConnection* BMDCameraConnection::instancePtr = &cameraConnection; // Required for the scan function to run non-blocking and call the object back with the result

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
TFT_eSprite window = TFT_eSprite(&tft);
TFT_eSprite spriteMPCSplash = TFT_eSprite(&tft);
TFT_eSprite spriteBluetooth = TFT_eSprite(&tft);
TFT_eSprite spritePocket4k = TFT_eSprite(&tft);
TFT_eSprite spritePassKey = TFT_eSprite(&tft);

#define IWIDTH 320
#define IHEIGHT 170

int connectedScreenIndex = 0; // The index of the screen we're on:
// -2 is Pass Key
// -1 is No Connection
// 0 is Dashboard
// 1 is Recording
// 2 is ISO
// 3 is Shutter
// 4 is WB / Tint
// 5 is Codec
// 6 is Framerate
// 7 is Resolution
// 8 is Media
// 9 is Lens

int tapped_x = -1;
int tapped_y = -1;

// Display elements on the screen common to all pages
void Screen_Common(int sideBarColour)
{
    // Sidebar colour
    window.fillRect(0, 0, 13, 170, sideBarColour);
    window.fillRect(13, 0, 2, 170, TFT_DARKGREY);

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

  connectedScreenIndex = -1;

  // window.fillSprite(TFT_BLACK);
  spriteMPCSplash.pushToSprite(&window, 0, 0);

  // Status
  // window.fillRect(13, 0, 2, 170, TFT_DARKGREY);

  // window.fillSmoothCircle(IWIDTH - 25, 25, 18, TFT_RED, TFT_TRANSPARENT);

  DEBUG_VERBOSE("Connection Value: %i", cameraConnection.status);

  // Black background for text and Bluetooth Logo
  window.fillRect(0, 3, IWIDTH, 51, TFT_BLACK);

  // Bluetooth Image
  spriteBluetooth.pushToSprite(&window, 26, 6);
  
  window.setTextSize(2);
  window.textbgcolor = TFT_BLACK;
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
      window.drawSmoothRoundRect(25 + (count * 125) + (count * 10), 60, 5, 2, 125, 100, TFT_GREEN, TFT_DARKGREY);
      
    spritePocket4k.pushToSprite(&window, 33 + (count * 125) + (count * 10), 69);
    window.setTextSize(1);
    window.textbgcolor = TFT_DARKGREY;
    window.drawString(cameraConnection.cameraAddresses[count].toString().c_str(), 33 + (count * 125) + (count * 10), 144);
  }

  window.pushSprite(0, 0);
  
  if(connectToCameraIndex != -1)
  {
    DEBUG_DEBUG("Camera status [0]: %i", cameraConnection.status);
    cameraConnection.connect(cameraConnection.cameraAddresses[connectToCameraIndex]);
    DEBUG_DEBUG("Camera status [2]: %i", cameraConnection.status);
    connectToCameraIndex = -1;
  }
}

void Screen_PassKey()
{
  connectedScreenIndex = -2;

  // Don't do anything here, it will be handled by the security handler
  // Wait for the screen security handler to do its work to get the pass key.
  vTaskDelay(1);
}

// Default screen for connected state
void Screen_Dashboard()
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = 0;

  auto camera = BMDControlSystem::getInstance()->getCamera();
  int xshift = 0;

  window.fillSprite(TFT_BLACK);

  Screen_Common(TFT_GREEN); // Common elements

  // ISO
  if(camera->hasSensorGainISOValue())
  {
    window.fillSmoothRoundRect(20, 5, 75, 65, 3, TFT_DARKGREY, TFT_TRANSPARENT);
    window.setTextSize(2);
    window.textcolor = TFT_WHITE;
    window.textbgcolor = TFT_DARKGREY;

    window.drawCentreString(String(camera->getSensorGainISOValue()), 58, 28, tft.textfont);
    
    window.setTextSize(1);
    window.drawCentreString("ISO", 58, 59, tft.textfont);
  }

  // Shutter
  xshift = 80;
  if(camera->hasShutterAngle())
  {
    window.fillSmoothRoundRect(20 + xshift, 5, 75, 65, 3, TFT_DARKGREY, TFT_TRANSPARENT);
    window.setTextSize(2);
    window.textcolor = TFT_WHITE;
    window.textbgcolor = TFT_DARKGREY;

    
    window.drawCentreString(String(camera->getShutterAngle() / 100), 58 + xshift, 28, tft.textfont);
    
    window.setTextSize(1);
    window.drawCentreString("SHUTTER", 58 + xshift, 59, tft.textfont);
  }

  // WhiteBalance and Tint
  xshift += 80;
  if(camera->hasWhiteBalance() || camera->hasTint())
  {
    window.fillSmoothRoundRect(20 + xshift, 5, 135, 65, 3, TFT_DARKGREY, TFT_TRANSPARENT);
    window.setTextSize(2);
    window.textcolor = TFT_WHITE;
    window.textbgcolor = TFT_DARKGREY;

    if(camera->hasWhiteBalance())
      window.drawCentreString(String(camera->getWhiteBalance()), 58 + xshift, 28, tft.textfont);
    
    window.setTextSize(1);
    window.drawCentreString("WB", 58 + xshift, 59, tft.textfont);

    xshift += 66;

    window.setTextSize(2);
    if(camera->hasTint())
      window.drawCentreString(String(camera->getTint()), 58 + xshift, 28, tft.textfont);
    
    window.setTextSize(1);
    window.drawCentreString("TINT", 58 + xshift, 59, tft.textfont);
  }

  // Codec
  if(camera->hasCodec())
  {
    window.fillSmoothRoundRect(20, 75, 155, 40, 3, TFT_DARKGREY, TFT_TRANSPARENT);

    window.setTextSize(2);
    window.drawCentreString(camera->getCodec().to_string().c_str(), 97, 87, tft.textfont);
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
          case CCUPacketTypes::ActiveStorageMedium::SSD:
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
      window.fillSmoothRoundRect(20, 120, 100, 40, 3, TFT_DARKGREY, TFT_TRANSPARENT);

      window.setTextSize(2);
      window.drawCentreString(slotString.c_str(), 70, 133, tft.textfont);
    }
  }

  // Recording Format - Frame Rate and Resolution
  if(camera->hasRecordingFormat())
  {
    // Frame Rate
    window.fillSmoothRoundRect(180, 75, 135, 40, 3, TFT_DARKGREY, TFT_TRANSPARENT);
    window.setTextSize(2);

    window.drawCentreString(camera->getRecordingFormat().frameRate_string().c_str(), 237, 87, tft.textfont);

    window.setTextSize(1);
    window.drawCentreString("fps", 285, 97, tft.textfont);

    // Resolution
    window.fillSmoothRoundRect(125, 120, 190, 40, 3, TFT_DARKGREY, TFT_TRANSPARENT);

    std::string resolution = camera->getRecordingFormat().frameDimensionsShort_string();
    window.setTextSize(2);
    window.drawCentreString(resolution.c_str(), 220, 133, tft.textfont);
  }

  window.pushSprite(0, 0);
}

void Screen_Recording()
{
  if(!BMDControlSystem::getInstance()->hasCamera())
    return;

  connectedScreenIndex = 1;

  auto camera = BMDControlSystem::getInstance()->getCamera();

  // If we have a tap, we should determine if it is on anything
  if(tapped_x != -1)
  {
    // Serial.print("Screen_Recording tapped at: ");
    // Serial.print(tapped_x);
    // Serial.print(",");
    // Serial.println(tapped_y);

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

      PacketWriter::writeTransportPacket(transportInfo, &cameraConnection);
    }
  }

  window.fillSprite(TFT_BLACK);

  Screen_Common(TFT_GREEN); // Common elements

  // Record button
  if(camera->isRecording) window.fillSmoothCircle(257, 63, 58, Constants::DARK_RED, TFT_RED); // Recording solid
  window.drawSmoothCircle(257, 63, 57, (camera->isRecording ? TFT_RED : TFT_LIGHTGREY), (camera->isRecording ? TFT_RED : TFT_BLACK)); // Outer
  window.fillSmoothCircle(257, 63, 38, TFT_RED, (camera->isRecording ? TFT_RED : TFT_BLACK)); // Inner

  // Timecode
  window.setTextSize(2);
  window.textcolor = (camera->isRecording ? TFT_RED : TFT_WHITE);
  window.textbgcolor = TFT_BLACK;
  window.drawString(camera->getTimecodeString().c_str(), 30, 57);

  // Remaining time
  if(camera->getMediaSlots().size() != 0)
  {
    window.textcolor = TFT_LIGHTGREY;
    window.drawString((camera->getActiveMediaSlot().GetMediumString() + " " + camera->getActiveMediaSlot().remainingRecordTimeString).c_str(), 30, 100);

    window.setTextSize(1);
    window.drawString("REMAINING TIME", 30, 120);
  }

  window.pushSprite(0, 0);
}

void setup() {

  // Power and Backlight settings for T-Display-S3
	pinMode(15, OUTPUT); // PIN_POWER_ON 15
	pinMode(38, OUTPUT); // PIN_LCD_BL 38
	digitalWrite(15, HIGH);
	digitalWrite(38, HIGH);

  Serial.begin(115200);

  // Allow a timeout of 20 seconds for time for the pass key entry.
  esp_task_wdt_init(20, true);

  // SET DEBUG LEVEL
  Debug.setDebugLevel(DBG_VERBOSE);
  Debug.timestampOn();

  // Initialise the screen
  tft.init();

  // Landscape mode
  tft.setRotation(3);
  // tft.fillScreen(TFT_BLACK);

  window.createSprite(IWIDTH, IHEIGHT);
  // window.drawString("Blackmagic Camera Control", 20, 20);

  spriteMPCSplash.createSprite(IWIDTH, IHEIGHT);
  spriteMPCSplash.setSwapBytes(true);
  spriteMPCSplash.pushImage(0, 0, IWIDTH, IHEIGHT, MPCSplash);

  spriteMPCSplash.pushToSprite(&window, 0, 0);
  window.pushSprite(0, 0);

  spriteBluetooth.createSprite(30, 46);
  spriteBluetooth.setSwapBytes(true);
  spriteBluetooth.pushImage(0, 0, 30, 46, Wikipedia_Bluetooth_30x46);

  spritePocket4k.createSprite(110, 61);
  spritePocket4k.setSwapBytes(true);
  spritePocket4k.pushImage(0, 0, 110, 61, blackmagic_pocket_4k_110x61);

  spritePassKey.createSprite(IWIDTH, IHEIGHT);

  // Prepare for Bluetooth connections and start scanning for cameras
  cameraConnection.initialise(&window, &spritePassKey, &touch, IWIDTH, IHEIGHT); // Screen Pass Key entry

  // Start capturing touchscreen touches
  touch.begin();
}

int memoryLoopCounter;

void loop() {

  static unsigned long lastConnectedTime = 0;
  const unsigned long reconnectInterval = 5000;  // 5 seconds (milliseconds)

  unsigned long currentTime = millis();

  if (cameraConnection.status == BMDCameraConnection::ConnectionStatus::Disconnected && currentTime - lastConnectedTime >= reconnectInterval) {
    DEBUG_VERBOSE("Disconnected for too long, trying to reconnect");

    // Set the status to Scanning and then show the NoConnection screen to render the Scanning page before starting the scan (which blocks so it can't render the Scanning page before it finishes)
    cameraConnection.status = BMDCameraConnection::ConnectionStatus::Scanning;
    Screen_NoConnection();

    cameraConnection.scan();

    if(connectedScreenIndex != -1) // Move to the No Connection screen if we're not on it
      Screen_NoConnection();
  }
  else if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::Connected)
  {
    if(connectedScreenIndex >= 0)
    {
      switch(connectedScreenIndex)
      {
        case 0:
          Screen_Dashboard();
          break;
        case 1:
          Screen_Recording();
          break;
      }

    }
    else
      Screen_Dashboard(); // Was on disconnected screen, now we're connected go to the dashboard

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

  // Reset tapped point
  tapped_x = -1;
  tapped_y = -1;


  // Is there a touch event available?
  if (touch.available()) {
   
    // We only consider events when the finger has lifted up (rather than pressed down or held)
    if(touch.data.eventID == CST816S::TOUCHEVENT::UP)
    {
      int oriented_x = IWIDTH - touch.data.y;
      int oriented_y = touch.data.x;

      // Display touch point
      // tft.fillSmoothCircle(IWIDTH - touch.data.y, touch.data.x, 10, TFT_GREEN, TFT_TRANSPARENT);

      switch(touch.data.gestureID)
      {
        case CST816S::GESTURE::SWIPE_DOWN:
          // Swiping Right (sideways)
          switch(connectedScreenIndex)
          {
            case 0:
              Screen_Recording();
              break;
          }
          break;
        case CST816S::GESTURE::SWIPE_UP:
          // Swiping Left  (sideways)
          switch(connectedScreenIndex)
          {
            case 1:
              Screen_Dashboard();
              break;
          }
          break;
        case CST816S::GESTURE::SWIPE_LEFT:
        case CST816S::GESTURE::SWIPE_RIGHT:
          break;
        case CST816S::GESTURE::NONE:
          DEBUG_VERBOSE("Tap");

          // Save the tapped position for the screens to pick up
          tapped_x = oriented_x;
          tapped_y = oriented_y;
          break;
      }
    }
  }

  delay(25);

  // Keep track of the memory use to check that there aren't memory leaks (or significant memory leaks)
  if(Debug.getDebugLevel() >= DBG_VERBOSE && memoryLoopCounter++ % 400 == 0)
  {
    DEBUG_VERBOSE("Heap Size Free: %d of %d", ESP.getFreeHeap(), ESP.getHeapSize());
  }
}