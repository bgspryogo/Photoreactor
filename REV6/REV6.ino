#include "EasyNextionLibrary.h"

EasyNex myNex(Serial3);

// =====================================
// OUTPUT PINS
// =====================================

const int waterLowsensor = 46;
const int waterHighsensor = 47;

const int INLET_PUMP = 22;
const int AIR_SOLENOID = 23;
const int INPUT_SOLENOID = 26;
const int LOOP_PUMP = 27;
const int OUTPUT_SOLENOID = 28;
const int OUTPUT_PUMP = 29;

const int RPWM = 44;
const int LPWM = 45;

// =====================================
// DATA VARIABLES
// =====================================

int waterLevel;
int prev_waterLevel;

int MOTOR_SPEED = 255;

void setup() {
  Serial.begin(9600);
  Serial3.begin(9600);
  myNex.begin(9600);

  //OUTPUTS
  pinMode(AIR_SOLENOID, OUTPUT);
  pinMode(INPUT_SOLENOID, OUTPUT);
  pinMode(OUTPUT_PUMP, OUTPUT);
  pinMode(OUTPUT_SOLENOID, OUTPUT);
  pinMode(INLET_PUMP, OUTPUT);
  pinMode(LOOP_PUMP, OUTPUT);

  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);

  //INPUTS
  pinMode(waterLowsensor, INPUT);
  pinMode(waterHighsensor, INPUT);

  //ACTIVE LOW RELAY DEFAULTS
  digitalWrite(INPUT_SOLENOID, HIGH);
  digitalWrite(INLET_PUMP, HIGH);
  digitalWrite(LOOP_PUMP, HIGH);
  digitalWrite(OUTPUT_SOLENOID, HIGH);
  digitalWrite(OUTPUT_PUMP, HIGH);
  digitalWrite(AIR_SOLENOID, LOW);   

  stopMotor();

  myNex.writeStr("page1.WATER_LVL.txt", "LOW");
  Serial.println("SYSTEM READY");
}

void loop() {
  // put your main code here, to run repeatedly:
  myNex.NextionListen(); 
  waterMonitoring();
  inletAutooff();
  outletAutooff();
}

//WATER LEVEL MONITORNG
int waterMonitoring() {
  if (digitalRead(waterLowsensor)==LOW && digitalRead(waterHighsensor)==LOW) {
  waterLevel = 1;
  } else if (digitalRead(waterLowsensor)==HIGH && digitalRead(waterHighsensor)==HIGH) {
  waterLevel = 2;
  } else if (digitalRead(waterLowsensor)==LOW && digitalRead(waterHighsensor)==HIGH){
  waterLevel = 3;
  }

  if(prev_waterLevel != waterLevel) {
    switch (waterLevel) {
      case 1: 
        Serial.println("Water Low");
        myNex.writeStr("page1.WATER_LVL.txt", "LOW");
        break;
      case 2:
        Serial.println("Water Full");
        myNex.writeStr("page1.WATER_LVL.txt", "FULL");
        break;
      case 3:
        Serial.println("Water Half");
        myNex.writeStr("page1.WATER_LVL.txt", "HALF");
        break;
    }
    prev_waterLevel = waterLevel;
  }
  return waterLevel;
}

//INLET ON
void trigger0() {
  digitalWrite(INPUT_SOLENOID, LOW);
  digitalWrite(INLET_PUMP, LOW);
}

//INLET MANUAL OFF 
void trigger1() {
  digitalWrite(INPUT_SOLENOID, HIGH);
  digitalWrite(INLET_PUMP, HIGH);
}

//INLET AUTO OFF
void inletAutooff() {
  if(waterLevel==2){
    digitalWrite(INPUT_SOLENOID, HIGH);
    digitalWrite(INLET_PUMP, HIGH);
  }
}

//CYCLE ON
void trigger2() {
  digitalWrite(LOOP_PUMP, LOW);
}

//CYCLE OFF 
void trigger3() {
  digitalWrite(LOOP_PUMP, HIGH);
}

//OUTLET ON
void trigger4() {
  digitalWrite(OUTPUT_SOLENOID, LOW);
  digitalWrite(OUTPUT_PUMP, LOW);   
}

//OUTLET MANUAL OFF
void trigger5() {
  digitalWrite(OUTPUT_PUMP, HIGH);   
  digitalWrite(OUTPUT_SOLENOID, HIGH);
}

//OUTLET AUTO OFF
void outletAutooff() {
  if(waterLevel==1) {
    digitalWrite(OUTPUT_PUMP, HIGH);   
    digitalWrite(OUTPUT_SOLENOID, HIGH);
  }
}

//SAMPLE FILL ON
void trigger6() {
  Serial.println("FILLING SAMPLE TANK");
  digitalWrite(AIR_SOLENOID, HIGH);
  analogWrite(RPWM, MOTOR_SPEED);
  analogWrite(LPWM, 0);
}

//SAMPLE FILL OFF
void trigger7() {
  Serial.println("STOPPED FILLING SAMPLE TANK");
  stopMotor();
}

//SAMPLE DRAIN ON
void trigger8() {
  Serial.println("DRAINING SAMPLE TANK");
  digitalWrite(AIR_SOLENOID, HIGH);
  analogWrite(RPWM, 0);
  analogWrite(LPWM, MOTOR_SPEED);
}

//SAMPLE DRAIN OFF
void trigger9() {
  Serial.println("STOPPED DRAINING SAMPLE TANK");
  stopMotor();
}

//STOP SAMPLE MOTOR
void stopMotor() {
  analogWrite(RPWM, 0);
  analogWrite(LPWM, 0);
}





