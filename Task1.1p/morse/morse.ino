int ledPin = LED_BUILTIN;

void setup() {
  pinMode(ledPin, OUTPUT);

  
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Starting ...");
}

void loop() {
  Serial.println("Blinking letter: E");
  blinkE();
  letterPause();

  Serial.println("Blinking letter: K");
  blinkK();
  letterPause();

  Serial.println("Blinking letter: A");
  blinkA();
  letterPause();

  Serial.println("Blinking letter: M");
  blinkM();

  Serial.println("Finished EKAM.");
  delay(3000); // Pause before repeating name
}


void blinkE() {
  dot();
}


void blinkK() {
  dash();
  symbolPause();
  dot();
  symbolPause();
  dash();
}


void blinkA() {
  dot();
  symbolPause();
  dash();
}


void blinkM() {
  dash();
  symbolPause();
  dash();
}


void dot() {
  Serial.println("Dot");
  digitalWrite(ledPin, HIGH);
  delay(250);
  digitalWrite(ledPin, LOW);
  delay(250);
}

void dash() {
  Serial.println("Dash");
  digitalWrite(ledPin, HIGH);
  delay(1000);
  digitalWrite(ledPin, LOW);
  delay(250);
}

void symbolPause() {
  delay(250); 
}

void letterPause() {
  delay(1000); 
}
