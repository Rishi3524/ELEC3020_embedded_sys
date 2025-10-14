#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// ---------------- Pin Configuration ----------------
#define BUTTON 0  

// Motor A pins
#define ENA  1   
#define IN1  3
#define IN2  2

// Motor B pins
#define ENB  11  
#define IN3  10
#define IN4  12

// Ultrasonic pins
#define TRIG_PIN 43
#define ECHO_PIN 44

// IR sensor pin (detects white line)
#define IR_PIN 18    // <-- CHANGE if your IR sensor uses a different pin

// ---------------- PWM Setup ----------------
const int pwmChannelA = 0;
const int pwmChannelB = 1;
const int pwmFreq = 5000;  
const int pwmResolution = 8; // 0–255

// ---------------- Behaviour Constants ----------------
const int ATTACK_SPEED = 255;        // speed when chasing
const int REVERSE_SPEED = 220;       // speed when escaping line
const int DIST_THRESHOLD_CM = 30;    // opponent detection threshold
const int REVERSE_TIME_MS = 500;     // reverse duration when IR detects white
const int RAMP_TIME_MS = 300;        // ramp up time for motors

// ---------------- Variables ----------------
int currentSpeed = 0;
bool ramping = false;
unsigned long rampStartMillis = 0;
unsigned long reverseStartMillis = 0;
bool reversing = false;

// ---------------- Function Prototypes ----------------
long readUltrasonic();
bool detectWhiteLine();
void setMotors(bool forward, int speed);
void stopMotors();
void updateDisplay(const char* status, long distance);
void handleOpponentDetection(long distance);
void handleLineEscape();

// ------------------------------------------------------
void setup() {
  Serial.begin(115200);

  // TFT Display setup
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("ELEC3020 Team 49", 10, 10);

  // Motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Ultrasonic setup
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // IR sensor setup
  pinMode(IR_PIN, INPUT);

  // PWM setup
  ledcSetup(pwmChannelA, pwmFreq, pwmResolution);
  ledcAttachPin(ENA, pwmChannelA);
  ledcSetup(pwmChannelB, pwmFreq, pwmResolution);
  ledcAttachPin(ENB, pwmChannelB);

  stopMotors();

    // Button (unused but kept for compatibility)
  pinMode(BUTTON, INPUT_PULLUP);

  tft.drawString("Press button to start", 10, 100);
  while (digitalRead(BUTTON) == HIGH) {
    delay(100);
  }

  Serial.println("Sumo Bot Ready — UWA ELEC3020 Team 49");
  delay(300);
}

// ------------------------------------------------------
void loop() {
  long distance = readUltrasonic();
  bool whiteLine = detectWhiteLine();

  if (whiteLine && !reversing) {
    // Start reverse sequence
    reversing = true;
    reverseStartMillis = millis();
    setMotors(false, REVERSE_SPEED);
    updateDisplay("ESCAPING LINE", distance);
    Serial.println("White line detected! Reversing...");
  }

  if (reversing) {
    if (millis() - reverseStartMillis >= REVERSE_TIME_MS) {
      reversing = false;
      stopMotors();
    }
    delay(50);
    return;
  }

  // Normal opponent detection
  handleOpponentDetection(distance);
  delay(100);
}

// ------------------------------------------------------
void handleOpponentDetection(long distance) {
  bool detected = (distance > 0 && distance <= DIST_THRESHOLD_CM);

  if (detected) {
    setMotors(true, ATTACK_SPEED);
    updateDisplay("OPPONENT!", distance);
  } else {
    stopMotors();
    updateDisplay("SEARCHING...", distance);
  }
}

// ------------------------------------------------------
void setMotors(bool forward, int speed) {
  speed = constrain(speed, 0, 255);

  if (speed == 0) {
    stopMotors();
    return;
  }

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

  ledcWrite(pwmChannelA, speed);
  ledcWrite(pwmChannelB, speed);
  currentSpeed = speed;
}

// ------------------------------------------------------
void stopMotors() {
  ledcWrite(pwmChannelA, 0);
  ledcWrite(pwmChannelB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  currentSpeed = 0;
}

// ------------------------------------------------------
long readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;
  return (duration * 0.034 / 2);
}

// ------------------------------------------------------
bool detectWhiteLine() {
  bool lineDetected = digitalRead(IR_PIN); // HIGH = white, LOW = black

  // Clear old reading area
  tft.fillRect(0, 120, 240, 30, TFT_BLACK);

  // Display sensor state
  tft.setCursor(10, 120);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.printf("IR: %s", lineDetected ? "WHITE" : "BLACK");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  return lineDetected; // true = white line detected
}


// ------------------------------------------------------
void updateDisplay(const char* status, long distance) {
  tft.fillRect(0, 40, 240, 80, TFT_BLACK);
  tft.setCursor(10, 40);
  if (distance > 0)
    tft.printf("Dist: %ld cm", distance);
  else
    tft.print("Dist: --- cm");

  tft.setCursor(10, 80);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.printf("Status: %s", status);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
}
