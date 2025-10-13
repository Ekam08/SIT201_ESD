#include <ArduinoBLE.h>  
#include <Wire.h> 
#include <BH1750.h>  

BH1750 lightMeter;  


BLEService lightService("181A");  
BLEFloatCharacteristic lightCharacteristic("2A6E", BLERead | BLENotify); 
    

void setup() {
  Serial.begin(9600);  
  while(!Serial);  
  
  // Initialize sensor
  Wire.begin();  
  
  if (!lightMeter.begin()) {
    Serial.println("BH1750 failed!");
    while(1);
  }
  Serial.println("BH1750 OK");

  
  if (!BLE.begin()) {
    Serial.println("BLE failed!");
    while(1);
  }

  BLE.setDeviceName("ArduinoLight");
  BLE.setLocalName("ArduinoLight");
  
  
  BLE.setAdvertisingInterval(320);  
  BLE.setConnectable(true); 
  
  BLE.setAdvertisedService(lightService);  
  lightService.addCharacteristic(lightCharacteristic);  
  BLE.addService(lightService);  
  
  
  BLE.advertise();
  
  Serial.print("BLE MAC: ");
  Serial.println(BLE.address());
  Serial.println("Advertising as 'ArduinoLight'");
  Serial.println("Ready for connections...");
}

void loop() {
  
  BLEDevice central = BLE.central();  
  
  if (central) {
    Serial.print("Connected to: ");
    Serial.println(central.address());  
    
    
    while (central.connected()) {
      float lux = lightMeter.readLightLevel(); 
      
      Serial.print("Light: ");
      Serial.print(lux);
      Serial.println(" lx");
      
      lightCharacteristic.writeValue(lux); 
      delay(1000);  
    }
    
    
    Serial.print("Disconnected from: ");
    Serial.println(central.address());
  }
  
  delay(1000);
}