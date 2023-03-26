#include <Arduino.h>
#include <stdint.h>
#include <string.h>

#include <TFT_eSPI.h> // Master copy here: https://github.com/Bodmer/TFT_eSPI

// #include <BLEDevice.h>

#include <Camera\ConstantsTypes.h>
#include <CCU\CCUUtility.h>
#include <CCU\CCUPacketTypes.h>
#include <CCU\CCUValidationFunctions.h>
#include <Camera\BMDCameraConnection.h>
#include <Camera\BMDCamera.h>
#include "BMDControlSystem.h"

BMDCameraConnection CameraConnection;
std::shared_ptr<BMDControlSystem> BMDControlSystem::instance = nullptr; // Required for Singleton pattern and the constructor for BMDControlSystem
// std::shared_ptr<BMDControlSystem> BMDSystem = BMDControlSystem::getInstance();
// BMDCamera *Camera;

/*

static BLEUUID OutgoingCameraControl("5DD3465F-1AEE-4299-8493-D2ECA2F8E1BB");
static BLEUUID IncomingCameraControl("B864E140-76A0-416A-BF30-5876504537D9");
static BLEUUID Timecode("6D8F2110-86F1-41BF-9AFB-451D87E976C8");
static BLEUUID CameraStatus("7FE8691D-95DC-4FC5-8ABD-CA74339B51B9");
static BLEUUID DeviceName("FFAC0C52-C9FB-41A0-B063-CC76282EB89C");

static BLEUUID ServiceId("00001800-0000-1000-8000-00805f9b34fb");
static BLEUUID BmdCameraService("291D567A-6D75-11E6-8B77-86F30CA893D3");

// https://www.electronicshub.org/esp32-ble-tutorial/
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pBMDChstc_OutgoingCameraControl;
static BLERemoteCharacteristic* pBMDChstc_IncomingCameraControl;
static BLERemoteCharacteristic* pBMDChstc_Timecode;
static BLERemoteCharacteristic* pBMDChstc_CameraStatus;
static BLERemoteCharacteristic* pBMDChstc_DeviceName;
static BLEAdvertisedDevice* bleCamera;
BLEDevice _device;
*/

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
TFT_eSprite window = TFT_eSprite(&tft);

#define IWIDTH 320
#define IHEIGHT 170

/*
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
  Serial.println((char*)pData);
}

class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient* pclient)
  {
    
  }

  void onDisconnect(BLEClient* pclient)
  {
    connected = false;
    Serial.println("onDisconnect");
  }
};
*/

/*

// Start connection to the BLE Server
bool connectToServer()
{
  Serial.print("Forming a connection to ");
  Serial.println(bleCamera->getAddress().toString().c_str());
    
  BLEClient* pClient = _device.createClient();

  if(pClient == nullptr)
  {
    Serial.println("Failed to create Client");
    return false;
  }
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remote BLE Server
  pClient->connect(bleCamera);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server
  BLERemoteService* pRemoteService = pClient->getService(BmdCameraService);
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(BmdCameraService.toString().c_str());
    pClient->disconnect();
    return false;
  }

  pBMDChstc_OutgoingCameraControl = pRemoteService->getCharacteristic(OutgoingCameraControl);
  if (pBMDChstc_OutgoingCameraControl == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(OutgoingCameraControl.toString().c_str());
    return false;
  }

  */

  /*
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server
  pBMDChstc_CameraStatus = pRemoteService->getCharacteristic(CameraStatus);
  if (pBMDChstc_CameraStatus == nullptr)
  {
    Serial.print("Failed to find our Camera Status characteristic UUID: ");
    Serial.println(CameraStatus.toString().c_str());
    pClient->disconnect();
    return false;
  }
    
  // Read the value of the characteristic
  // Initial value is 'Hello, World!'
  if(pBMDChstc_CameraStatus->canRead())
  {
    std::string value = pBMDChstc_CameraStatus->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }
  else
    Serial.println("Characteristic can't be read.");

  if(pBMDChstc_CameraStatus->canNotify())
  {
    pBMDChstc_CameraStatus->registerForNotify(notifyCallback);
  }

    connected = true;
    return true;
}
  */

 /*

/* Scan for BLE servers and find the first one that advertises the service we are looking for.
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
  // Called for each advertising BLE server.
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BmdCameraService))
    {
      Serial.print("Found Blackmagic Camera Service, stopping the scan.");

      _device.getScan()->stop();
      bleCamera = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

*/

void setup() {

  Serial.begin(115200);
  Serial.println("Booting...");

  // Initialise the screen
  tft.init();

  // Ideally set orientation for good viewing angle range because
  // the anti-aliasing effectiveness varies with screen viewing angle
  // Usually this is when screen ribbon connector is at the bottom
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  window.createSprite(IWIDTH, IHEIGHT);
  window.drawString("Blackmagic Camera Control", 20, 20);

  // Serial.print("Forming a connection to ");
  // Serial.println(myDevice->getAddress().toString().c_str());
    
  // BLEClient* pClient = BLEDevice::createClient();
  // Serial.println(" - Created client");

  // pClient->setClientCallbacks(new MyClientCallback());

  /* Connect to the remote BLE Server */
  // pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  // Serial.println(" - Connected to server");

  //BLEDevice::init("ESP32-BLE-Client");

  /*
  _device.init("ESP32-BLE-Client");
  _device.setPower(ESP_PWR_LVL_P9);
  _device.setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  _device.setSecurityCallbacks(new MySecurity());

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
  pSecurity->setCapability(ESP_IO_CAP_IN);
  pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  /* Retrieve a Scanner and set the callback we want to use to be informed when we
     have detected a new device.  Specify that we want active scanning and start the
     scan to run for 5 seconds. */
  // BLEScan* pBLEScan = _device.getScan();
  // pBLEScan->clearResults(); // MS added based on BlueMagic32
  /*
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(false);
  pBLEScan->start(5);

  */

  CameraConnection.initialise();
  CameraConnection.scan();
  CameraConnection.connect();

  // window.drawString("Camera Connection: Initialised", 20, 30);
  window.drawSmoothCircle(IWIDTH - 25, 25, 22, TFT_WHITE, TFT_TRANSPARENT);
  window.pushSprite(0, 0);

  /*
  String hex = "faea6a9aff6a"; // "Hello World" in hex
  byte* byteArray = BMDCore_CCUUtility::StringToByteArray(hex);
  
  Serial.println("StringToByteArray");
  // Print each byte in the array
  for (int i = 0; i < hex.length() / 2; i++) {
    Serial.print(byteArray[i], HEX);
    Serial.print(" ");
  }
  Serial.println("StringToByteArray End");
  
  // Don't forget to free the allocated memory
  delete[] byteArray;
  */
}

// bool recordingTemp = false;

 int disconnectedIterations = 0;

void loop() {
  
  /* If the flag "doConnect" is true, then we have scanned for and found the desired
     BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
     connected we set the connected flag to be true.
  if (doConnect == true)
  {
    delay(2000);
    Serial.println("[doConnect]");
    if (connectToServer())
    {
      Serial.println("We are now connected to the BLE Server.");
    } 
    else
    {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  */

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot

  /*
  if (connected)
  {
    Serial.print("trying to set recording.");
    // Serial.print("[Connected]");
    // String newValue = "Time since boot: " + String(millis()/2000);
    // Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    
    // Set the characteristic's value to be the array of bytes that is actually a string
    // pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
    // pBMDChstc_CameraStatus->writeValue(0x01, 1);
    // You can see this value updated in the Server's Characteristic

    uint8_t data[12] = {255, 5, 0, 0, 10, 1, 1, 0, 0, 0, 0, 0};
     if (!recordingTemp)
     {
    // Triggers recording.
      data[8] = 2;

     }

    // Don't record for now...
    // pBMDChstc_OutgoingCameraControl->writeValue(data, 12, true);

    recordingTemp = !recordingTemp;

    Serial.print("Written, waiting.");

    delay(5000);
  }
  else if(doScan)
  {
    Serial.print(">");
    _device.getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  */

 if(disconnectedIterations == 5)
 {
    disconnectedIterations = 0;
    Serial.println("Disconnected for too long, trying to reconnect");
    CameraConnection.scan();
    CameraConnection.connect();
 }

  if(CameraConnection.status == BMDCameraConnection::ConnectionStatus::Connected)
  {
    // tft.fillScreen(TFT_GREEN);

    // window.drawString("Connected", 20, 30);
    // window.pushSprite(0, 0);

    // tft.drawSmoothCircle(50, 50, 50, TFT_GREEN, TFT_TRANSPARENT);
    tft.fillSmoothCircle(IWIDTH - 25, 25, 18, TFT_GREEN, TFT_TRANSPARENT);

    // Serial.print("+");

    disconnectedIterations = 0;
  }
  else if(CameraConnection.status == BMDCameraConnection::ConnectionStatus::Connecting)
  {
    // tft.fillScreen(TFT_BLUE);

    // window.drawString("Connecting...", 20, 30);
    // window.pushSprite(0, 0);

    // tft.drawSmoothCircle(50, 50, 50, TFT_BLUE, TFT_TRANSPARENT);
    tft.fillSmoothCircle(IWIDTH - 25, 25, 18, TFT_BLUE, TFT_TRANSPARENT);

    // Serial.print("o");

    disconnectedIterations = 0;
  }
  else
  {
    // tft.fillScreen(TFT_RED);

    // window.drawString("Disconnected", 20, 30);
    // window.pushSprite(0, 0);

    // tft.drawSmoothCircle(100, 100, 10, TFT_RED, TFT_TRANSPARENT);
    tft.fillSmoothCircle(IWIDTH - 25, 25, 18, TFT_RED, TFT_TRANSPARENT);

    // Serial.print("-");

    disconnectedIterations++;
  }

  delay(2000); /* Delay a second between loops */

  if(BMDControlSystem::getInstance()->hasCamera())
  {
    // std::shared_ptr<BMDCamera> camera = BMDControlSystem::getInstance()->getCamera();
    auto camera = BMDControlSystem::getInstance()->getCamera();

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
}