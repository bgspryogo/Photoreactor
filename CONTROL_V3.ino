// =====================================
// NEXTION EVENT-DRIVEN CONTROL SYSTEM
// ARDUINO MEGA + NEXTION
// =====================================

#define nextion Serial3

// =====================================
// OUTPUT PINS
// =====================================

const int TMP36_1 = A0;
const int TMP36_2 = A1;

const int FAN_1 = 24;
const int FAN_2 = 25;

const int WATER_SEN_LOW = 46;
const int WATER_SEN_HIGH = 47;

const int INPUT_SOLENOID = 26;
const int LOOP_SOLENOID = 27;
const int OUTPUT_SOLENOID = 28;
const int OUTPUT_PUMP = 29;
const int INLET_LOOP_PUMP = 31;

const int RPWM = 44;
const int LPWM = 45;

// =====================================
// DATA VARIABLES
// =====================================

int WATER_LEVEL;

double TEMPERATURE_1;
double TEMPERATURE_2;
double TEMPERATURE_AVG;

int MOTOR_SPEED = 255;
int RUN_TIME = 5000;

// =====================================
// BUTTON STATES
// =====================================

bool inlet_toggle = false;
bool outlet_toggle = false;
bool loop_toggle = false;
bool sample_toggle = false;

bool sampleDone = false;
bool prevSampleToggle = false;

String incoming = "";

// =====================================
// SETUP
// =====================================

void setup() {

  Serial.begin(9600);
  nextion.begin(9600);

  // OUTPUTS
  pinMode(INPUT_SOLENOID, OUTPUT);
  pinMode(LOOP_SOLENOID, OUTPUT);
  pinMode(OUTPUT_PUMP, OUTPUT);
  pinMode(OUTPUT_SOLENOID, OUTPUT);
  pinMode(INLET_LOOP_PUMP, OUTPUT);

  pinMode(FAN_1, OUTPUT);
  pinMode(FAN_2, OUTPUT);

  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);

  // INPUTS
  pinMode(TMP36_1, INPUT);
  pinMode(TMP36_2, INPUT);

  pinMode(WATER_SEN_LOW, INPUT);
  pinMode(WATER_SEN_HIGH, INPUT);

  // ACTIVE LOW RELAY DEFAULTS
  digitalWrite(INPUT_SOLENOID, HIGH);
  digitalWrite(LOOP_SOLENOID, HIGH);
  digitalWrite(OUTPUT_PUMP, HIGH);
  digitalWrite(INLET_LOOP_PUMP, HIGH);

  stopMotor();

  Serial.println("SYSTEM READY");
}

// =====================================
// MAIN LOOP
// =====================================

void loop() {
  readNextion();
  WATER_LEVEL_MONITORING();
  controlOutputs();
  SAMPLE_TANK();
    FAN_CONTROL();
}

// =====================================
// WATER LEVEL
// =====================================

int WATER_LEVEL_MONITORING() {

  if (digitalRead(WATER_SEN_LOW) == LOW &&
      digitalRead(WATER_SEN_HIGH) == LOW) {

    WATER_LEVEL = 1;
    Serial.print("Water Low");

  }
  
  else if (digitalRead(WATER_SEN_LOW) == HIGH &&
           digitalRead(WATER_SEN_HIGH) == HIGH) {

    WATER_LEVEL = 2;
    Serial.print("Water High");

  }

  return WATER_LEVEL;
}

// =====================================
// FAN CONTROL
// =====================================

void FAN_CONTROL() {

  double reading_temp1 = analogRead(TMP36_1);
  double voltage_1 = reading_temp1 * (5.0 / 1024.0);
  TEMPERATURE_1 = (voltage_1 - 0.5) * 100;

  double reading_temp2 = analogRead(TMP36_2);
  double voltage_2 = reading_temp2 * (5.0 / 1024.0);
  TEMPERATURE_2 = (voltage_2 - 0.5) * 100;

  TEMPERATURE_AVG = (TEMPERATURE_1 + TEMPERATURE_2) / 2.0;

  Serial.print("TEMP AVG: ");
  Serial.println(TEMPERATURE_AVG);

  if (TEMPERATURE_AVG >= 40) {

    digitalWrite(FAN_1, LOW);
    digitalWrite(FAN_2, LOW);

  } else {

    digitalWrite(FAN_1, HIGH);
    digitalWrite(FAN_2, HIGH);
  }

  nextion_temperature_logging();

  delay(1000);
}

// =====================================
// NEXTION TEMP LOGGING
// =====================================

void nextion_temperature_logging() {

  int temperature_nextion = TEMPERATURE_AVG * 100;

  nextion.print("TEMPERATURE.val=");
  nextion.print(temperature_nextion);

  nextion.write(0xFF);
  nextion.write(0xFF);
  nextion.write(0xFF);
}

// =====================================
// READ NEXTION
// =====================================

void readNextion() {

  while (nextion.available()) {

    char c = nextion.read();

    if (c == '\n' || c == '\r') {
      continue;
    }

    incoming += c;

    if (incoming.endsWith("0") ||
        incoming.endsWith("1")) {

      Serial.print("RECEIVED: ");
      Serial.println(incoming);

      parseCommand(incoming);

      incoming = "";
    }
  }
}

// =====================================
// NEXTION HELPER
// =====================================

void nextionSend(String cmd) {

  nextion.print(cmd);

  nextion.write(0xFF);
  nextion.write(0xFF);
  nextion.write(0xFF);
}

// =====================================
// BLOCK TOGGLE
// =====================================

void blockToggle(String toggleName) {

  Serial.println(toggleName + " BLOCKED");

  nextionSend(toggleName + "_TOGGLE.val=0");
}

// =====================================
// PARSE COMMAND
// =====================================

void parseCommand(String cmd) {

  int sep = cmd.indexOf('=');

  String key = cmd.substring(0, sep);

  bool turnOn = cmd.substring(sep + 1) == "1";

  // =====================================
  // INLET
  // =====================================

  if (key == "INLET") {

    if (turnOn) {

      if (!loop_toggle && !outlet_toggle) {

        inlet_toggle = true;

        Serial.println("INLET ON");

      } else {

        blockToggle("INLET");
      }

    } else {

      inlet_toggle = false;

      Serial.println("INLET OFF");
    }
  }

  // =====================================
  // OUTLET
  // =====================================

  else if (key == "OUTLET") {

    if (turnOn) {

      if (!inlet_toggle && !loop_toggle) {

        outlet_toggle = true;

        Serial.println("OUTLET ON");

      } else {

        blockToggle("OUTLET");
      }

    } else {

      outlet_toggle = false;

      Serial.println("OUTLET OFF");
    }
  }

  // =====================================
  // LOOP
  // =====================================

  else if (key == "LOOP") {

    if (turnOn) {

      if (!inlet_toggle && !outlet_toggle) {

        loop_toggle = true;

        Serial.println("LOOP ON");

      } else {

        blockToggle("LOOP");
      }

    } else {

      loop_toggle = false;

      Serial.println("LOOP OFF");
    }
  }

  // =====================================
  // SAMPLE
  // =====================================

  else if (key == "SAMPLE") {

    sample_toggle = turnOn;

    Serial.println(turnOn ? "SAMPLE ON" : "SAMPLE OFF");
  }
}

// =====================================
// CONTROL OUTPUTS
// =====================================

void controlOutputs() {

  String global_status = "NO OPERATION";

  // =====================================
  // INLET
  // =====================================

  if (WATER_LEVEL == 1 && inlet_toggle == true) {

    digitalWrite(INPUT_SOLENOID, LOW);

    MAIN_PUMP_CONTROL();

    nextionSend("INLET_PROCESS.txt=\"FILLING\"");

    nextionSend("OUTLET_TOGGLE.val=0");
    nextionSend("LOOP_TOGGLE.val=0");

    global_status = "FILLING MAIN TANK";

  } else {

    digitalWrite(INPUT_SOLENOID, HIGH);

    nextionSend("INLET_PROCESS.txt=\"OFFLINE\"");
    nextionSend("INLET_TOGGLE.val=0");
  }

  // =====================================
  // OUTLET
  // =====================================

  if (WATER_LEVEL == 2 && outlet_toggle == true) {

    digitalWrite(OUTPUT_SOLENOID, LOW);
    delay(200);
    digitalWrite(OUTPUT_PUMP, LOW);

    nextionSend("OUTLET_PROCESS.txt=\"DRAINING\"");

    nextionSend("INLET_TOGGLE.val=0");
    nextionSend("LOOP_TOGGLE.val=0");

    global_status = "DRAINING MAIN TANK";

  } else {

    digitalWrite(OUTPUT_PUMP, HIGH);
    delay(200);
    digitalWrite(OUTPUT_SOLENOID, HIGH);

    nextionSend("OUTLET_PROCESS.txt=\"OFFLINE\"");
    nextionSend("OUTLET_TOGGLE.val=0");
  }

  // =====================================
  // LOOP
  // =====================================

  if (loop_toggle == true) {

    MAIN_PUMP_CONTROL();

    nextionSend("INLET_TOGGLE.val=0");
    nextionSend("OUTLET_TOGGLE.val=0");

    global_status = "CYCLING MAIN TANK";

  } else {

    MAIN_PUMP_CONTROL();

    nextionSend("LOOP_TOGGLE.val=0");
  }

  // =====================================
  // GLOBAL STATUS
  // =====================================

  nextion.print("GLOBAL_STATUS.txt=\"");
  nextion.print(global_status);
  nextion.print("\"");

  nextion.write(0xFF);
  nextion.write(0xFF);
  nextion.write(0xFF);
}

// =====================================
// MAIN PUMP CONTROL
// =====================================

void MAIN_PUMP_CONTROL() {

  if (loop_toggle == true ||
      (WATER_LEVEL == 1 && inlet_toggle == true)) {

    digitalWrite(INLET_LOOP_PUMP, LOW);

  } else {

    digitalWrite(INLET_LOOP_PUMP, HIGH);
  }
}

// =====================================
// SAMPLE TANK
// =====================================

void SAMPLE_TANK() {

  if (sample_toggle != prevSampleToggle) {
    sampleDone = false;
    prevSampleToggle = sample_toggle;
  }

  if (sampleDone) return;

  // =====================================
  // FORWARD
  // =====================================

  if (sample_toggle == true) {
    Serial.println("[FORWARD]");
    analogWrite(RPWM, MOTOR_SPEED);
    analogWrite(LPWM, 0);
    nextionSend("SAMPLE_FORWARD.txt=\"FILLING\"");
    delay(RUN_TIME);
    stopMotor();
    nextionSend("SAMPLE_FORWARD.txt=\"OFFLINE\"");
    sampleDone = true;
  }

  // =====================================
  // REVERSE
  // =====================================

  else if (sample_toggle == false) {
    Serial.println("[REVERSE]");
    analogWrite(RPWM, 0);
    analogWrite(LPWM, MOTOR_SPEED);
    nextionSend("SAMPLE_REVERSE.txt=\"DRAINING\"");
    delay(RUN_TIME);
    stopMotor();
    nextionSend("SAMPLE_REVERSE.txt=\"OFFLINE\"");
    sampleDone = true;
  }
}

// =====================================
// STOP MOTOR
// =====================================

void stopMotor() {

  analogWrite(RPWM, 0);
  analogWrite(LPWM, 0);
}