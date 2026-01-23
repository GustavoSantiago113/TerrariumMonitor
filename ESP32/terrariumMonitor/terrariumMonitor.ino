#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_camera.h"
#include <Wire.h>
#include "secrets.h"
#include "sensors.h"
#include "actuators.h"

// ==== Camera Pins ====
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

// ==== API and WiFi Configuration in secrets.h ====

// ==== Timing ====
unsigned long lastDataMillis = 0;
const unsigned long sendInterval = 2 * 60 * 1000; // 2 minutes

// Track last scheduled upload to avoid duplicate uploads within the same hour/day
int lastUploadDay = -1;
int lastUploadHour = -1;

// ==== Sensor Values ====
float lastLux = 0;
float lastTemp = 0;
float lastHumi = 0;

// ==== Function Prototypes ====
void capturePhotoSaveLittleFS(const String &photoPath);
void initLittleFS();
void initWiFi();
void initCamera();
bool sendDataToAPI(const String &imagePath, float lux, float temp, float humi, time_t timestamp);
size_t fsTotalBytes();
size_t fsUsedBytes();
size_t fsFreeBytes();
void printFSInfo(const char *tag = "FS");
void deleteAllJpgFiles();
bool ensureFsFree(size_t minFreeBytes);

// ==== File System ====
String uniquePath = "";

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  Serial.println("Starting ESP32-S3-CAM + Sensor System...");
  
  pinMode(LED_BUILTIN, OUTPUT); // Built-in LED for error indication
  digitalWrite(LED_BUILTIN, LOW); // Turn off LED initially

  // Test motors and lights
  Serial.println("Testing motors and lights...");
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR1_PIN, OUTPUT);
  pinMode(MOTOR2_PIN, OUTPUT);

  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);

  digitalWrite(MOTOR1_PIN, HIGH);
  delay(500);
  digitalWrite(MOTOR1_PIN, LOW);
  delay(500);

  digitalWrite(MOTOR2_PIN, HIGH);
  delay(500);
  digitalWrite(MOTOR2_PIN, LOW);
  delay(500);

  Serial.println("Motors and lights tested successfully.");

  // Initialize WiFi
  Serial.println("Connecting to WiFi...");
  initWiFi();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed!");
    digitalWrite(LED_BUILTIN, HIGH); // Turn on error LED
    while (true) delay(1000); // Halt execution
  }
  Serial.println("WiFi connected!");

  // Configure time
  configTime(-5 * 3600, 3600, "pool.ntp.org", "time.nist.gov");

  // Initialize camera
  Serial.println("Initializing camera...");
  initCamera();
  if (!esp_camera_sensor_get()) {
    Serial.println("Camera initialization failed!");
    digitalWrite(LED_BUILTIN, HIGH); // Turn on error LED
    while (true) delay(1000); // Halt execution
  }
  Serial.println("Camera initialized successfully.");

  // Initialize LittleFS
  Serial.println("Initializing LittleFS...");
  initLittleFS();
  printFSInfo("Boot");
  // Make sure we have enough headroom before first capture
  // Target at least ~300KB free to accommodate a VGA/low-quality JPEG
  if (!ensureFsFree(300 * 1024)) {
    Serial.println("Warning: Not enough free FS space even after cleanup.");
  }

  // Initialize sensors
  Serial.println("Initializing sensors...");
  initSensors();

  Serial.println("Setup complete!");
}

// ==== Main Loop ====
void loop() {
  // Every 2 minutes, send data and image
  if (millis() - lastDataMillis >= sendInterval) {
    // Proactively ensure some free space for regular operation
    ensureFsFree(250 * 1024);

    // Monitor sensors
    Serial.println("Reading sensors...");
    readSensors(lastLux, lastTemp, lastHumi);
    Serial.print("✓ Light: ");
    Serial.print(lastLux);
    Serial.println(" lux");
    Serial.print("✓ Temperature: ");
    Serial.print(lastTemp);
    Serial.print("°C, Humidity: ");
    Serial.print(lastHumi);
    Serial.println("%");

    // Actuate as before
    controlActuators(lastLux, lastTemp, lastHumi);

    lastDataMillis = millis();

    // Get current timestamp / local time
    time_t now;
    struct tm timeinfo;
    bool haveTime = getLocalTime(&timeinfo);
    if (haveTime) {
      now = mktime(&timeinfo);
    } else {
      now = millis() / 1000; // Fallback to uptime if NTP fails
    }

    // Decide whether to capture image locally: only at ~9:00 and ~16:00 once per day
    bool shouldCapture = false;
    if (haveTime) {
      int currentHour = timeinfo.tm_hour;
      int currentMin = timeinfo.tm_min;
      int intervalMin = (int)(sendInterval / 60000);
      if ((currentHour == 9 || currentHour == 16) && currentMin < intervalMin) {
        if (!(lastUploadDay == timeinfo.tm_mday && lastUploadHour == currentHour)) {
          shouldCapture = true;
          lastUploadDay = timeinfo.tm_mday;
          lastUploadHour = currentHour;
        }
      }
    }

    if (shouldCapture) {
      Serial.println("Scheduled time reached — capturing photo and uploading image+data...");
      // Delete old image if it exists
      if (uniquePath.length() > 0 && LittleFS.exists(uniquePath)) {
        Serial.println("Deleting old image...");
        if (LittleFS.remove(uniquePath)) {
          Serial.print("Successfully deleted: ");
          Serial.println(uniquePath);
        } else {
          Serial.print("Failed to delete: ");
          Serial.println(uniquePath);
        }
      }

      uniquePath = "/" + String(now) + ".jpg";
      // Turn on light before photo
      Serial.println("Turning on light for photo...");
      digitalWrite(LED_PIN, HIGH);
      delay(300); // allow light to stabilize
      capturePhotoSaveLittleFS(uniquePath);
      // Turn off light after photo
      digitalWrite(LED_PIN, LOW);
      Serial.println("Light turned off after photo.");

      // Upload image + data
      Serial.println("Sending image+data to API...");
      if (sendDataToAPI(uniquePath, lastLux, lastTemp, lastHumi, now)) {
        Serial.println("✅ Image+data upload successful");
      } else {
        Serial.println("❌ Image+data upload failed");
      }

      // Remove local image after upload
      if (LittleFS.remove(uniquePath)) {
        Serial.println("Image deleted.");
        uniquePath = "";
      }
      printFSInfo("PostUpload");
    } else {
      // Not capture time: upload data-only
      Serial.println("Uploading sensor data (no image)...");
      if (sendSensorDataToAPI(lastLux, lastTemp, lastHumi, now)) {
        Serial.println("✅ Data-only upload successful");
      } else {
        Serial.println("❌ Data-only upload failed");
      }

      Serial.println("Not capture time — skipping photo.");
      printFSInfo("NoCaptureCycle");
    }
  }
}

// ==== Camera and File System Functions ====
void capturePhotoSaveLittleFS(const String &photoPath) {
  camera_fb_t* fb = NULL;
  for (int i = 0; i < 3; i++) {
    fb = esp_camera_fb_get();
    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
    }
    delay(100);
  }

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  Serial.printf("Picture file name: %s\n", photoPath.c_str());

  // ensure parent directory exists
  int slash = photoPath.lastIndexOf('/');
  String dir = (slash > 0) ? photoPath.substring(0, slash) : String("/");
  if (dir.length() > 1 && !LittleFS.exists(dir)) {
    Serial.print("Creating directory: ");
    Serial.println(dir);
    if (!LittleFS.mkdir(dir)) {
      Serial.print("Failed to create directory: ");
      Serial.println(dir);
      // fall back to root filename
    } else {
      Serial.println("Directory created");
    }
  }

  File file = LittleFS.open(photoPath, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode - aborting save");
    esp_camera_fb_return(fb);
    return;
  }

  // write, flush and close to ensure content is fully committed to LittleFS
  size_t written = file.write(fb->buf, fb->len);
  file.flush();
  file.close();

  Serial.print("The picture has been saved in ");
  Serial.print(photoPath);
  Serial.print(" - Size written: ");
  Serial.print(written);
  Serial.print(" bytes (fb len: ");
  Serial.print(fb->len);
  Serial.println(")");

  esp_camera_fb_return(fb);
  delay(100);

  // verify file exists and size matches before setting media_file
  if (LittleFS.exists(photoPath)) {
    File check = LittleFS.open(photoPath, "r");
    if (check) {
      size_t sz = check.size();
      check.close();
      if (sz == written && written > 0) {
        Serial.print("Photo saved successfully: ");
        Serial.println(photoPath);
        return;
      }
      Serial.printf("Warning: written %u bytes but file size is %u\n", (unsigned)written, (unsigned)sz);
    } else {
      Serial.println("Warning: unable to open file for verification");
    }
  } else {
    Serial.println("Warning: file does not exist after write");
  }

  // fallback: do not update media_file if verification failed
  Serial.println("Capture saved but media_file not updated due to verification failure.");
}

void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An Error has occurred while mounting LittleFS");
    ESP.restart();
  } else {
    Serial.println("LittleFS mounted successfully");
  }
}

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
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
    config.jpeg_quality = 12;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
  
  // Camera settings
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 0);
  s->set_contrast(s, 0);
  s->set_saturation(s, 0);
  s->set_special_effect(s, 0);
  s->set_whitebal(s, 1);
  s->set_awb_gain(s, 1);
  s->set_wb_mode(s, 0);
  s->set_exposure_ctrl(s, 1);
  s->set_aec2(s, 0);
  s->set_ae_level(s, 0);
  s->set_aec_value(s, 300);
  s->set_gain_ctrl(s, 1);
  s->set_agc_gain(s, 0);
  s->set_gainceiling(s, (gainceiling_t)0);
  s->set_bpc(s, 0);
  s->set_wpc(s, 1);
  s->set_raw_gma(s, 1);
  s->set_lenc(s, 1);
  s->set_hmirror(s, 0);
  s->set_vflip(s, 0);
  s->set_dcw(s, 1);
  s->set_colorbar(s, 0);
  
  Serial.println("Camera initialized successfully");
}

// ==== FS Utilities ====
size_t fsTotalBytes() {
  return LittleFS.totalBytes();
}

size_t fsUsedBytes() {
  return LittleFS.usedBytes();
}

size_t fsFreeBytes() {
  size_t total = fsTotalBytes();
  size_t used = fsUsedBytes();
  return (total > used) ? (total - used) : 0;
}

void printFSInfo(const char *tag) {
  size_t total = fsTotalBytes();
  size_t used = fsUsedBytes();
  size_t freeB = (total > used) ? (total - used) : 0;
  Serial.printf("[%s] LittleFS - Total: %u bytes, Used: %u bytes, Free: %u bytes\n", tag, (unsigned)total, (unsigned)used, (unsigned)freeB);
}

void deleteAllJpgFiles() {
  Serial.println("Cleaning up .jpg files in LittleFS...");
  File root = LittleFS.open("/");
  if (!root) {
    Serial.println("Failed to open root directory for cleanup");
    return;
  }
  File file = root.openNextFile();
  int removed = 0;
  while (file) {
    String name = file.name();
    if (!name.startsWith("/")) name = String("/") + name; // normalize path
    file.close();
    if (name.endsWith(".jpg")) {
      if (LittleFS.remove(name)) {
        removed++;
        Serial.print("Removed: ");
        Serial.println(name);
      } else {
        Serial.print("Failed to remove: ");
        Serial.println(name);
      }
    }
    file = root.openNextFile();
  }
  root.close();
  Serial.printf("Cleanup complete. Files removed: %d\n", removed);
}

bool ensureFsFree(size_t minFreeBytes) {
  size_t freeB = fsFreeBytes();
  if (freeB >= minFreeBytes) {
    return true;
  }
  Serial.printf("Free space low (%u bytes). Target: %u. Attempting cleanup...\n", (unsigned)freeB, (unsigned)minFreeBytes);
  deleteAllJpgFiles();
  freeB = fsFreeBytes();
  printFSInfo("AfterCleanup");
  return freeB >= minFreeBytes;
}

// ==== Data-only API Upload Function ====
bool sendSensorDataToAPI(float lux, float temp, float humi, time_t timestamp) {
  HTTPClient http;
  http.begin(API_ENDPOINT);
  http.addHeader(API_KEY_HEADER, API_KEY_VALUE);

  // Use multipart/form-data to remain compatible with server expecting form fields
  String boundary = "----WebKitFormBoundary" + String(random(0xffff), HEX);
  http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

  String body = "";
  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"timestamp\"\r\n\r\n";
  body += String(timestamp) + "\r\n";

  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"luminosity\"\r\n\r\n";
  body += String(lux, 2) + "\r\n";

  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"temperature\"\r\n\r\n";
  body += String(temp, 2) + "\r\n";

  body += "--" + boundary + "\r\n";
  body += "Content-Disposition: form-data; name=\"humidity\"\r\n\r\n";
  body += String(humi, 2) + "\r\n";

  body += "--" + boundary + "--\r\n";

  http.setTimeout(15000);
  int httpCode = http.POST(body);
  if (httpCode > 0) {
    Serial.printf("HTTP Response code (data-only): %d\n", httpCode);
    String response = http.getString();
    Serial.println("Response: " + response);
    http.end();
    return (httpCode == 200 || httpCode == 201);
  } else {
    Serial.printf("HTTP request failed (data-only), error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }
}

// ==== API Upload Function ====
bool sendDataToAPI(const String &imagePath, float lux, float temp, float humi, time_t timestamp) {
  if (!LittleFS.exists(imagePath)) {
    Serial.println("Image file does not exist!");
    return false;
  }

  File imageFile = LittleFS.open(imagePath, "r");
  if (!imageFile) {
    Serial.println("Failed to open image file!");
    return false;
  }

  size_t fileSize = imageFile.size();
  Serial.printf("Image size: %u bytes\n", (unsigned)fileSize);

  HTTPClient http;
  http.begin(API_ENDPOINT);
  http.addHeader(API_KEY_HEADER, API_KEY_VALUE);

  // Generate boundary for multipart/form-data
  String boundary = "----WebKitFormBoundary" + String(random(0xffff), HEX);
  http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

  // Build multipart form data body
  String bodyStart = "";
  
  // Add timestamp field
  bodyStart += "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"timestamp\"\r\n\r\n";
  bodyStart += String(timestamp) + "\r\n";
  
  // Add luminosity field
  bodyStart += "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"luminosity\"\r\n\r\n";
  bodyStart += String(lux, 2) + "\r\n";
  
  // Add temperature field
  bodyStart += "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"temperature\"\r\n\r\n";
  bodyStart += String(temp, 2) + "\r\n";
  
  // Add humidity field
  bodyStart += "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"humidity\"\r\n\r\n";
  bodyStart += String(humi, 2) + "\r\n";
  
  // Add image field header
  bodyStart += "--" + boundary + "\r\n";
  bodyStart += "Content-Disposition: form-data; name=\"images\"; filename=\"" + String(timestamp) + ".jpg\"\r\n";
  bodyStart += "Content-Type: image/jpeg\r\n\r\n";
  
  String bodyEnd = "\r\n--" + boundary + "--\r\n";
  
  // Build complete multipart body
  String completeBody = bodyStart;
  
  // Read image data and append to body
  uint8_t buffer[1024];
  while (imageFile.available()) {
    size_t bytesRead = imageFile.read(buffer, sizeof(buffer));
    for (size_t i = 0; i < bytesRead; i++) {
      completeBody += (char)buffer[i];
    }
  }
  imageFile.close();
  
  // Append body end
  completeBody += bodyEnd;
  
  Serial.printf("Total payload size: %u bytes\n", completeBody.length());
  
  // Set timeout
  http.setTimeout(30000); // 30 seconds
  
  // Send POST request
  int httpCode = http.POST(completeBody);
  
  if (httpCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpCode);
    String response = http.getString();
    Serial.println("Response: " + response);
    http.end();
    return (httpCode == 200 || httpCode == 201);
  } else {
    Serial.printf("HTTP request failed, error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }
}