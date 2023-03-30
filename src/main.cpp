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

#define IWIDTH 320
#define IHEIGHT 170

/*
uint32_t generateRandomColor() {
    // Generate random values for red, green, and blue
    uint8_t red = random(256);
    uint8_t green = random(256);
    uint8_t blue = random(256);

    // Combine the values into a 32-bit color value (RGB format)
    uint32_t color = ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;

    // Return the 32-bit color value
    return color;
}
*/

/*
// PNGdec globals Position variables must be global (PNGdec does not handle position coordinates)
#define MAX_IMAGE_WIDTH 320
int16_t _PNGdec_xpos = 0;
int16_t _PNGdec_ypos = 0;

//=========================================v==========================================
//  pngDraw: Callback function to draw pixels to the display
//====================================================================================
// This function will be called during decoding of the png file to render each image
// line to the TFT. PNGdec generates the image line and a 1bpp mask.
void pushMaskedImage(int32_t x, int32_t y, int32_t w, uint16_t *img, uint8_t *mask)
{
  uint8_t  *mptr = mask;
  uint8_t  *eptr = mask + ((w + 7) >> 3);
  uint16_t *iptr = img;

  uint32_t xp = 0;
  uint32_t setCount = 0;
  uint32_t clearCount = 0;
  uint8_t  mbyte = 0;
  uint8_t  bits  = 0;

  // Scan through each bit of the mask
  while (mptr < eptr) {
    if (bits == 0) {
        mbyte = *mptr++;
        bits  = 8;
    }
    // Likely image lines starts with transparent pixels
    while ((mbyte & 0x80) == 0) {
        // Deal with case where remaining bits in byte are clear
        if (mbyte == 0x00) {
        clearCount += bits;
        if (mptr >= eptr) break;
        mbyte = *mptr++;
        bits  = 8;
        continue;
        }
        mbyte += mbyte;
        clearCount ++;
        if (--bits) continue;;
        if (mptr >= eptr) break;
        mbyte = *mptr++;
        bits  = 8;
    }

    //Get run length for set bits
    while (mbyte & 0x80) {
        // Deal with case where all bits are set
        if (mbyte == 0xFF) {
        setCount += bits;
        if (mptr >= eptr)  break;
        mbyte = *mptr++;
        continue;
        }
        mbyte += mbyte;
        setCount ++;
        if (--bits) continue;
        if (mptr >= eptr) break;
        mbyte = *mptr++;
        bits  = 8;
    }

    // Dump the pixels
    if (setCount) {
        xp += clearCount;
        tft.pushImage(x + xp, y, setCount, 1, iptr + xp);
        xp += setCount;
        clearCount = 0;
        setCount = 0;
    }
  }
}

//=========================================v==========================================
//  pngDraw: Callback function to draw pixels to the display
//====================================================================================
// This function will be called during decoding of the png file to render each image
// line to the TFT. PNGdec generates the image line and a 1bpp mask.
void pngDraw(PNGDRAW *pDraw) {
  uint16_t lineBuffer[MAX_IMAGE_WIDTH];          // Line buffer for rendering
  uint8_t  maskBuffer[1 + MAX_IMAGE_WIDTH / 8];  // Mask buffer

  png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);

  if (png.getAlphaMask(pDraw, maskBuffer, 255)) {
      pushMaskedImage(_PNGdec_xpos, _PNGdec_ypos + pDraw->y, pDraw->iWidth, lineBuffer, maskBuffer);
  }
}
*/

// Screen for when there's no connection, it's scanning, and it's trying to connect.
void Screen_NoConnection()
{
  // The camera to connect to.
  int connectToCameraIndex = -1;

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

// Screen for when there's no connection
void Screen_Home()
{
  window.fillSprite(TFT_BLACK);

  // Left side
  window.fillRect(0, 0, 13, 170, TFT_GREEN);
  window.fillRect(13, 0, 2, 170, TFT_DARKGREY);

  /*

  // Bluetooth Image
  spriteBluetooth.pushToSprite(&window, 26, 6);
  
  window.setTextSize(2);
  window.textbgcolor = TFT_BLACK;
  window.drawString("No connection", 70, 20);

  // Cameras
  // window.drawSmoothRoundRect(25, 60, 5, 2, 125, 100, TFT_DARKGREY, TFT_TRANSPARENT);
  window.fillRoundRect(25, 60, 125, 100, 5, TFT_DARKGREY);
  spritePocket4k.pushToSprite(&window, 33, 69);
  window.setTextSize(1);
  window.textbgcolor = TFT_DARKGREY;
  window.drawString("34:85:18:72:1B:F1", 33, 144);
  
  */

  window.pushSprite(0, 0);
}

void setup() {

  Serial.begin(115200);
  Serial.println("Booting...");

  // Initialise the screen
  tft.init();

  // Landscape mode
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  window.createSprite(IWIDTH, IHEIGHT);
  window.drawString("Blackmagic Camera Control", 20, 20);

  spriteBluetooth.createSprite(30, 46);
  spriteBluetooth.setSwapBytes(true);
  spriteBluetooth.pushImage(0,0,30,46,Wikipedia_Bluetooth_30x46);

  spritePocket4k.createSprite(110, 61);
  spritePocket4k.setSwapBytes(true);
  spritePocket4k.pushImage(0,0,110,61,blackmagic_pocket_4k_110x61);

  // Prepare for Bluetooth connections and start scanning for cameras
  cameraConnection.initialise();

  window.drawSmoothCircle(IWIDTH - 25, 25, 22, TFT_WHITE, TFT_TRANSPARENT);
  window.pushSprite(0, 0);

  // Start capturing touchscreen touches
  touch.begin();
}

void loop() {

  static unsigned long lastConnectedTime = 0;
  const unsigned long reconnectInterval = 5000;  // 5 seconds (milliseconds)

  unsigned long currentTime = millis();

  if (cameraConnection.status == BMDCameraConnection::ConnectionStatus::Disconnected && currentTime - lastConnectedTime >= reconnectInterval) {

    // disconnectedIterations = 0;
    Serial.println("Disconnected for too long, trying to reconnect");
    cameraConnection.scan();
  }
  else if(cameraConnection.status == BMDCameraConnection::ScanningFound)
  {
    // Serial.println("Status Scanning Found. Connecting.");
    // cameraConnection.status = BMDCameraConnection::Connecting;
    // cameraConnection.connect();
    // Serial.println("Connecting Finished.");
  }
  else if(cameraConnection.status == BMDCameraConnection::ScanningNoneFound)
  {
    Serial.println("Status Scanning NONE Found. Marking as Disconnected.");
    cameraConnection.status = BMDCameraConnection::Disconnected;
    lastConnectedTime = currentTime;
  }

  if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::Connected)
  {
    // tft.fillScreen(TFT_GREEN);

    // window.drawString("Connected", 20, 30);
    // window.pushSprite(0, 0);

    // tft.drawSmoothCircle(50, 50, 50, TFT_GREEN, TFT_TRANSPARENT);
    // tft.fillSmoothCircle(IWIDTH - 25, 25, 18, TFT_GREEN, TFT_TRANSPARENT);

    Screen_Home();

    // Serial.print("+");

    // disconnectedIterations = 0;
    lastConnectedTime = currentTime;
  }
  // else if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::Connecting)
  // {
    // tft.fillScreen(TFT_BLUE);

    // window.drawString("Connecting...", 20, 30);
    // window.pushSprite(0, 0);

    // tft.drawSmoothCircle(50, 50, 50, TFT_BLUE, TFT_TRANSPARENT);
    // tft.fillSmoothCircle(IWIDTH - 25, 25, 18, TFT_BLUE, TFT_TRANSPARENT);

    // Screen_NoConnection();

    // Serial.print("o");

    // disconnectedIterations = 0;
  // }
  else // Disconnected
  {
    // tft.fillSmoothCircle(IWIDTH - 25, 25, 18, TFT_RED, TFT_TRANSPARENT);

    Screen_NoConnection();
  }

  // delay(25); /* Delay a second between loops */

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

      switch(touch.data.gestureID)
      {
        case CST816S::GESTURE::SWIPE_DOWN:
        case CST816S::GESTURE::SWIPE_UP:
        case CST816S::GESTURE::SWIPE_LEFT:
        case CST816S::GESTURE::SWIPE_RIGHT:
          // tft.fillSmoothCircle(IWIDTH - touch.data.y, touch.data.x, 10, generateRandomColor(), TFT_TRANSPARENT);
          break;
        case CST816S::GESTURE::NONE:
          // tft.fillSmoothCircle(IWIDTH - touch.data.y, touch.data.x, 10, TFT_GREEN, TFT_TRANSPARENT);
          break;
      }
    }
  }

  delay(25);
  // Serial.println("<X>");
}