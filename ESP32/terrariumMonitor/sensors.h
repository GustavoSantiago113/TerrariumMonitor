// sensors.h
#ifndef SENSORS_H
#define SENSORS_H

// Only include Adafruit libraries in this header
#include <Adafruit_AHTX0.h>
#include <Adafruit_TSL2561_U.h>

// Declare your sensor objects
extern Adafruit_AHTX0 aht;
extern Adafruit_TSL2561_Unified tsl;

// Function declarations
void initSensors();
void readSensorData();

#endif