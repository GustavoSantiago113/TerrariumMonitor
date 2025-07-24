#include <Wire.h>
#include <AHT10.h>

// Pin definitions (adjust as needed)
#define LDR_PIN 34         // Analog pin for LDR (ADC1_CH6 on ESP32 S3)
#define LED_PIN 2          // Built-in LED or change to your LED pin

AHT10 aht10;

void setup() {
  Serial.begin(115200);
  // Initialize I2C for AHT10
  Wire.begin();
  if (aht10.begin() == false) {
    Serial.println("AHT10 not detected. Check wiring!");
    while (1);
  }
  Serial.println("AHT10 detected!");

  pinMode(LDR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // 1. Read temperature and humidity from AHT10
  float temperature = aht10.readTemperature();
  float humidity = aht10.readHumidity();
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  // 2. Read light from LDR
  int ldrValue = analogRead(LDR_PIN);
  Serial.print("LDR Value: ");
  Serial.println(ldrValue);

  // 3. Blink LED
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
}
