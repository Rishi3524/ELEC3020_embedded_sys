#include <Arduino.h>

// -------- Motor A pins --------
#define ENA  1    // PWM pin (Motor A enable)
#define IN1  3
#define IN2  2

// -------- Motor B pins --------
#define ENB  11  // PWM pin (Motor B enable)
#define IN3  10
#define IN4  12

// -------- Button --------
#define BUTTON 0   // Built-in button on ESP32 T-Display S3

// -------- Motor state --------
bool direction = true;   // true = forward, false = backward

// -------- PWM setup --------
const int pwmChannelA = 0;
const int pwmChannelB = 1;
const int pwmFreq = 5000;   // 5 kHz
const int pwmResolution = 8; // 8-bit (0–255)

// -------- Debounce --------
unsigned long lastPress = 0;
const unsigned long debounceDelay = 200;

void setMotors(bool forward, int speed);

void setup() {
  Serial.begin(115200);

  // Motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Button
  pinMode(BUTTON, INPUT_PULLUP);

  // Setup PWM channels
  ledcSetup(pwmChannelA, pwmFreq, pwmResolution);
  ledcAttachPin(ENA, pwmChannelA);

  ledcSetup(pwmChannelB, pwmFreq, pwmResolution);
  ledcAttachPin(ENB, pwmChannelB);



  Serial.println("Dual motor control started");
}

void loop() {
  int buttonState = digitalRead(BUTTON);

  if (buttonState == LOW && (millis() - lastPress > debounceDelay)) {
    direction = !direction; // Toggle direction

    setMotors(direction, 180); // Run at ~70% speed

    Serial.print("Motors now: ");
    Serial.println(direction ? "Forward" : "Backward");

    lastPress = millis();
  }
}

// -------- Helper function --------
void setMotors(bool forward, int speed) {
  if (forward) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  }

  // Apply PWM speed
  ledcWrite(pwmChannelA, speed); // 0–255
  ledcWrite(pwmChannelB, speed); // 0–255
}
