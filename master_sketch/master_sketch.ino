#include <PinChangeInterrupt.h>
#include <Wire.h>
#include <Servo.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- ENCODEURS ----------
const byte encLA = 11;
const byte encLB = 12;
const byte encRA = 2;
const byte encRB = 10;

volatile long countL = 0;
volatile long countR = 0;

byte lastLA;
byte lastRA;

// ---------- MOTEURS ----------
const byte PWMA = 3;
const byte AIN1 = 5;
const byte AIN2 = 4;

const byte PWMB = 9;
const byte BIN1 = 7;
const byte BIN2 = 8;

// ---------- CONTROLE ----------
int basePWM = 190;
int PWM_correction = 5;
float Kp = 2.0;
int direction = 1;
int STATUS = "go";

// distance cible en pulses
const float diametre_roue = 60; //mm
const float distance_roues = 132; //mm
volatile float rotation;
volatile float deplacement;
volatile float revolution;
volatile int targetDist;
volatile int targetTurn;

// ------ Arduino Pro mini ------
#define Mini_ADDR 1
#define Machoire_ouvrir 2
#define Machoire_fermer 3
#define Queue_battre 4
#define Colier_clignoter 5

// ---- Joystick ----
const int xPin = A2;
const int buttonPin = A3;
int PWMDrive;
int PWMRot;
int PWMPos;
int speedMode = 2;

// ---- Écran LCD ----
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define ADDR_display1 0x3C

Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup()
{
  Serial.begin(57600);
  Wire.begin();

  pinMode(encLA, INPUT_PULLUP);
  pinMode(encLB, INPUT_PULLUP);
  pinMode(encRA, INPUT_PULLUP);
  pinMode(encRB, INPUT_PULLUP);

  lastLA = digitalRead(encLA);
  lastRA = digitalRead(encRA);

  attachPCINT(digitalPinToPCINT(encLA), isrLeft, CHANGE);
  attachPCINT(digitalPinToPCINT(encRA), isrRight, CHANGE);

  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);

  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  forward();

  // LCD
  display1.begin(SSD1306_SWITCHCAPVCC, ADDR_display1);
  display1.clearDisplay();
  clignerYeux(1);

  // Joystick
  pinMode(xPin, INPUT);
  pinMode(buttonPin,INPUT_PULLUP);
  
  // Choix de la vitesse
  PWMDrive = controllerPWM();  // vitesse entre cercles
  PWMRot = 100;                // vitesse pour rotation
  PWMPos = 100;                // vitesse pour positionnement
  clignerYeux(1);
  delay(1000);

  // Prendre le Jouet
  prendreJouet();
  clignerYeux(1);
  delay(1000);

  // COMMENCER
  commencer();
  clignerYeux(1);
  delay(1000);
}

void loop()
{
  Serial.println("go");
  if (STATUS == "go"){
  // Se deplacer vers le cercle 1  
    driveForward(644.4, PWMDrive);  // avancer de 644,4 mm = 3200 pulse
    delay(500);

  //debut deposer poulet
    rotate(45, PWMRot);         // tourner ~45° = 250 pulse
    delay(500);

    driveForward(250, PWMPos);  // avancer 250 mm = 820 pulse
    delay(500);

    call(Machoire_ouvrir);
    delay(2000);

    call(Machoire_fermer);
    delay(500);

    driveForward(-250, PWMPos);  // reculer 250 mm = -850 pulse
    delay(500);

    rotate(90, PWMRot);    // tourner 90° = 551 pulse
    delay(1500);

  //fin deposer poulet

  // Clignoter le colier
    call(Colier_clignoter);
    delay(7000);

  // Se deplacer vers le cercle 2
    rotate(-90, PWMRot);    // tourner -90° = -220 pulse  
    delay(500);
    
    driveForward(1288.8, PWMDrive);  // avancer de 1288,8 mm = 6300 pulse
    delay(500);

    rotate(135, PWMRot);         // tourner ~135° = 825 pulse
    delay(500);

    call(Queue_battre);
    delay(20000);            // pause de 20s avant parcours bonus 

  // Parcour bonus
    rotate(-45, PWMRot);       // tourner -45° = -250 pulse
    delay(500);

  // Se deplacer vers le cercle 3
    driveForward(644.4, PWMDrive);  // avancer de 644,4 mm = 3250 pulse
    delay(500);

    rotate(135, PWMRot);        // tourner -135° = 500 pulse
    delay(500);

    rotate(360, PWMRot);      // tourner 360° = 2490 pulse

    STATUS = "no go";
  }
}


// -------- Arduino pro mini ---------
void call(byte cmd) 
{
  Wire.beginTransmission(Mini_ADDR);
  Wire.write(cmd);
  Wire.endTransmission();
  }

void commencer(){
  int buttonState = digitalRead(buttonPin);
  display1.clearDisplay();
  display1.setTextColor(WHITE);
  display1.setTextSize(3);
  display1.setCursor(40,20);
  display1.println("GO?");
  display1.display();

  while (buttonState==1){
    buttonState = digitalRead(buttonPin);
    if (buttonState==0){ 
      Serial.println("Go");
    }
   }
}

void dessinerOeil() {
  display1.clearDisplay();
  display1.fillCircle(64, 32, 28, SSD1306_WHITE);  // blanc de l'oeil, centré
  display1.fillCircle(64, 32, 15, SSD1306_BLACK);  // pupille
  display1.fillCircle(56, 24,  6, SSD1306_WHITE);  // reflet
  display1.display();
}

void dessinerOeilFerme() {
  display1.clearDisplay();
  display1.fillRoundRect(20, 29, 88, 6, 3, SSD1306_WHITE);  // paupière centrée
  display1.display();
}

void clignerYeux(int blink){
  display1.clearDisplay();
  for (int i=0;i<blink;i++){
    dessinerOeil();
    delay(2000);
    dessinerOeilFerme();
    delay(120);
  }
  dessinerOeil();
}

int controllerPWM(){
  int xVal = analogRead(xPin);
  int buttonState = digitalRead(buttonPin);
  int PWM;

  while(buttonState==1){
    xVal = analogRead(xPin);
    buttonState = digitalRead(buttonPin);

    if (xVal < 300){
      if (speedMode>=2){
        speedMode -= 1;
        displaySpeed(speedMode);
      } 
    }

    else if(xVal>800){
      if (speedMode<=2){
        speedMode += 1;
        displaySpeed(speedMode);
      } 
    }

    Serial.print("X: ");
    Serial.print(xVal);
    Serial.print(" | Button: ");
    Serial.print(buttonState); 
    Serial.print(" | speedMode: ");
    Serial.println(speedMode);
    delay(250);
  }

  if(speedMode==1){
    PWM = 140;
  }
  else if(speedMode==2){
    PWM = 190;
  }
  else{
    PWM = 230;
  }
  Serial.print("PWM: ");
  Serial.println(PWM);
  return PWM;
}

// --- Display vitesse sur LCD ---
void displaySpeed(int mode){
  display1.clearDisplay();
  display1.setTextColor(WHITE);
  display1.setTextSize(3);

  if(mode==1){
    display1.setCursor(30,20);
    display1.println("LENT");
    display1.display();
  }
  else if(mode==2){
    display1.setCursor(10,20);
    display1.println("MODERE");
    display1.display();
  }
  else{
    display1.setCursor(10,20);
    display1.println("RAPIDE");
    display1.display();
  }
}

// Prendre jouet
void prendreJouet(){
  display1.clearDisplay();
  display1.setTextColor(WHITE);
  display1.setTextSize(3);
  display1.setCursor(20,20);
  display1.println("Jouet");
  display1.display();

  int buttonState = digitalRead(buttonPin);

  while (buttonState==1){
    buttonState = digitalRead(buttonPin);
    if (buttonState == 0){
      delay(2000);
      call(Machoire_fermer);  // tenir le jouet
    }
   }
}

// ---- Transformer distance en pulse ----
int convertDistance(float distance){
  rotation = distance / (PI*diametre_roue);
  targetDist = round(rotation*120*8);
  return targetDist;
}

// ---- Transformer angle en pulse ----
int convertAngle(float angle){
  deplacement = angle / 360 * (distance_roues*PI);
  revolution = deplacement / (PI*diametre_roue);
  targetTurn = round(revolution*120*8);
  return targetTurn;
}

// ---------- INTERRUPTIONS ----------
void driveForward(long distance, int PWM)
{
  countL = 0;
  countR = 0;
  basePWM=PWM;
  targetDist = convertDistance(distance);

// avance ou recule
  if(targetDist>0){
    forward();
  }else{
    backward();
  }

// engage les moteurs
  while(abs(countL) < abs(targetDist) && abs(countR) < abs(targetDist))
  {
    int pwmL;
    int pwmR;

    PID(pwmL,pwmR);

    analogWrite(PWMA,pwmL);
    analogWrite(PWMB,pwmR);

    monitorPrint(pwmL,pwmR);
  
  // deceleration avant stop
  if(abs(countL) > abs(targetDist)-300){
    basePWM=90;
    }

    delay(20);
  }

  stopMotors();
}

void rotate(long angle, int PWM)
{
  countL = 0;
  countR = 0;
  basePWM = PWM;
  targetTurn = convertAngle(angle);

// virage droit ou gauche
  if(targetTurn>0){
    turnLeft();
  }else{
    turnRight();
  }
  
// engage les moteurs
  while(abs(countL) < abs(targetTurn) && abs(countR) < abs(targetTurn))
  {
    analogWrite(PWMA, basePWM);
    analogWrite(PWMB, basePWM);

    if(abs(countL) > abs(targetTurn)-100){
    basePWM=70;
    }
  }

  stopMotors();
  delay(300);
}

void PID(int &pwmL, int &pwmR)
{
  long error = countL - countR;

  int correction = direction * Kp * error;

  pwmL = basePWM - correction;
  pwmR = basePWM + correction;

  pwmL = constrain(pwmL, 0, 255);
  pwmR = constrain(pwmR, 0, 255);
}

void monitorPrint(int pwmL, int pwmR)
{
  Serial.print("L:");
  Serial.print(countL);
  Serial.print(" R:");
  Serial.print(countR);
  Serial.print(" pwmL:");
  Serial.print(pwmL);
  Serial.print(" pwmR:");
  Serial.println(pwmR);
}

void isrLeft()
{
  byte stateA = digitalRead(encLA);

  if (lastLA == LOW && stateA == HIGH)
  {
    if (digitalRead(encLB)) countL--;
    else countL++;
  }

  lastLA = stateA;
}

void isrRight()
{
  byte stateA = digitalRead(encRA);

  if (lastRA == LOW && stateA == HIGH)
  {
    if (digitalRead(encRB)) countR++;
    else countR--;
  }

  lastRA = stateA;
}

// ---------- MOTEURS ----------
void forward()
{
  direction = 1;

  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);

  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
}
void backward()
{
  direction = -1;

  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);

  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
}

void turnLeft()
{
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);

  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
}

void turnRight()
{
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);

  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
}

void stopMotors()
{
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
}
