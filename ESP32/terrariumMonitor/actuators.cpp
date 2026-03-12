#include <Arduino.h>
#include "actuators.h"

void controlActuators(float lux, float temp, float humi) {
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    int hour = timeinfo.tm_hour;

    // LED logic: If luminosity < 50 and between 7am and 7pm, turn LEDs on
    if (lux < 25 && hour >= 7 && hour < 19) {
        digitalWrite(LED_PIN, HIGH);
    } else {
        digitalWrite(LED_PIN, LOW);
    }

    // Motor1: exhaust fan control (unchanged behavior)
    if (temp > 33 || humi > 75) {
        digitalWrite(MOTOR1_PIN, HIGH);
    } else {
        digitalWrite(MOTOR1_PIN, LOW);
    }

    // Motor2: heater control with hysteresis
    static bool heaterOn = false;
    if (!heaterOn && temp <= HEATER_ON_TEMP) {
        heaterOn = true;
        digitalWrite(MOTOR2_PIN, HIGH);
        Serial.println("Heater turned ON (MOTOR2_PIN)");
    } else if (heaterOn && temp >= HEATER_OFF_TEMP) {
        heaterOn = false;
        digitalWrite(MOTOR2_PIN, LOW);
        Serial.println("Heater turned OFF (MOTOR2_PIN)");
    }
}