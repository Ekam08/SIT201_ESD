#include <WiFiNINA.h>
#include <PubSubClient.h>

// ===== WiFi credentials =====
char ssid[] = "DESKTOP";
char password[] = "ekamgaba";

// ===== MQTT Broker Settings =====
const char* mqttServer = "broker.emqx.io";
const int mqttPort = 1883;
const char* mqttClientID = "arduinoClient";
const char* mqttTopicWave = "SIT210/wave";
const char* mqttTopicPat  = "SIT210/pat";
const char* yourName = "ekam";

// ===== Hardware Pins =====
const int trigPin = 2;
const int echoPin = 3;
const int ledPin = 6;

// ===== Ultrasonic Sensor Variables =====
long duration;
int distance;
int waveThreshold = 20; // cm threshold for wave
int patThreshold  = 10; // cm threshold for pat

// ===== WiFi and MQTT Clients =====
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ===== Function: Measure distance =====
int getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2; // convert to cm
  return distance;
}

// ===== Function: Handle incoming MQTT messages =====
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // Handle "wave"
  if (String(topic) == mqttTopicWave && message.indexOf("wave") >= 0) {
    for (int i = 0; i < 3; i++) { // fast blink 3 times
      digitalWrite(ledPin, HIGH);
      delay(200);
      digitalWrite(ledPin, LOW);
      delay(200);
    }
  }

  // Handle "pat"
  else if (String(topic) == mqttTopicPat && message.indexOf("pat") >= 0) {
    for (int i = 0; i < 5; i++) { // slow blink 2 times
      digitalWrite(ledPin, HIGH);
      delay(600);
      digitalWrite(ledPin, LOW);
      delay(600);
    }
  }
}

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(mqttClientID)) {
      Serial.println("connected");
      mqttClient.subscribe(mqttTopicWave);
      mqttClient.subscribe(mqttTopicPat);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// ===== Arduino Setup =====
void setup() {
  Serial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);
}

// ===== Arduino Loop =====
void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  int currentDistance = getDistance();
  Serial.print("Distance: ");
  Serial.print(currentDistance);
  Serial.println(" cm");

  // Detect wave
  if (currentDistance > patThreshold && currentDistance < waveThreshold) {
    String message = "wave from ";
    message += yourName;
    mqttClient.publish(mqttTopicWave, message.c_str());
    Serial.println("Published a 'wave' message!");
    delay(5000); // debounce
  }

  // Detect pat
  else if (currentDistance > 0 && currentDistance <= patThreshold) {
    String message = "pat from ";
    message += yourName;
    mqttClient.publish(mqttTopicPat, message.c_str());
    Serial.println("Published a 'pat' message!");
    delay(5000); // debounce
  }

  delay(500);
}