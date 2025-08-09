#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

// TSL2561 sensor address
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

// Network and Firebase credentials
#define WIFI_SSID "*******************"
#define WIFI_PASSWORD "************"

#define Web_API_KEY "****************"
#define DATABASE_URL "***************"
#define USER_EMAIL "****************"
#define USER_PASS "*********************"

Adafruit_AHTX0 aht;

// User function
void processData(AsyncResult &aResult);

// Authentication
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

// Firebase components
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);
RealtimeDatabase Database;

// Timer variables for sending data every 10 seconds
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000; // 10 seconds in milliseconds

// Add these global variables at the top:
float lastLux = 0;
float lastTemp = 0;
float lastHumi = 0;

void setup(){
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  
  // Configure SSL client
  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);
  
  // Initialize Firebase
  initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);

  if (!aht.begin()) {
    Serial.println("Failed to initialize AHT10 sensor!");
    while (1);
  }
  if (!tsl.begin()) {
    Serial.println("Failed to initialize TSL sensor!");
    while (1);
  }
  Serial.println("Sensors AHT10 and TSL initialized.");

  tsl.setGain(TSL2561_GAIN_1X);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);
}

void loop() {
  app.loop();

  // In loop()
  if (app.ready()) {
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= sendInterval) {
      lastSendTime = currentTime;

      // Read luminosity
      sensors_event_t event;
      tsl.getEvent(&event);
      lastLux = event.light;

      // Read temperature and humidity
      sensors_event_t humidity, temp;
      aht.getEvent(&humidity, &temp);
      lastTemp = temp.temperature;
      lastHumi = humidity.relative_humidity;
      unsigned long now = millis();
      String key = String(now);
      Database.set<float>(aClient, "/test/lux/" + key, lastLux, processData, "RTDB_Send_LUX");
      Database.set<float>(aClient, "/test/temp/" + key, lastTemp, processData, "RTDB_Send_TEMP");
      Database.set<float>(aClient, "/test/humi/" + key, lastHumi, processData, "RTDB_Send_HUMI");
    }
  }

}

void processData(AsyncResult &aResult) {
  if (!aResult.isResult())
    return;

  if (aResult.isEvent())
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());

  if (aResult.isDebug())
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());

  if (aResult.isError())
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());

  if (aResult.available()) {
    Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());

    // Check which task finished
    if (aResult.uid() == "RTDB_Push_LUX") {
      String key = aResult.c_str();
      Database.set<float>(aClient, "/test/lux/" + key, lastLux, processData, "RTDB_Send_LUX");
    }
    // Add similar blocks for temp and humi
    else if (aResult.uid() == "RTDB_Push_TEMP") {
      String key = aResult.c_str();
      Database.set<float>(aClient, "/test/temp/" + key, lastTemp, processData, "RTDB_Send_TEMP");
    }
    else if (aResult.uid() == "RTDB_Push_HUMI") {
      String key = aResult.c_str();
      Database.set<float>(aClient, "/test/humi/" + key, lastHumi, processData, "RTDB_Send_HUMI");
    }
  }
}