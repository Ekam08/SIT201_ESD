#include "BluetoothSerial.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ====== WiFi + MQTT Config ======
const char* ssid = "DESKTOP";
const char* password = "ekamgaba";
const char* mqtt_server = "192.168.137.167";   // Raspberry Pi IP

WiFiClient espClient;
PubSubClient client(espClient);

// ====== Hardware Setup ======
#define MR1 12
#define MR2 14
#define ML1 27
#define ML2 26
#define TRIG_PIN 4
#define ECHO_PIN 5
#define SS_PIN 25
#define RST_PIN 33

MFRC522 rfid(SS_PIN, RST_PIN);
BluetoothSerial SerialBT;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ====== Function Prototypes ======
void setup_wifi();
void reconnectMQTT();
void Forward();
void Backward();
void Left();
void Right();
void Stop();
long getDistance();
void handleBluetooth(char cmd);
void bluetoothTask(void *parameter);
void rfidTask(void *parameter);

// ====== SETUP ======
void setup() {
  delay(3000);
  Serial.begin(115200);
  SerialBT.begin("ESP32Car");
  lcd.init();
  lcd.backlight();

  pinMode(MR1, OUTPUT); pinMode(MR2, OUTPUT);
  pinMode(ML1, OUTPUT); pinMode(ML2, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);

  lcd.clear(); lcd.print("Booting...");
  Serial.println("ESP32 Car Booting...");

  SPI.end();
  SPI.begin(18, 19, 23, 25);
  rfid.PCD_Init();

  setup_wifi();
  client.setServer(mqtt_server, 1883);

  lcd.clear(); lcd.print("System Ready");
  delay(1000);
  lcd.clear();

  // 🔹 Run Bluetooth task on Core 1
  xTaskCreatePinnedToCore(
    bluetoothTask,
    "bluetoothTask",
    4096,
    NULL,
    1,
    NULL,
    1
  );

  // 🔹 Run RFID task on Core 0 (higher priority)
  xTaskCreatePinnedToCore(
    rfidTask,
    "rfidTask",
    4096,
    NULL,
    2,
    NULL,
    0
  );
}

// ====== LOOP (Mainly MQTT + Ultrasonic) ======
void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  long distance = getDistance();
  if (distance > 0 && distance < 20) {
    Stop();
    lcd.setCursor(0, 1);
    lcd.print("Obstacle! ");
    lcd.print(distance);
    lcd.print("cm");
    client.publish("esp32/cart/alert", "Obstacle Detected!");
    delay(500);
  }

  delay(10);
}

// ====== BLUETOOTH TASK ======
void bluetoothTask(void *parameter) {
  while (true) {
    if (SerialBT.available()) {
      char cmd = SerialBT.read();
      handleBluetooth(cmd);
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ====== RFID TASK (Independent Parallel Thread) ======
void rfidTask(void *parameter) {
  unsigned long lastSPIReset = millis();
  while (true) {
    // RFID read check
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String uid = "";
      for (byte i = 0; i < rfid.uid.size; i++) {
        uid += String(rfid.uid.uidByte[i], HEX);
      }
      uid.toUpperCase();

      // Show and send data
      Serial.println("RFID UID: " + uid);
      lcd.setCursor(0, 0);
      lcd.print("RFID: "); 
      lcd.print(uid);
      String message = "RFID:" + uid;
      client.publish("esp32/cart/rfid", message.c_str());
      SerialBT.println(message);

      rfid.PICC_HaltA();
      delay(500);
    }

    // 🔄 Prevent SPI hang
    if (millis() - lastSPIReset > 3000) {
      rfid.PCD_Init();
      lastSPIReset = millis();
    }

    vTaskDelay(400 / portTICK_PERIOD_MS);  // check every 400ms
  }
}

// ====== Handle Bluetooth Commands ======
void handleBluetooth(char cmd) {
  switch (cmd) {
    case 'F': Forward(); break;
    case 'B': Backward(); break;
    case 'L': Left(); break;
    case 'R': Right(); break;
    case 'S': Stop(); break;
    default: break;
  }
}

// ====== MOVEMENT FUNCTIONS ======
void Forward() {
  digitalWrite(MR1, HIGH); digitalWrite(MR2, LOW);
  digitalWrite(ML1, HIGH); digitalWrite(ML2, LOW);
  lcd.setCursor(0, 1); lcd.print("Forward   ");
}

void Backward() {
  digitalWrite(MR1, LOW); digitalWrite(MR2, HIGH);
  digitalWrite(ML1, LOW); digitalWrite(ML2, HIGH);
  lcd.setCursor(0, 1); lcd.print("Backward  ");
}

void Left() {
  digitalWrite(MR1, HIGH); digitalWrite(MR2, LOW);
  digitalWrite(ML1, LOW); digitalWrite(ML2, LOW);
  lcd.setCursor(0, 1); lcd.print("Left      ");
}

void Right() {
  digitalWrite(MR1, LOW); digitalWrite(MR2, LOW);
  digitalWrite(ML1, HIGH); digitalWrite(ML2, LOW);
  lcd.setCursor(0, 1); lcd.print("Right     ");
}

void Stop() {
  digitalWrite(MR1, LOW); digitalWrite(MR2, LOW);
  digitalWrite(ML1, LOW); digitalWrite(ML2, LOW);
  lcd.setCursor(0, 1); lcd.print("Stopped   ");
}

// ====== ULTRASONIC SENSOR ======
long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  long distance = duration * 0.034 / 2;
  return distance;
}

// ====== WIFI + MQTT ======
void setup_wifi() {
  Serial.println("Connecting WiFi...");
  WiFi.begin(ssid, password);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n WiFi Connected!");
    Serial.print("IP: "); Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n WiFi Failed!");
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println(" MQTT Connected!");
      client.publish("esp32/status", "ESP32 Connected");
    } else {
      Serial.print("Failed, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}
