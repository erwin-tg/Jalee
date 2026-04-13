#include <Wire.h>
#include <Servo.h>

// --- Adresse du pro mini ---
#define Mini_ADDR 1

// --- Servo 1 : MACHOIRE ---
#define Machoire_ouvrir 2
#define Machoire_fermer 3
Servo Servo1;

// --- Servo 2 : QUEUE ---
#define Queue_battre 4
Servo Servo2;

// --- Bande DEL ---
#define Colier_clignoter 5
const int mosfetPin = 3;

// --- received command from Arduino uno ---
volatile byte command = 0;

void setup() {
  Serial.begin(57600);
  // --- Connection Arduino Uno et Pro mini ---
  Wire.begin(Mini_ADDR);
  Wire.onReceive(receiveEvent);

  // --- MACHOIRE ---
  Servo1.attach(9);
  Servo1.write(180); //machoire ouverte

  // --- QUEUE ---
  Servo2.attach(10);
  Servo2.write(90);

  // --- Bande DEL ---
  pinMode(mosfetPin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {
    switch (command) {
    case Machoire_ouvrir : controlerMACHOIRE(180);       break;
    case Machoire_fermer : controlerMACHOIRE(45);        break;
    case Queue_battre : battreQUEUE(3);                  break;
    case Colier_clignoter : clignoterDEL(3);             break;
  }
  command = 0;
  delay(100);

}

void receiveEvent(int howMany)
{
  if (Wire.available())
  {
    command = Wire.read();
    Serial.println(command); // DEBUG
  }
}

void controlerMACHOIRE(int angle){
  Serial.println("servo commande");
  Servo1.write(angle);
  delay(100);
}

void battreQUEUE(int battement){
  for (int i=0;i<=battement;i++){
    for (int j=90;j>=45;j-=2){
      Servo2.write(j);
      delay(15);
    }
    for (int j=45;j<=135;j+=2){
      Servo2.write(j);
      delay(15);
    }
    for (int j=135;j>=90;j-=2){
      Servo2.write(j);
      delay(15);
    } 
  }
  Servo2.detach();
}

void clignoterDEL(int clignote){
  for (int i=0;i<clignote;i++){
      // Augmente luminosité
    for (int i = 0; i <= 50; i+=5) {
      analogWrite(mosfetPin, i);
      delay(10);
    }
    delay(1000);

    // Diminue luminosité
    for (int i = 50; i >= 0; i-=5) {
      analogWrite(mosfetPin, i);
      delay(10);
    }
    delay(1000);
  }
}