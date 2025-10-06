// Pin definitions
#define LED_PIN 1
#define MOTOR1_PIN 2
#define MOTOR2_PIN 42

// Timing variables
const unsigned long MOTOR_ON_TIME = 30000; // 30 seconds
const unsigned long MOTOR_OFF_TIME = 30000; // 30 seconds
const unsigned long LED_ON_TIME = 5000;    // 5 seconds

unsigned long previousMillis = 0;
bool motorsOn = false;

void setup() {

  Serial.begin(115200);
  // Initialize pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR1_PIN, OUTPUT);
  pinMode(MOTOR2_PIN, OUTPUT);

  // Start with everything off
  digitalWrite(LED_PIN, LOW);
  digitalWrite(MOTOR1_PIN, LOW);
  digitalWrite(MOTOR2_PIN, LOW);
}

void loop() {
  unsigned long currentMillis = millis();

  if (motorsOn) {
    Serial.println("Motors on");
    // Turn LEDs on for 5 seconds during the motor operation
    if (currentMillis - previousMillis < LED_ON_TIME) {
      digitalWrite(LED_PIN, HIGH);
    } else {
      digitalWrite(LED_PIN, LOW);
    }

    // Turn motors off after 30 seconds
    if (currentMillis - previousMillis >= MOTOR_ON_TIME) {
      digitalWrite(MOTOR1_PIN, LOW);
      digitalWrite(MOTOR2_PIN, LOW);
      motorsOn = false;
      previousMillis = currentMillis; // Reset the timer for the off period
    }
  } else {
    Serial.println("Motors off");
    // Turn motors off for 30 seconds
    if (currentMillis - previousMillis >= MOTOR_OFF_TIME) {
      digitalWrite(MOTOR1_PIN, HIGH);
      digitalWrite(MOTOR2_PIN, HIGH);
      motorsOn = true;
      previousMillis = currentMillis; // Reset the timer for the on period
    }
  }
}