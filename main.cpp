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

// IR sensor pin
#define IR_PIN 18    

// Opponent indicator LED
#define LED_PIN 17    

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
const int RAMP_TIME_MS = 300;        // (unused but kept)
const int SEARCH_SPIN_SPEED = 180;   // speed while rotating to search
const int RECOVER_TIME_MS = 600;     // time for recovery turn
const int NO_DETECT_TIMEOUT_MS = 2500; // timeout before spinning search

// ---------------- Variables ----------------
int currentSpeed = 0;
unsigned long lastDetectTime = 0;
unsigned long stateStartTime = 0;

// ---------------- State Machine ----------------
enum BotState { SEARCHING, ATTACKING, ESCAPING, RECOVERING };
BotState state = SEARCHING;

// ---------------- Function Prototypes ----------------
long readUltrasonic();
bool detectWhiteLine();
void setMotors(bool forward, int speed);
void stopMotors();
void updateDisplay(const char* status, long distance);

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

  // LED setup
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // PWM setup
  ledcSetup(pwmChannelA, pwmFreq, pwmResolution);
  ledcAttachPin(ENA, pwmChannelA);
  ledcSetup(pwmChannelB, pwmFreq, pwmResolution);
  ledcAttachPin(ENB, pwmChannelB);

  stopMotors();

  // Button
  pinMode(BUTTON, INPUT_PULLUP);
  tft.drawString("Press button to start", 10, 100);

  while (digitalRead(BUTTON) == HIGH) {
    delay(100);
  }

  Serial.println("Sumo Bot Ready — Battle Mode");
  tft.fillScreen(TFT_BLACK);
  delay(300);
}

// ------------------------------------------------------
void loop() {
  long distance = readUltrasonic();
  bool whiteLine = detectWhiteLine();
  unsigned long now = millis();

  // 1️. ESCAPE LINE 
  if (whiteLine && state != ESCAPING) {
    state = ESCAPING;
    stateStartTime = now;
    setMotors(false, REVERSE_SPEED);
    updateDisplay("ESCAPING LINE", distance);
    digitalWrite(LED_PIN, LOW);
    Serial.println("White line detected! Reversing...");
    return;
  }

  if (state == ESCAPING) {
    if (now - stateStartTime >= REVERSE_TIME_MS) {
      state = RECOVERING;
      stateStartTime = now;
      // Turn in place after reverse
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      ledcWrite(pwmChannelA, SEARCH_SPIN_SPEED);
      ledcWrite(pwmChannelB, SEARCH_SPIN_SPEED);
      updateDisplay("RECOVERING...", distance);
    }
    return;
  }

  // 2️. RECOVERY TURN
  if (state == RECOVERING) {
    if (now - stateStartTime >= RECOVER_TIME_MS) {
      state = SEARCHING;
      stopMotors();
      updateDisplay("SEARCHING...", distance);
    }
    return;
  }

  // 3️. ATTACK & SEARCH LOGIC
  bool opponentDetected = (distance > 0 && distance <= DIST_THRESHOLD_CM);

  if (opponentDetected) {
    lastDetectTime = now;
    state = ATTACKING;

    // Adaptive attack speed (faster when closer)
    int speed = map(distance, 5, DIST_THRESHOLD_CM, 255, 150);
    speed = constrain(speed, 150, 255);

    setMotors(true, speed);
    digitalWrite(LED_PIN, HIGH);
    updateDisplay("ATTACKING!", distance);
  } else {
    digitalWrite(LED_PIN, LOW);

    if (now - lastDetectTime > NO_DETECT_TIMEOUT_MS) {
      // Spin to search
      state = SEARCHING;
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      ledcWrite(pwmChannelA, SEARCH_SPIN_SPEED);
      ledcWrite(pwmChannelB, SEARCH_SPIN_SPEED);
      updateDisplay("SPIN SEARCH...", distance);
    } else {
      stopMotors();
      updateDisplay("SEARCHING...", distance);
    }
  }

  delay(100);
}

// ------------------------------------------------------
void setMotors(bool forward, int speed) {
  speed = constrain(speed, 0, 255);
  if (speed == 0) { stopMotors(); return; }

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
