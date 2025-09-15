#include <Arduino.h>
#include <DHT.h>
#include <RTCZero.h>

// ===== Pins =====
const uint8_t BUTTON_PIN = 2;
const uint8_t TRIG_PIN   = 6;
const uint8_t ECHO_PIN   = 7;
const uint8_t DHT_PIN    = 5;
const uint8_t LED1_PIN   = 12;
const uint8_t LED2_PIN   = 10;
const uint8_t LED3_PIN   = 11;

#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE);

const uint16_t PROXIMITY_THRESHOLD_CM = 20;

// ===== Globals =====
volatile bool buttonPressed = false;
unsigned long lastButtonMillis = 0;
const unsigned long DEBOUNCE_MS = 50;

volatile bool led3State = false;
unsigned long lastUltrasonic = 0;
const unsigned long ULTRASONIC_INTERVAL = 1000; // ms

RTCZero rtc;

// ===== ISRs =====
void buttonISR() {
  buttonPressed = true;
}

void TimerHandler0() {
  led3State = !led3State;
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);

  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);

  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  dht.begin();

  // ===== Conditional RTC / SAMD check =====
#ifdef SAMD
  // --- SAMD/Nano 33 IoT RTCZero timer interrupt ---
  rtc.begin();
  rtc.setTime(0,0,0);
  rtc.setAlarmTime(0,0,1);
  rtc.enableAlarm(rtc.MATCH_SS);
  rtc.attachInterrupt(TimerHandler0);
  Serial.println("SAMD RTCZero initialized");
#else
  // --- Fallback generic RTC / Arduino code ---
  Serial.println("Generic RTC / fallback code running");
#endif
}

// ===== Loop =====
void loop() {
  unsigned long now = millis();

  // --- Button interrupt with debounce ---
  if (buttonPressed && (now - lastButtonMillis > DEBOUNCE_MS)) {
    lastButtonMillis = now;
    buttonPressed = false;
    digitalWrite(LED1_PIN, !digitalRead(LED1_PIN));
    Serial.println("Button pressed → LED1 toggled");
  }

  // --- Ultrasonic + DHT read ---
  if (now - lastUltrasonic >= ULTRASONIC_INTERVAL) {
    lastUltrasonic = now;

    // --- Ultrasonic ---
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);
    if (duration > 0) {
      unsigned int distance_cm = duration / 58;
      Serial.print("Ultrasonic distance: ");
      Serial.print(distance_cm);
      Serial.println(" cm");

      if (distance_cm > 0 && distance_cm <= PROXIMITY_THRESHOLD_CM) {
        digitalWrite(LED2_PIN, HIGH);
        Serial.println("Proximity detected → LED2 ON");
      } else {
        digitalWrite(LED2_PIN, LOW);
      }
    } else {
      Serial.println("Ultrasonic read timeout");
    }

    // --- DHT22 ---
    float hum = dht.readHumidity();
    float temp = dht.readTemperature();
    if (isnan(hum) || isnan(temp)) {
      Serial.println("DHT read FAILED");
    } else {
      Serial.print("DHT22 → Temp: ");
      Serial.print(temp, 1);
      Serial.print(" C, Humidity: ");
      Serial.print(hum, 1);
      Serial.println(" %");
    }

#ifdef SAMD
    // --- LED3 toggle via RTCZero ---
    digitalWrite(LED3_PIN, led3State ? HIGH : LOW);
#else
    // --- Fallback LED3 toggle every loop ---
    digitalWrite(LED3_PIN, !digitalRead(LED3_PIN));
#endif

    Serial.println("---------------------------");
  }

  delay(5);
}
