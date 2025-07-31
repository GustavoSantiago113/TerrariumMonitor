#include <Wire.h>
#include <Adafruit_AHTX0.h>

// Pin definitions
const int LDR_PIN = 5;

// AHT10 sensor object
Adafruit_AHTX0 aht;

// Timing variables
const unsigned long READ_INTERVAL = 1000; // 10 seconds
unsigned long lastReadTime = 0;

int readLDR(int pin) {
  long sum = 0;
  for (int i = 0; i < 64; i++) {
    sum += analogRead(pin);
    delayMicroseconds(100);
  }
  return sum / 64;
}

void setup() {
  Serial.begin(115200);

  // Initialize LDR pin
  pinMode(LDR_PIN, INPUT);

  // Initialize AHT10 sensor
  if (!aht.begin()) {
    Serial.println("Failed to initialize AHT10 sensor!");
    while (1);
  }
  Serial.println("AHT10 sensor initialized.");
}

void loop() {
  unsigned long currentTime = millis();

  // Check if 30 seconds have passed
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    lastReadTime = currentTime;

    // Read luminosity from LDR
    int ldrValue = readLDR(LDR_PIN);

    // Map LDR value (0-450) to percentage (0-100%)
    int ldrPercent = map(ldrValue, 0, 4095, 100, 0);
    if (ldrPercent > 100) ldrPercent = 100;
    if (ldrPercent < 0) ldrPercent = 0;
    Serial.print("Luminosity (LDR): ");
    Serial.print(ldrValue);
    Serial.println(" %");

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