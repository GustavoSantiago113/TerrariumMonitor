#include "sensors.h"
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>

TwoWire sensorsWire = TwoWire(0);

Adafruit_AHTX0 aht;
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

void initSensors() {
  // Initialize dedicated I2C bus for sensors
  sensorsWire.begin(SENSORS_SDA_PIN, SENSORS_SCL_PIN, 100000);
  delay(50);
  
  Serial.println("Initializing sensors on dedicated I2C bus...");
  Serial.print("Using SDA=");
  Serial.print(SENSORS_SDA_PIN);
  Serial.print(", SCL=");
  Serial.println(SENSORS_SCL_PIN);
  
  if (!aht.begin(&sensorsWire)) { 
    Serial.println("AHT init failed - check wiring on pins"); 
    while (1) delay(1000);
  }
  
  if (!tsl.begin(&sensorsWire)) { 
    Serial.println("TSL init failed - check wiring on pins"); 
    while (1) delay(1000);
  }
  
  tsl.setGain(TSL2561_GAIN_1X);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);
  
  Serial.println("✓ Sensors initialized successfully on I2C bus 1 (pins 21/47)");
}

void readSensors(float &lux, float &temp, float &humi) {
  sensors_event_t ev;
  tsl.getEvent(&ev);
  lux = ev.light;
  
  sensors_event_t h, t;
  aht.getEvent(&h, &t);
  temp = t.temperature;
  humi = h.relative_humidity;
}