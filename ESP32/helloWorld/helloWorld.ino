#include <Arduino.h>

void setup(){
    Serial.begin(115200);
    Serial.println("Hello, World!");
}

void loop(){
    // Your main code here
    delay(1000); // Just to avoid flooding the output
    Serial.println("Still running...");
}