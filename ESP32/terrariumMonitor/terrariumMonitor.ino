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

// ==== Firebase Objects ====
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASS, 3000);
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;
Storage storage;

// ==== Timing ====
unsigned long lastDataMillis = 0;
const unsigned long sendInterval = 2 * 60 * 1000; // 2 minutes

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
size_t fsTotalBytes();
size_t fsUsedBytes();
size_t fsFreeBytes();
void printFSInfo(const char *tag = "FS");
void deleteAllJpgFiles();
bool ensureFsFree(size_t minFreeBytes);

// ==== File System ====
FileConfig media_file(FILE_PHOTO_PATH, file_operation_callback);
File myFile;
String uniquePath = "";
String lastUploadedPath = ""; // track the file we most recently sent

void file_operation_callback(File &file, const char *filename, file_operating_mode mode) {
  switch (mode) {
    case file_mode_open_read:
      myFile = LittleFS.open(filename, "r");
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

  // Firebase setup
  Serial.println("Initializing Firebase...");
  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);
  initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  app.getApp<Storage>(storage);

  Serial.println("Setup complete!");
}

// ==== Main Loop ====
void loop() {
  app.loop();

  // Every 2 minutes, send data and image
  if (app.ready() && (millis() - lastDataMillis >= sendInterval)) {
  // Proactively ensure we have free space for the next photo
  ensureFsFree(250 * 1024);

    // Monitor sensors with error handling
    Serial.println("Reading sensors...");
    
    // Use external sensor reading function
    readSensors(lastLux, lastTemp, lastHumi);
    
    Serial.print("✓ Light: ");
    Serial.print(lastLux);
    Serial.println(" lux");
    Serial.print("✓ Temperature: ");
    Serial.print(lastTemp);
    Serial.print("°C, Humidity: ");
    Serial.print(lastHumi);
    Serial.println("%");

    // Use external actuator control function
    controlActuators(lastLux, lastTemp, lastHumi);
    
    lastDataMillis = millis();
    Serial.println("Sending data to Firebase...");

    // Send sensor data
    time_t now;
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    now = mktime(&timeinfo);
    Database.set<float>(aClient, "/terrarium_monitor_images/lux/" + String(now), lastLux, processData, "RTDB_Send_LUX");
    Database.set<float>(aClient, "/terrarium_monitor_images/temp/" + String(now), lastTemp, processData, "RTDB_Send_TEMP");
    Database.set<float>(aClient, "/terrarium_monitor_images/humi/" + String(now), lastHumi, processData, "RTDB_Send_HUMI");

    // Capture and send image
    Serial.println("Capturing photo...");
    uniquePath = "/" + String(now) + ".jpg";
    capturePhotoSaveLittleFS(uniquePath);

    // make absolutely sure FileConfig points to the new file right before upload
    media_file.setFile(uniquePath.c_str(), file_operation_callback);
    Serial.print("Uploading file: ");
    Serial.println(uniquePath);

    // Keep a copy to delete on successful upload callback
    lastUploadedPath = uniquePath;

    String firebaseStoragePath = "terrarium_monitor_images/" + String(now) + ".jpg";
    storage.upload(aClient, FirebaseStorage::Parent(STORAGE_BUCKET_ID, firebaseStoragePath.c_str()), getFile(media_file), "image/jpg", processData, "⬆️  uploadTask");
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
        // Update media_file to point to the new photo (ensure FileConfig points to current file)
        media_file.setFile(photoPath.c_str(), file_operation_callback);
        Serial.print("media_file set to: ");
        Serial.println(photoPath);
        delay(50); // small pause to ensure FS is stable before upload
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
  
  // Reduce frame size and increase jpeg_quality number to minimize file size on FS
  // Typical resulting JPEG size at VGA/quality ~20 is ~40-90KB depending on scene
  config.frame_size = FRAMESIZE_VGA;    // 640x480
  config.jpeg_quality = 17;             // 10=high quality (big), 63=lowest quality (small)
  config.fb_count = 1;
  
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

// ==== Firebase Callbacks ====
// Helper: robustly close and remove the tracked uploaded file (with retries)
void deleteLocalUploadedFile(const String &path, const char *contextTag) {
  if (path.length() == 0) return;
  if (!LittleFS.exists(path)) return;

  // Close global handle if it matches
  if (myFile) {
    String mfName = String(myFile.name());
    if (!mfName.startsWith("/")) mfName = String("/") + mfName;
    String p = path;
    if (!p.startsWith("/")) p = String("/") + p;
    if (mfName == p) {
      myFile.close();
      Serial.println("Closed global myFile handle before removal");
    }
  }

  // Temporary open/close to help release FD
  File tempF = LittleFS.open(path, "r");
  if (tempF) {
    tempF.close();
    Serial.println("Temporary file handle opened+closed to help release FD");
  }

  const int maxAttempts = 8;
  bool removed = false;
  for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
    if (LittleFS.remove(path)) {
      Serial.printf("[%s] Deleted local file: %s\n", contextTag, path.c_str());
      removed = true;
      break;
    }
    Serial.printf("[%s] Attempt %d: Failed to unlink '%s' - retrying...\n", contextTag, attempt, path.c_str());
    // give Firebase/other libs a bit of time to release handles
    for (int i = 0; i < 10; ++i) {
      app.loop();
      delay(50);
    }
  }
  if (!removed) {
    Serial.printf("[%s] Final failure: could not delete %s\n", contextTag, path.c_str());
  }
  // reset tracking regardless to avoid infinite retries
  uniquePath = "";
  lastUploadedPath = "";
  printFSInfo(contextTag);
}

void processData(AsyncResult &aResult) {
  if (!aResult.isResult()) return;

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
    // Completed upload
    if (aResult.uploadInfo().total == aResult.uploadInfo().uploaded) {
      Firebase.printf("Upload task: %s, complete!✅️\n", aResult.uid().c_str());
      Serial.print("Download URL: ");
      Serial.println(aResult.uploadInfo().downloadUrl);
      // If this upload corresponds to our media upload, delete the local file
      if (lastUploadedPath.length() > 0 && String(aResult.uid()).indexOf("uploadTask") >= 0) {
        deleteLocalUploadedFile(lastUploadedPath, "PostUpload");
      }
    }
  }

  // If the upload task failed, also delete the local file to avoid filling FS
  if (aResult.isError()) {
    String uid = aResult.uid();
    if (uid.indexOf("uploadTask") >= 0) {
      Serial.printf("Upload task error for uid: %s, deleting local file if present\n", uid.c_str());
      if (lastUploadedPath.length() > 0) {
        deleteLocalUploadedFile(lastUploadedPath, "PostUploadError");
      }
    }
  }
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