#pragma once

#include <Arduino.h>

#define LED_PIN 1
#define MOTOR1_PIN 2
#define MOTOR2_PIN 42

void controlActuators(float lux, float temp, float humi);