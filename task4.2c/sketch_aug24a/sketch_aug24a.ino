
const int buttonPin = 2;    
const int pirPin = 3;       
const int led1Pin = 10;     
const int led2Pin = 11;     

volatile bool led1State = LOW;
volatile bool led2State = LOW;

void setup() {
  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(pirPin, INPUT);           
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);

  Serial.begin(9600);

  attachInterrupt(digitalPinToInterrupt(buttonPin), toggleLED1, FALLING); 
  attachInterrupt(digitalPinToInterrupt(pirPin), toggleLED2, RISING);  
}

void loop() {
  digitalWrite(led1Pin, led1State);
  digitalWrite(led2Pin, led2State);
}

void toggleLED1() {
  led1State = !led1State;  
  Serial.println("Button pressed → LED1 toggled");
}

void toggleLED2() {
  led2State = !led2State;  
  Serial.println("Motion detected → LED2 toggled");
}
