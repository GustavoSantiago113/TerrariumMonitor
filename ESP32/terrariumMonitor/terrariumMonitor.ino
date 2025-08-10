#define ENABLE_USER_AUTH
#define ENABLE_STORAGE
#define ENABLE_FS
#define ENABLE_DATABASE

#include <Arduino.h>
#include <FirebaseClient.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include <Wire.h>

// ==== WiFi and Firebase credentials ====
#define WIFI_SSID "*******"
#define WIFI_PASSWORD "*********"
#define API_KEY "*************"
#define USER_EMAIL "*************"
#define USER_PASS "*************"
#define STORAGE_BUCKET_ID "*********************"
#define DATABASE_URL "**********************"

// ==== Hardware Pins ====
#define LED_PIN 1
#define MOTOR1_PIN 2
#define MOTOR2_PIN 42

// ==== I2C Sensor Addresses ====
#define AHT20_I2C_ADDR 0x38
#define TSL2561_I2C_ADDR 0x39

// ==== Camera Pins (adjust for your board) ====
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  15
#define SIOD_GPIO_NUM  4
#define SIOC_GPIO_NUM  5
#define Y2_GPIO_NUM 11
#define Y3_GPIO_NUM 9
#define Y4_GPIO_NUM 8
#define Y5_GPIO_NUM 10
#define Y6_GPIO_NUM 12
#define Y7_GPIO_NUM 18
#define Y8_GPIO_NUM 17
#define Y9_GPIO_NUM 16
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM  7
#define PCLK_GPIO_NUM  13
#define LED_GPIO_NUM -1

// ==== Firebase Objects ====
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASS, 3000);
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;
Storage storage;

// ==== File System ====
#define FILE_PHOTO_PATH "/photo.jpg"
FileConfig media_file(FILE_PHOTO_PATH, nullptr);
File myFile;

// ==== Timing ====
unsigned long lastDataMillis = 0;
unsigned long lastPhotoMillis = 0;
const unsigned long sendInterval = 5 * 60 * 1000; // 5 minutes

// ==== Sensor Values ====
float lastLux = 0;
float lastTemp = 0;
float lastHumi = 0;

// ==== Function Prototypes ====
void processData(AsyncResult &aResult);
void file_operation_callback(File &file, const char *filename, file_operating_mode mode);
void capturePhotoSaveLittleFS();
void initLittleFS();
void initWiFi();
void initCamera();
void initSensors();
bool readAHT20(float &temperature, float &humidity);
bool readTSL2561(float &lux);
String getUniquePhotoPath();
void controlActuators();

// ==== I2C Sensor Functions ====

void initSensors() {
  Wire.begin();
  Serial.println("Initializing I2C sensors...");
  
  // Initialize AHT20
  Wire.beginTransmission(AHT20_I2C_ADDR);
  Wire.write(0xBE);  // Initialize command
  Wire.write(0x08);
  Wire.write(0x00);
  if (Wire.endTransmission() == 0) {
    Serial.println("AHT20 initialized successfully");
  } else {
    Serial.println("Failed to initialize AHT20");
  }
  
  delay(100);
  
  // Initialize TSL2561
  Wire.beginTransmission(TSL2561_I2C_ADDR);
  Wire.write(0x80);  // Command register
  Wire.write(0x03);  // Power on
  if (Wire.endTransmission() == 0) {
    Serial.println("TSL2561 initialized successfully");
    
    // Set integration time to 101ms
    Wire.beginTransmission(TSL2561_I2C_ADDR);
    Wire.write(0x81);  // Timing register
    Wire.write(0x01);  // 101ms integration time
    Wire.endTransmission();
    
    // Set gain to 1x
    Wire.beginTransmission(TSL2561_I2C_ADDR);
    Wire.write(0x87);  // Gain register
    Wire.write(0x00);  // 1x gain
    Wire.endTransmission();
  } else {
    Serial.println("Failed to initialize TSL2561");
  }
}

bool readAHT20(float &temperature, float &humidity) {
  // Trigger measurement
  Wire.beginTransmission(AHT20_I2C_ADDR);
  Wire.write(0xAC);  // Trigger measurement command
  Wire.write(0x33);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    Serial.println("Failed to trigger AHT20 measurement");
    return false;
  }
  
  delay(80); // Wait for measurement to complete
  
  // Check if measurement is ready
  int retries = 0;
  bool ready = false;
  while (retries < 10 && !ready) {
    Wire.requestFrom(AHT20_I2C_ADDR, 1);
    if (Wire.available()) {
      uint8_t status = Wire.read();
      if ((status & 0x80) == 0) { // Bit 7 = 0 means measurement is complete
        ready = true;
      } else {
        delay(10);
        retries++;
      }
    } else {
      delay(10);
      retries++;
    }
  }
  
  if (!ready) {
    Serial.println("AHT20 measurement timeout");
    return false;
  }
  
  // Read measurement data
  Wire.requestFrom(AHT20_I2C_ADDR, 6);
  if (Wire.available() != 6) {
    Serial.println("Failed to read AHT20 data");
    return false;
  }
  
  uint8_t data[6];
  for (int i = 0; i < 6; i++) {
    data[i] = Wire.read();
  }
  
  // Parse humidity data (20 bits)
  uint32_t humidity_raw = ((uint32_t)data[1] << 12) | 
                         ((uint32_t)data[2] << 4) | 
                         ((uint32_t)data[3] >> 4);
  
  // Parse temperature data (20 bits)
  uint32_t temperature_raw = ((uint32_t)(data[3] & 0x0F) << 16) | 
                            ((uint32_t)data[4] << 8) | 
                            (uint32_t)data[5];
  
  // Convert to actual values
  humidity = ((float)humidity_raw / 1048576.0) * 100.0;
  temperature = ((float)temperature_raw / 1048576.0) * 200.0 - 50.0;
  
  return true;
}

bool readTSL2561(float &lux) {
  // Read channel 0 (visible + IR)
  Wire.beginTransmission(TSL2561_I2C_ADDR);
  Wire.write(0x8C);  // DATA0LOW register
  Wire.endTransmission();
  Wire.requestFrom(TSL2561_I2C_ADDR, 2);
  
  if (Wire.available() != 2) {
    Serial.println("Failed to read TSL2561 channel 0");
    return false;
  }
  
  uint16_t ch0 = Wire.read();
  ch0 |= (Wire.read() << 8);
  
  // Read channel 1 (IR only)
  Wire.beginTransmission(TSL2561_I2C_ADDR);
  Wire.write(0x8E);  // DATA1LOW register
  Wire.endTransmission();
  Wire.requestFrom(TSL2561_I2C_ADDR, 2);
  
  if (Wire.available() != 2) {
    Serial.println("Failed to read TSL2561 channel 1");
    return false;
  }
  
  uint16_t ch1 = Wire.read();
  ch1 |= (Wire.read() << 8);
  
  // Calculate lux using TSL2561 algorithm
  // Integration time: 101ms, Gain: 1x
  float ratio = 0;
  if (ch0 != 0) {
    ratio = (float)ch1 / (float)ch0;
  }
  
  float luxValue = 0;
  if (ratio <= 0.50) {
    luxValue = 0.0304 * ch0 - 0.062 * ch0 * pow(ratio, 1.4);
  } else if (ratio <= 0.61) {
    luxValue = 0.0224 * ch0 - 0.031 * ch1;
  } else if (ratio <= 0.80) {
    luxValue = 0.0128 * ch0 - 0.0153 * ch1;
  } else if (ratio <= 1.30) {
    luxValue = 0.00146 * ch0 - 0.00112 * ch1;
  } else {
    luxValue = 0;
  }
  
  lux = luxValue > 0 ? luxValue : 0;
  return true;
}

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  initWiFi();
  configTime(-5 * 3600, 3600, "pool.ntp.org", "time.nist.gov");

  //initCamera();
  //initLittleFS();
  initSensors();

  // Initialize actuators
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR1_PIN, OUTPUT);
  pinMode(MOTOR2_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(MOTOR1_PIN, LOW);
  digitalWrite(MOTOR2_PIN, LOW);

  // Firebase
  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);
  initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  app.getApp<Storage>(storage);
}

// ==== Main Loop ====
void loop() {
  app.loop();

  // Continually monitor sensors
  if (readTSL2561(lastLux)) {
    // Light sensor read successfully
  } else {
    Serial.println("Failed to read light sensor");
  }
  
  if (readAHT20(lastTemp, lastHumi)) {
    // Temperature and humidity read successfully
  } else {
    Serial.println("Failed to read temperature/humidity sensor");
  }

  // Actuator logic
  controlActuators();

  // Every 5 minutes, send data and image
  if (app.ready() && (millis() - lastDataMillis >= sendInterval)) {
    lastDataMillis = millis();

    // Send sensor data
    unsigned long now = millis();
    String key = String(now);
    Database.set<float>(aClient, "/spider_monitor_images/lux/" + key, lastLux, processData, "RTDB_Send_LUX");
    Database.set<float>(aClient, "/spider_monitor_images/temp/" + key, lastTemp, processData, "RTDB_Send_TEMP");
    Database.set<float>(aClient, "/spider_monitor_images/humi/" + key, lastHumi, processData, "RTDB_Send_HUMI");

    // Capture and send image
    //capturePhotoSaveLittleFS();
    //String uniquePath = getUniquePhotoPath();
    //storage.upload(aClient, FirebaseStorage::Parent(STORAGE_BUCKET_ID, uniquePath.c_str()), getFile(media_file), "image/jpg", processData, "⬆️  uploadTask");
  }
}

// ==== Actuator Logic ====
void controlActuators() {
  // Get current time
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  int hour = timeinfo.tm_hour;

  // LED logic: If luminosity < 50 and between 7am and 7pm, turn LEDs on
  if (lastLux < 0.45 && hour >= 7 && hour < 19) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  // Motor logic: If temp > 33 or humidity > 75, turn both motors on
  if (lastTemp > 33 || lastHumi > 75) {
    digitalWrite(MOTOR1_PIN, HIGH);
    digitalWrite(MOTOR2_PIN, HIGH);
  } else {
    digitalWrite(MOTOR1_PIN, LOW);
    digitalWrite(MOTOR2_PIN, LOW);
  }
}

// ==== Camera and File System Functions ====
void capturePhotoSaveLittleFS() {
  camera_fb_t* fb = NULL;
  for (int i = 0; i < 10; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = NULL;
  }
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }
  File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  } else {
    file.write(fb->buf, fb->len);
    Serial.print("The picture has been saved in ");
    Serial.print(FILE_PHOTO_PATH);
    Serial.print(" - Size: ");
    Serial.print(fb->len);
    Serial.println(" bytes");
  }
  file.close();
  esp_camera_fb_return(fb);
  delay(100);
}

void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An Error has occurred while mounting LittleFS");
    ESP.restart();
  } else {
    delay(500);
    Serial.println("LittleFS mounted successfully");
  }
}

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 2;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 2;
    config.fb_count = 1;
  }
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 0);       // -2 to 2
  s->set_saturation(s, 0);     // -2 to 2
  s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
  s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
  s->set_aec2(s, 0);           // 0 = disable , 1 = enable
  s->set_ae_level(s, 0);       // -2 to 2
  s->set_aec_value(s, 300);    // 0 to 1200
  s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);       // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
  s->set_bpc(s, 0);            // 0 = disable , 1 = enable
  s->set_wpc(s, 1);            // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
  s->set_lenc(s, 1);           // 0 = disable , 1 = enable
  s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
  s->set_vflip(s, 0);          // 0 = disable , 1 = enable
  s->set_dcw(s, 1);            // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
  Serial.println("Camera init success");
}

String getUniquePhotoPath() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    now = millis();
  } else {
    now = mktime(&timeinfo);
  }
  return "/spider_monitor_images/photo_" + String(now) + ".jpg";
}

// ==== Firebase Callbacks ====
void processData(AsyncResult &aResult) {
  if (!aResult.isResult())
    return;
  if (aResult.isEvent())
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
  if (aResult.isDebug())
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
  if (aResult.isError())
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
  if (aResult.downloadProgress())
    Firebase.printf("Downloaded, task: %s, %d%s (%d of %d)\n", aResult.uid().c_str(), aResult.downloadInfo().progress, "%", aResult.downloadInfo().downloaded, aResult.downloadInfo().total);
  if (aResult.uploadProgress()) {
    Firebase.printf("Uploaded, task: %s, %d%s (%d of %d)\n", aResult.uid().c_str(), aResult.uploadInfo().progress, "%", aResult.uploadInfo().uploaded, aResult.uploadInfo().total);
    if (aResult.uploadInfo().total == aResult.uploadInfo().uploaded) {
      Firebase.printf("Upload task: %s, complete!✅️\n", aResult.uid().c_str());
      Serial.print("Download URL: ");
      Serial.println(aResult.uploadInfo().downloadUrl);
    }
  }
}

void file_operation_callback(File &file, const char *filename, file_operating_mode mode) {
  switch (mode) {
    case file_mode_open_read:
      myFile = LittleFS.open(filename, "r");
      if (!myFile || !myFile.available()) {
        Serial.println("[ERROR] Failed to open file for reading");
      }
      break;
    case file_mode_open_write:
      myFile = LittleFS.open(filename, "w");
      break;
    case file_mode_open_append:
      myFile = LittleFS.open(filename, "a");
      break;
    case file_mode_remove:
      LittleFS.remove(filename);
      break;
    default:
      break;
  }
  file = myFile;
}