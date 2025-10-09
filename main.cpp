#include <TFT_eSPI.h>
#include <SPI.h>

// === Motor Pins (adjust if different) ===
#define ENA 2      // Motor A PWM
#define IN1 4
#define IN2 5
#define ENB 6      // Motor B PWM
#define IN3 7
#define IN4 8

// === Ultrasonic Sensor Pins ===
#define TRIG 10
#define ECHO 9

// === TFT Backlight (depends on board) ===
#define TFT_BL 38

TFT_eSPI tft = TFT_eSPI();  // TFT instance

// PWM channels for ESP32 LEDC
const int PWM_CH_A = 0;
const int PWM_CH_B = 1;

void setup() {
  Serial.begin(115200);

  // === TFT Setup ===
  tft.init();
  tft.setRotation(1);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Motor + Sensor Test");

  // === Motor Setup ===
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  ledcSetup(PWM_CH_A, 5000, 8);  // 5kHz, 8-bit resolution
  ledcSetup(PWM_CH_B, 5000, 8);
  ledcAttachPin(ENA, PWM_CH_A);
  ledcAttachPin(ENB, PWM_CH_B);

  // === Ultrasonic Setup ===
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
}

// Measure distance in cm
float getDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duration = pulseIn(ECHO, HIGH, 25000); // 25ms timeout (~4m)
  float distance = duration * 0.034 / 2;
  return distance;
}

// Set both motors
void setMotors(bool forward, int speed) {
  if (forward) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }
  ledcWrite(PWM_CH_A, speed);
  ledcWrite(PWM_CH_B, speed);
}

void loop() {
  float distance = getDistance();

  bool obstacle = (distance <= 20.0 && distance > 0);
  if (obstacle) {
    setMotors(true, 255);  // Forward full speed
  } else {
    setMotors(false, 0);   // Stop
  }

  // === Serial Output ===
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // === TFT Output ===
  tft.fillRect(0, 50, 240, 80, TFT_BLACK);
  tft.setCursor(10, 60);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.printf("Dist: %.1f cm", distance);

  tft.setCursor(10, 90);
  tft.setTextColor(obstacle ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.printf("Motors: %s", obstacle ? "FORWARD" : "STOPPED");

  delay(200);
}
