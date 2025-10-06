#pragma once
#include <Arduino.h>
#include <Wire.h>

#define SENSORS_SDA_PIN 21
#define SENSORS_SCL_PIN 47

class Adafruit_AHTX0;
class Adafruit_TSL2561_Unified;

extern TwoWire sensorsWire;
extern Adafruit_AHTX0 aht;
extern Adafruit_TSL2561_Unified tsl;

void initSensors();
void readSensors(float &lux, float &temp, float &humi);