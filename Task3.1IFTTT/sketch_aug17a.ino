#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter(0x23); // Use 0x23 as detected

void setup() {
  Serial.begin(9600);
  Wire.begin();

  if (lightMeter.begin()) {
    Serial.println("BH1750 initialized successfully");
  } else {
    Serial.println("BH1750 not detected!");
  }
}

void loop() {
  float lux = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
  delay(1000); // Read every 1 second
}
