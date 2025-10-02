#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

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

// Motor state
bool direction = true;   // true = forward, false = backward

// PWM setup
const int pwmChannelA = 0;
const int pwmChannelB = 1;
const int pwmFreq = 5000;  
const int pwmResolution = 8; 

// Debounce
unsigned long lastPress = 0;
const unsigned long debounceDelay = 200;

// Function prototypes
void setMotors(bool forward, int speed);
long readUltrasonic();

void setup() {
  Serial.begin(115200);

  // Init TFT
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

  // Button
  pinMode(BUTTON, INPUT_PULLUP);

  // Ultrasonic pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Setup PWM channels
  ledcSetup(pwmChannelA, pwmFreq, pwmResolution);
  ledcAttachPin(ENA, pwmChannelA);

  ledcSetup(pwmChannelB, pwmFreq, pwmResolution);
  ledcAttachPin(ENB, pwmChannelB);

  setMotors(true, 180);

  Serial.println("Dual motor control + Ultrasonic started");
}

void loop() {
  int buttonState = digitalRead(BUTTON);

  if (buttonState == LOW && (millis() - lastPress > debounceDelay)) {
    direction = !direction; // Toggle direction

    setMotors(direction, 170); 

    Serial.print("Motors now: ");
    Serial.println(direction ? "Forward" : "Backward");

    lastPress = millis();
  }

  long distance = readUltrasonic();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // --- TFT output ---
  tft.fillRect(0, 40, 240, 60, TFT_BLACK); // Clear old distance
  tft.setCursor(10, 40);
  tft.printf("Dist: %ld cm", distance);

  tft.fillRect(0, 100, 240, 40, TFT_BLACK); // Clear old direction
  tft.setCursor(10, 100);
  tft.printf("Motors: %s", direction ? "Forward" : "Backward");
  


  delay(200);
}

// Motor control
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
  ledcWrite(pwmChannelA, speed); 
  ledcWrite(pwmChannelB, speed);
}

// Ultrasonic sensor
long readUltrasonic() {
  // Trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure echo pulse length
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); 

  // Convert to cm
  long distance = duration * 0.034 / 2;
  return distance;
}
