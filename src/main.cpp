// Magic Pocket Control, by Mark Sze
// For viewing and changing basic settings on Blackmagic Design Pocket Cinema Cameras and URSA 4.6K G2 and 12K.

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

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
// #include "PNGdec.h"
#include "Images\ImageBluetooth.h"
#include "Images\ImagePocket4k.h"
// PNG png;

// Include the watchdog library so we can hold stop it timing out while pass key entry.
#include "esp_task_wdt.h"

// Remove later
// #include <random>
// #include <cstdint>


// https://github.com/fbiego/CST816S
// Touch with Gesture for the touch controller on the T-Display-S3
CST816S touch(PIN_IIC_SDA, PIN_IIC_SCL, PIN_TOUCH_RES, PIN_TOUCH_INT);

BMDCameraConnection cameraConnection;
std::shared_ptr<BMDControlSystem> BMDControlSystem::instance = nullptr; // Required for Singleton pattern and the constructor for BMDControlSystem
BMDCameraConnection* BMDCameraConnection::instancePtr = &cameraConnection; // Required for the scan function to run non-blocking and call the object back with the result

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
TFT_eSprite window = TFT_eSprite(&tft);
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

int tapped_x = -1;
int tapped_y = -1;


// Screen for when there's no connection, it's scanning, and it's trying to connect.
void Screen_NoConnection()
{
  // The camera to connect to.
  int connectToCameraIndex = -1;

  connectedScreenIndex = -1;

  window.fillSprite(TFT_BLACK);

  // Status
  window.fillRect(13, 0, 2, 170, TFT_DARKGREY);

  // window.fillSmoothCircle(IWIDTH - 25, 25, 18, TFT_RED, TFT_TRANSPARENT);

  // Bluetooth Image
  spriteBluetooth.pushToSprite(&window, 26, 6);
  
  window.setTextSize(2);
  window.textbgcolor = TFT_BLACK;
  switch(cameraConnection.status)
  {
    case BMDCameraConnection::Scanning:
      window.fillRect(0, 0, 13, 170, TFT_BLUE);
      window.drawString("Scanning...", 70, 20);
      break;
    case BMDCameraConnection::ScanningFound:
      window.fillRect(0, 0, 13, 170, TFT_BLUE);
      if(cameraConnection.cameraAddresses.size() == 1)
      {
        window.drawString("Found, connecting...", 70, 20);
        connectToCameraIndex = 0;
      }
      else
        window.drawString("Found cameras", 70, 20);
      break;
    case BMDCameraConnection::ScanningNoneFound:
      window.fillRect(0, 0, 13, 170, TFT_RED);
      window.drawString("No camera found", 70, 20);
      break;
    case BMDCameraConnection::Connecting:
      window.fillRect(0, 0, 13, 170, TFT_YELLOW);
      window.drawString("Connecting...", 70, 20);
      break;
    case BMDCameraConnection::NeedPassKey:
      window.fillRect(0, 0, 13, 170, TFT_PURPLE);
      window.drawString("Need Pass Key", 70, 20);
      break;
    case BMDCameraConnection::FailedPassKey:
      window.fillRect(0, 0, 13, 170, TFT_ORANGE);
      window.drawString("Wrong Pass Key", 70, 20);
      break;
    case BMDCameraConnection::Disconnected:
      window.fillRect(0, 0, 13, 170, TFT_RED);
      window.drawString("Disconnected (wait)", 70, 20);
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
    cameraConnection.connect(cameraConnection.cameraAddresses[connectToCameraIndex]);
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

  // Left side
  window.fillRect(0, 0, 13, 170, TFT_GREEN);
  window.fillRect(13, 0, 2, 170, TFT_DARKGREY);

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
        Serial.print("Record Stop");
        transportInfo.mode = CCUPacketTypes::MediaTransportMode::Preview;
      }
      else
      {
        Serial.print("Record Start");
        transportInfo.mode = CCUPacketTypes::MediaTransportMode::Record;
      }

      PacketWriter::writeTransportPacket(transportInfo, &cameraConnection);
      // PacketWriter::writeWhiteBalance(5600, -10, &cameraConnection);
      // PacketWriter::writeISO(3200, &cameraConnection);
    }
  }


  window.fillSprite(TFT_BLACK);

  // Left side
  window.fillRect(0, 0, 13, 170, TFT_GREEN);
  window.fillRect(13, 0, 2, 170, TFT_DARKGREY);

  // Record button
  if(camera->isRecording) window.fillSmoothCircle(257, 62, 62, Constants::LIGHT_RED, TFT_TRANSPARENT); // Recording solid
  window.drawSmoothCircle(257, 62, 61, (camera->isRecording ? TFT_RED : TFT_LIGHTGREY), TFT_TRANSPARENT); // Outer
  window.fillSmoothCircle(257, 62, 40, TFT_RED, TFT_TRANSPARENT); // Inner

  // Timecode
  window.setTextSize(2);
  window.textcolor = TFT_WHITE;
  window.textbgcolor = TFT_BLACK;
  window.drawCentreString("00:00:00:00", 167, 136, tft.textfont);


  window.pushSprite(0, 0);
}


/*
void Screen_SlideTest()
{
  connectedScreenIndex = 2;

  slideWindow.fillSprite(TFT_BLACK);

  for(int i = IWIDTH; i > (IWIDTH * -1); i--)
  {
    // Left Half One Screen
    slideWindow.drawRect(IWIDTH * -1, 0, IWIDTH, IHEIGHT, TFT_PURPLE);

    // Right Half One Screen
    slideWindow.drawRect(IWIDTH * 1, 0, IWIDTH, IHEIGHT, TFT_GREEN);

    slideWindow.pushToSprite(&window, i, 0);

    window.pushSprite(0, 0);

    delay(10);
  }
}
*/

void setup() {

  Serial.begin(115200);
  Serial.println("Booting...");

  // Allow a timeout of 20 seconds for time for the pass key entry.
  esp_task_wdt_init(20, true);

  // Initialise the screen
  tft.init();

  // Landscape mode
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  window.createSprite(IWIDTH, IHEIGHT);
  // window.drawString("Blackmagic Camera Control", 20, 20);

  spriteBluetooth.createSprite(30, 46);
  spriteBluetooth.setSwapBytes(true);
  spriteBluetooth.pushImage(0,0,30,46,Wikipedia_Bluetooth_30x46);

  spritePocket4k.createSprite(110, 61);
  spritePocket4k.setSwapBytes(true);
  spritePocket4k.pushImage(0,0,110,61,blackmagic_pocket_4k_110x61);

  spritePassKey.createSprite(IWIDTH, IHEIGHT);

  // Prepare for Bluetooth connections and start scanning for cameras
  // cameraConnection.initialise(); // Serial Pass Key entry
  cameraConnection.initialise(&window, &spritePassKey, &touch, IWIDTH, IHEIGHT); // Screen Pass Key entry

  // window.drawSmoothCircle(IWIDTH - 25, 25, 22, TFT_WHITE, TFT_TRANSPARENT);
  window.pushSprite(0, 0);

  // Start capturing touchscreen touches
  touch.begin();
}

void loop() {

  static unsigned long lastConnectedTime = 0;
  const unsigned long reconnectInterval = 5000;  // 5 seconds (milliseconds)

  unsigned long currentTime = millis();

  if (cameraConnection.status == BMDCameraConnection::ConnectionStatus::Disconnected && currentTime - lastConnectedTime >= reconnectInterval) {
    Serial.println("Disconnected for too long, trying to reconnect");
    cameraConnection.scan();

    if(connectedScreenIndex != -1) // Move to the No Connection screen if we're not on it
      Screen_NoConnection();
  }
  else if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::Connected)
  {
    // Serial.print(connectedScreenIndex);

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
  else if(cameraConnection.status == BMDCameraConnection::ScanningFound)
  {
    // if(connectedScreenIndex != -1) // Move to the No Connection screen if we're not on it
    Screen_NoConnection();
  }
  else if(cameraConnection.status == BMDCameraConnection::ScanningNoneFound)
  {
    Serial.println("Status Scanning NONE Found. Marking as Disconnected.");
    cameraConnection.status = BMDCameraConnection::Disconnected;
    lastConnectedTime = currentTime;

    // if(connectedScreenIndex != -1) // Move to the No Connection screen if we're not on it
    Screen_NoConnection();
  }

  /*
  if(connectedScreenIndex < 0 && cameraConnection.status == BMDCameraConnection::ConnectionStatus::Connected)
  {
    // When we've just connected, go to the dashboard
    Screen_Dashboard();

    lastConnectedTime = currentTime;
  }
  else if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::NeedPassKey)
  {
    // If we need the Pass Key, show the Pass Key screen
    Screen_PassKey();
  }
  else if(connectedScreenIndex >= 0 && cameraConnection.status == BMDCameraConnection::ConnectionStatus::Disconnected) // Disconnected
  {
    // Lost connection
    Screen_NoConnection();
  }
  */

  if(false && BMDControlSystem::getInstance()->hasCamera())
  {
    // std::shared_ptr<BMDCamera> camera = BMDControlSystem::getInstance()->getCamera();
    auto camera = BMDControlSystem::getInstance()->getCamera();

    tft.drawString("ISO: ", 20, 40);
    if(camera->hasSensorGainISOValue())
      tft.drawString(String(camera->getSensorGainISOValue()), 100, 40);

    tft.drawString("Codec: ", 20, 60);
    if(camera->hasCodec())
      tft.drawString(String(camera->getCodec().to_string().c_str()), 100, 60);

    tft.drawString("White Bal.: ", 20, 80);
    if(camera->hasWhiteBalance())
      tft.drawString(String(camera->getWhiteBalance()), 100, 80);

    tft.drawString("Tint: ", 20, 100);
    if(camera->hasTint())
      tft.drawString(String(camera->getTint()), 100, 100);

    std::vector<BMDCamera::MediaSlot> mediaSlots = camera->getMediaSlots();
    for(int i = 0; i < mediaSlots.size(); i++)
    {
      tft.drawString("Slot", 20, 120 + (i * 20));
      tft.drawString(String(i + 1), 50, 120 + (i * 20));

      if(mediaSlots[i].active)
        tft.drawString("*", 60, 120 + (i * 20));

      tft.drawString(camera->getSlotActiveStorageMediumString(i).c_str(), 75, 120 + (i * 20));

      tft.drawString(camera->getSlotMediumStatusString(i).c_str(), 120, 120 + (i * 20));

      tft.drawString(mediaSlots[i].remainingRecordTimeString.c_str(), 160, 120 + (i * 20));
    }

    // if(mediaSlots.size() == 0)
      // tft.drawString("No Media Slots", 20, 120);

    /*
    tft.drawString("ISO: ", 20, 40);
    if(camera->hasSensorGainISO())
      tft.drawString(String(camera->getSensorGainISO()), 100, 40);

    tft.drawString("ISO Value: ", 20, 60);
    if(camera->hasSensorGainISOValue())
      tft.drawString(String(camera->getSensorGainISOValue()).c_str(), 100, 60);

    tft.drawString("White Balance: ", 20, 80);
    if(camera->hasWhiteBalance())
      tft.drawString(String(camera->getWhiteBalance()), 100, 80);

    tft.drawString("Tint: ", 20, 100);
    if(camera->hasTint())
      tft.drawString(String(camera->getTint()), 100, 100);

    tft.drawString("Shutter Speed: ", 20, 120);
    if(camera->hasShutterSpeed())
      tft.drawString(String(camera->getShutterSpeed()), 100, 120);

    tft.drawString("LUT Enabled? ", 20, 140);
    if(camera->hasSelectedLUTEnabled() && camera->getSelectedLUTEnabled())
      tft.drawString("Yes", 100, 140);

    */

    /* Meta Attributes

    tft.drawString("Model Name: ", 20, 40);
    if(camera->hasModelName())
      tft.drawString(camera->getModelName().c_str(), 100, 40);

    tft.drawString("Lens Type: ", 20, 60);
    if(camera->hasLensType())
      tft.drawString(camera->getLensType().c_str(), 100, 60);

    tft.drawString("Lens Focal: ", 20, 80);
    if(camera->hasLensFocalLength())
      tft.drawString(camera->getLensFocalLength().c_str(), 100, 80);

    tft.drawString("Lens Aperture: ", 20, 100);
    if(camera->hasLensIris())
      tft.drawString(camera->getLensIris().c_str(), 100, 100);

    tft.drawString("Camera Id: ", 20, 120);
    if(camera->hasCameraId())
      tft.drawString(camera->getCameraId().c_str(), 100, 120);

    tft.drawString("Project Name: ", 20, 140);
    if(camera->hasProjectName())
      tft.drawString(camera->getProjectName().c_str(), 100, 140);
    */
  }

  // Reset tapped point
  tapped_x = -1;
  tapped_y = -1;


  if (touch.available()) {
    /*
    Serial.print(touch.gesture());
    Serial.print("\t");
    Serial.print(touch.event());
    Serial.print("\t");
    Serial.print(touch.data.x);
    Serial.print("\t");
    Serial.println(touch.data.y);
    */
   
    if(touch.data.eventID == CST816S::TOUCHEVENT::UP)
    {
      int oriented_x = IWIDTH - touch.data.y;
      int oriented_y = touch.data.x;

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
          // tft.fillSmoothCircle(IWIDTH - touch.data.y, touch.data.x, 10, generateRandomColor(), TFT_TRANSPARENT);
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
          Serial.println("Tap");
          // tft.fillSmoothCircle(IWIDTH - touch.data.y, touch.data.x, 10, TFT_GREEN, TFT_TRANSPARENT);

          // Save the tapped position for the screens to pick up
          tapped_x = oriented_x;
          tapped_y = oriented_y;
          break;
      }
    }
  }

  delay(25);
  // Serial.println("<X>");
}