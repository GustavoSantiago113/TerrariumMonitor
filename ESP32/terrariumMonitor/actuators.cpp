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

    // Motor logic: If temp > 33 or humidity > 75, turn both motors on
    if (temp > 33 || humi > 75) {
        digitalWrite(MOTOR1_PIN, HIGH);
        digitalWrite(MOTOR2_PIN, HIGH);
    } else {
        digitalWrite(MOTOR1_PIN, LOW);
        digitalWrite(MOTOR2_PIN, LOW);
    }
}