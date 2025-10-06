#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

// TSL2561 sensor address
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

// AHT10 sensor object
Adafruit_AHTX0 aht;

// Timing variables
const unsigned long READ_INTERVAL = 1000; // 10 seconds
unsigned long lastReadTime = 0;

void readLuminosity() {
  sensors_event_t event;
  tsl.getEvent(&event);
  if (event.light)
  {
    Serial.print(event.light);
    Serial.println(" lux");
  }
  else
  {
    Serial.println("Sensor overload");
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize AHT10 sensor
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
  unsigned long currentTime = millis();

  // Check if 10 seconds have passed
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    lastReadTime = currentTime;

    // Read luminosity
    readLuminosity();

    // Read temperature and humidity from AHT10
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    Serial.print("Temperature: ");
    Serial.print(temp.temperature);
    Serial.println(" °C");
    Serial.print("Humidity: ");
    Serial.print(humidity.relative_humidity);
    Serial.println(" %");
  }
}