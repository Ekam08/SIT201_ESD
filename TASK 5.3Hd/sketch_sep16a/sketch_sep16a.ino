#include <WiFiNINA.h>
#include <WiFiSSLClient.h>

// ==== WiFi Setup ====
char ssid[] = "DESKTOP";
char pass[] = "ekamgaba";

// ==== Firebase Setup ====
const char* host = "task-hd-a14c0-default-rtdb.firebaseio.com";
String auth = "q6ekpIOkuif1C1zyJBsBdE5y40yG3SF4c9HIdGNv"; // Database secret

// ==== LED Pins ====
const int RED = 2;
const int YELLOW = 3;
const int GREEN = 4;

// WiFiSSLClient for HTTPS
WiFiSSLClient client;

// ==== Connect to WiFi ====
void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    WiFi.begin(ssid, pass);
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Failed to connect WiFi");
  }
}

// ==== Check WiFi connection ====
void checkWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    connectWiFi();
  }
}

// ==== Read LED states from Firebase ====
String readFirebase() {
  if (WiFi.status() != WL_CONNECTED) return "{}";

  if (client.connect(host, 443)) { // HTTPS
    String url = "/leds.json?auth=" + auth; // read all LEDs at once

    client.println("GET " + url + " HTTP/1.1");
    client.println("Host: " + String(host));
    client.println("Connection: close");
    client.println();

    // Skip HTTP headers
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") break;
    }

    // Read response body
    String response = client.readString();
    client.stop();

    response.trim();
    Serial.println("Firebase says: " + response);
    return response;
  } else {
    Serial.println("❌ Connection to Firebase failed!");
    return "{}";
  }
}

// ==== Control LEDs based on nested JSON ====
void controlLEDs(String json) {
  // RED LED
  if (json.indexOf("\"red\":{\"state\":1}") > 0) {
    digitalWrite(RED, HIGH);
  } else {
    digitalWrite(RED, LOW);
  }

  // YELLOW LED
  if (json.indexOf("\"yellow\":{\"state\":1}") > 0) {
    digitalWrite(YELLOW, HIGH);
  } else {
    digitalWrite(YELLOW, LOW);
  }

  // GREEN LED
  if (json.indexOf("\"green\":{\"state\":1}") > 0) {
    digitalWrite(GREEN, HIGH);
  } else {
    digitalWrite(GREEN, LOW);
  }

  // Debug print
  Serial.print("LED States => Red: "); Serial.print(digitalRead(RED));
  Serial.print(" | Yellow: "); Serial.print(digitalRead(YELLOW));
  Serial.print(" | Green: "); Serial.println(digitalRead(GREEN));
}

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(RED, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(GREEN, OUTPUT);

  connectWiFi();
}

// ==== Main Loop ====
void loop() {
  checkWiFi(); // ensure Wi-Fi is connected

  String ledStates = readFirebase();
  controlLEDs(ledStates);

  delay(5000); // poll every 5 seconds
}
