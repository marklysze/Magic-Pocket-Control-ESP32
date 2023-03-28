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
#include <Camera\ConstantsTypes.h>
#include <CCU\CCUUtility.h>
#include <CCU\CCUPacketTypes.h>
#include <CCU\CCUValidationFunctions.h>
#include <Camera\BMDCameraConnection.h>
#include <Camera\BMDCamera.h>
#include "BMDControlSystem.h"

// Remove later
#include <random>
#include <cstdint>


// https://github.com/fbiego/CST816S
// Touch with Gesture for the touch controller on the T-Display-S3
CST816S touch(PIN_IIC_SDA, PIN_IIC_SCL, PIN_TOUCH_RES, PIN_TOUCH_INT);

BMDCameraConnection cameraConnection;
std::shared_ptr<BMDControlSystem> BMDControlSystem::instance = nullptr; // Required for Singleton pattern and the constructor for BMDControlSystem
BMDCameraConnection* BMDCameraConnection::instancePtr = &cameraConnection; // Required for the scan function to run non-blocking and call the object back with the result

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
TFT_eSprite window = TFT_eSprite(&tft);

#define IWIDTH 320
#define IHEIGHT 170

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

/*
TaskHandle_t myTaskHandle;

void myTask(void *parameter)
{
  for(;;)
  {
    // Serial.print("myTask running on core: ");
    // Serial.println(xPortGetCoreID());

    // delay(500);
  if (touch.available()) {
    // Serial.print(touch.gesture());
    // Serial.print("\t");
    // Serial.print(touch.event());
    // Serial.print("\t");
    Serial.print(touch.data.x);
    Serial.print("\t");
    Serial.println(touch.data.y);
       
    if(false && touch.data.eventID == CST816S::TOUCHEVENT::UP)
    {
      int oriented_x = IWIDTH - touch.data.y;
      int oriented_y = touch.data.x;

      switch(touch.data.gestureID)
      {
        case CST816S::GESTURE::SWIPE_DOWN:
        case CST816S::GESTURE::SWIPE_UP:
        case CST816S::GESTURE::SWIPE_LEFT:
        case CST816S::GESTURE::SWIPE_RIGHT:
          tft.fillSmoothCircle(IWIDTH - touch.data.y, touch.data.x, 10, generateRandomColor(), TFT_TRANSPARENT);
          break;
        case CST816S::GESTURE::NONE:
          tft.fillSmoothCircle(IWIDTH - touch.data.y, touch.data.x, 10, TFT_YELLOW, TFT_TRANSPARENT);
          break;
      }
    }
  }
  }
}
*/

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

  // Prepare for Bluetooth connections and start scanning for cameras
  cameraConnection.initialise();
  // cameraConnection.scan();
  // cameraConnection.connect();

  window.drawSmoothCircle(IWIDTH - 25, 25, 22, TFT_WHITE, TFT_TRANSPARENT);
  window.pushSprite(0, 0);

  // Start capturing touchscreen touches
  touch.begin();

  // Create task for multi-tasking
  /*
  xTaskCreatePinnedToCore(
    myTask,
    "MyTask",
    1000,
    NULL,
    0,
    &myTaskHandle,
    0);
    */
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
    Serial.println("Status Scanning Found. Connecting.");
    cameraConnection.status = BMDCameraConnection::Connecting;
    cameraConnection.connect();
    Serial.println("Connecting Finished.");
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
    tft.fillSmoothCircle(IWIDTH - 25, 25, 18, TFT_GREEN, TFT_TRANSPARENT);

    // Serial.print("+");

    // disconnectedIterations = 0;
    lastConnectedTime = currentTime;
  }
  else if(cameraConnection.status == BMDCameraConnection::ConnectionStatus::Connecting)
  {
    // tft.fillScreen(TFT_BLUE);

    // window.drawString("Connecting...", 20, 30);
    // window.pushSprite(0, 0);

    // tft.drawSmoothCircle(50, 50, 50, TFT_BLUE, TFT_TRANSPARENT);
    tft.fillSmoothCircle(IWIDTH - 25, 25, 18, TFT_BLUE, TFT_TRANSPARENT);

    // Serial.print("o");

    // disconnectedIterations = 0;
  }
  else // Disconnected
  {
    // tft.fillScreen(TFT_RED);

    // window.drawString("Disconnected", 20, 30);
    // window.pushSprite(0, 0);

    // tft.drawSmoothCircle(100, 100, 10, TFT_RED, TFT_TRANSPARENT);
    tft.fillSmoothCircle(IWIDTH - 25, 25, 18, TFT_RED, TFT_TRANSPARENT);

    // Serial.print("-");

    // disconnectedIterations++;
  }

  // delay(25); /* Delay a second between loops */

  if(BMDControlSystem::getInstance()->hasCamera())
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
          tft.fillSmoothCircle(IWIDTH - touch.data.y, touch.data.x, 10, generateRandomColor(), TFT_TRANSPARENT);
          break;
        case CST816S::GESTURE::NONE:
          tft.fillSmoothCircle(IWIDTH - touch.data.y, touch.data.x, 10, TFT_GREEN, TFT_TRANSPARENT);
          break;
      }
    }
  }

  delay(25);
  // Serial.println("<X>");
}