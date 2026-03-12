#pragma once

#include <Arduino.h>

#define LED_PIN 1
#define MOTOR1_PIN 2
#define MOTOR2_PIN 42

// Heater thresholds (Celsius)
// MOTOR2_PIN is used as heater output. Heater turns ON at or below HEATER_ON_TEMP
// and turns OFF at or above HEATER_OFF_TEMP to provide hysteresis.
#define HEATER_ON_TEMP 26.0
#define HEATER_OFF_TEMP 33.0

void controlActuators(float lux, float temp, float humi);