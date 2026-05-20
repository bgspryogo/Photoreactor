#include <SoftwareSerial.h>

SoftwareSerial nextionSerial(0, 1); // RX, TX

const int TMP36_1 = A0, TMP36_2 = A1;
const int FAN_1 = 24, FAN_2 = 25;
double temperature_1, temperature_2;

double calc_TMP36_1() {
  double reading_temp1 = analogRead(TMP36_1);
  double voltage = reading_temp1 * (5.0 / 1024.0);
  double temperatureC_1 = (voltage - 0.5) * 100;  
  return temperature_1;
}

double calc_TMP36_2() {
  double reading_temp2 = analogRead(TMP36_2);
  double voltage = reading_temp2 * (5.0 / 1024.0);
  double temperatureC_2 = (voltage - 0.5) * 100;  
  return temperature_2;
}

void reactor_fan_control() {
  double temperature_sum = (temperature_1 + temperature_2) / 2;

  if (temperature_sum >= 40) {
    digitalWrite(TMP36_1, LOW);
    digitalWrite(TMP36_2, LOW);
  } else {
    digitalWrite(TMP36_1, HIGH);
    digitalWrite(TMP36_2, HIGH);
  }
}

const int water_low = 28, water_high = 29;
String water_status;

String water_monitoring() {
  if (digitalRead(water_low) == LOW && digitalRead(water_high) == LOW) {
    water_status = "Water level low";
    } else if (digitalRead(water_low) == HIGH && digitalRead(water_high) == HIGH) {
    water_status = "Water level high";
  }
}

const int input_pump = 26, input_solenoid = 27;

void execute_input() {
  if (water_status == "Water level low" && INPUT.val == 1) {
    digitalWrite(input_solenoid, LOW);
    delay(100);
    digitalWrite(input_pump, LOW);
  } else if (water_status == "Water level high" || INPUT.val == 0) {
    digitalWrite(input_pump, HIGH);
    delay(100);
    digitalWrite(input_solenoid, HIGH);
  } else {
    digitalWrite(input_pump, HIGH);
    delay(100);
    digitalWrite(input_solenoid, HIGH);
  }
}

const int loop_pump = 28, loop_solenoid = 29;

void execute_loop() {
  if (water_status == "Water level low" && LOOP.val == 1) {
    digitalWrite(input_solenoid, LOW);
    delay(100);
    digitalWrite(input_pump, LOW);
  } else if (water_status == "Water level high" || LOOP.val == 0) {
    digitalWrite(input_pump, HIGH);
    delay(100);
    digitalWrite(input_solenoid, HIGH);
  } else {
    digitalWrite(input_pump, HIGH);
    delay(100);
    digitalWrite(input_solenoid, HIGH);
  }
}

const int sample_pump = 30;

void execute_sample_pump() {


  
}


void setup() {
  // put your setup code here, to run once:
  pinMode(TMP36_1, INPUT);
  pinMode(TMP36_2, INPUT);
  pinMode(FAN_1, OUTPUT);
  pinMode(FAN_2, OUTPUT);
  Serial.begin(115200);
  nextionSerial.begin(115200);
}

void loop() {
  calc_TMP36_1();
  calc_TMP36_2();
  reactor_fan_control();

}
