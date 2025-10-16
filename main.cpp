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
#define ENB  43 
#define IN3  18
#define IN4  44

// Ultrasonic pins
#define TRIG_PIN 10
#define ECHO_PIN 11

// IR sensor pin (detects white/black)
#define IR_PIN 17

// LCD power pins
#define LCD_POWER_ON 15
#define TFT_BL 38

// ---------------- PWM Setup ----------------
const int pwmChannelA = 0;
const int pwmChannelB = 1;
const int pwmFreq = 5000;  
const int pwmResolution = 8; // 0–255

// ---------------- Behaviour Constants ----------------
const int ATTACK_SPEED = 255;           
const int BACKUP_SPEED = 220;           
const int DIST_THRESHOLD_CM = 40;       
const unsigned long MAX_BLACK_DRIVE_MS = 250; // max time allowed on black
const unsigned long BACKUP_TIME_MS = 300;    // reverse duration from black limit
const int PIVOT_SPEED = 220;                  // speed for pivoting/searching

// ---------------- Variables ----------------
bool onBlack = false;
unsigned long blackStartMillis = 0;
bool backingUp = false;
unsigned long backupStartMillis = 0;
bool scanning = true;   // pivoting
bool scanningDirection = true; // true = left, false = right

// ---------------- Function Prototypes ----------------
long readUltrasonic();
bool detectWhiteLine();
void setMotors(bool forward, int speed);
void stopMotors();
void updateDisplay(const char* status, long distance);
void handleOpponentDetection(long distance);
void pivotScan();

// ------------------------------------------------------
void setup() {

  pinMode(LCD_POWER_ON, OUTPUT);
  digitalWrite(LCD_POWER_ON, HIGH); 
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);       

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

  // Button start
  pinMode(BUTTON, INPUT_PULLUP);
  tft.drawString("Press button to start", 10, 100);
  while (digitalRead(BUTTON) == HIGH) delay(100);

  Serial.println("Sumo Bot Ready — UWA ELEC3020 Team 49");
  tft.fillScreen(TFT_BLACK);
  delay(300);
}

// ------------------------------------------------------
void loop() {
  long distance = readUltrasonic();
  bool whiteLine = detectWhiteLine(); // true = white, false = black

  // Handle black zone timing to avoid red
  if (!whiteLine && !backingUp) {
    if (!onBlack) {
      onBlack = true;
      blackStartMillis = millis();
    } else if (millis() - blackStartMillis >= MAX_BLACK_DRIVE_MS) {
      backingUp = true;
      backupStartMillis = millis();
      setMotors(false, BACKUP_SPEED); 
      updateDisplay("BLACK LIMIT", distance);
    }
  } else if (whiteLine) {
    onBlack = false;
  }

  // Handle backing up after black limit
  if (backingUp) {
    if (millis() - backupStartMillis >= BACKUP_TIME_MS) {
      backingUp = false;
      stopMotors();
    }
    delay(50);
    return; // skip scanning/attacking while backing up
  }

  // Normal opponent detection
  bool opponentDetected = (distance > 0 && distance <= DIST_THRESHOLD_CM);
  if (opponentDetected) {
    scanning = false;
    setMotors(true, ATTACK_SPEED);
    updateDisplay("OPPONENT!", distance);
  } else {
    // Pivot scan when no opponent detected
    scanning = true;
  }

  if (scanning) pivotScan();

  delay(100);
}

// ------------------------------------------------------
void pivotScan() {

  // Stop any forward/backward ramping
  stopMotors();

  // Pivot in place to search for opponent
  if (scanningDirection) {
    // Left pivot
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  } else {
    // Right pivot
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  }
  ledcWrite(pwmChannelA, PIVOT_SPEED);
  ledcWrite(pwmChannelB, PIVOT_SPEED);

  // Alternate direction every 1 second for sweeping
  static unsigned long lastPivotMillis = 0;
  if (millis() - lastPivotMillis > 1000) {
    scanningDirection = !scanningDirection;
    lastPivotMillis = millis();
  }

  updateDisplay("SCANNING...", readUltrasonic());
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
  if (speed == 0) { stopMotors(); return; }

  if (forward) {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  }

  ledcWrite(pwmChannelA, speed);
  ledcWrite(pwmChannelB, speed);
}

// ------------------------------------------------------
void stopMotors() {
  ledcWrite(pwmChannelA, 0);
  ledcWrite(pwmChannelB, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
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

  // Display IR sensor value
  tft.fillRect(0, 120, 240, 30, TFT_BLACK);
  tft.setCursor(10, 120);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.printf("IR: %s", lineDetected ? "WHITE" : "BLACK");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  return lineDetected;
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
