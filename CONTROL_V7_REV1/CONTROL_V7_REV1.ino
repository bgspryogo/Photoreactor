// =====================================
// NEXTION EVENT-DRIVEN CONTROL SYSTEM
// ARDUINO MEGA + NEXTION
// =====================================

#define nextion Serial3

// =====================================
// OUTPUT PINS
// =====================================

const int WATER_SEN_LOW = 46;
const int WATER_SEN_HIGH = 47;

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

int WATER_LEVEL;
int prev_WATER_LEVEL;

int MOTOR_SPEED = 255;

const unsigned long RUN_TIME = 30000UL;
const unsigned long OUT_IN_TIME = 40000UL;

// =====================================
// BUTTON STATES
// =====================================

bool inlet_toggle = false;
bool outlet_toggle = false;
bool loop_toggle = false;
bool sample_toggle = false;
bool prevSampleToggle = false;

// ADD THESE - Track when buttons were last pressed
unsigned long lastInletPress = 0;
unsigned long lastOutletPress = 0;
unsigned long lastLoopPress = 0;

// ADD THESE - Auto-stop tracking
bool inletAutoStopped = false;
bool outletAutoStopped = false; 

unsigned long sampleStartTime = 0;
bool sampleRunning = false;
bool sampleDone = true;
String incoming = "";

unsigned long outletDelayStart = 0;
bool outletDelayActive = false;

unsigned long inletDelayStart = 0;
bool inletDelayActive = false;
bool previousInletToggle = false;

// =====================================
// WATER LEVEL
// =====================================

int WATER_LEVEL_MONITORING() {
  if (digitalRead(WATER_SEN_LOW) == LOW && digitalRead(WATER_SEN_HIGH) == LOW) {
    WATER_LEVEL = 1;
  } else if (digitalRead(WATER_SEN_LOW) == HIGH && digitalRead(WATER_SEN_HIGH) == HIGH) {
    WATER_LEVEL = 2;
     } else if (digitalRead(WATER_SEN_LOW) == HIGH && digitalRead(WATER_SEN_HIGH) == LOW){
    WATER_LEVEL = 3;
   }

  if (prev_WATER_LEVEL != WATER_LEVEL) {
    switch (WATER_LEVEL) {
      case 1: 
        Serial.println("Water Low");
        nextionSend("TANK_STATUS.txt=\"EMPTY\"");
        break;
      case 2:
        Serial.println("Water High");
        nextionSend("TANK_STATUS.txt=\"FULL\"");
        break;
      case 3:
        Serial.println("Water Half");
        nextionSend("TANK_STATUS.txt=\"HALF\"");
        break;
    }

    prev_WATER_LEVEL = WATER_LEVEL;
  }

  return WATER_LEVEL;
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

  if (incoming.endsWith("=0") || incoming.endsWith("=1")) {
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

  if (sep == -1) return;

  String key = cmd.substring(0, sep);
  bool turnOn = cmd.substring(sep + 1) == "1";

  // =====================================
  // INLET
  // =====================================

  if (key == "INLET") {

    if (turnOn) {

      if (!loop_toggle && !outlet_toggle) {

        inlet_toggle = true;
        lastInletPress = millis();
        inletAutoStopped = false;

        Serial.println("INLET ON");

      } else {

        Serial.println("INLET BLOCKED");

      }

    } else {

      inlet_toggle = false;
      lastInletPress = millis();
      inletAutoStopped = false;

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
        lastOutletPress = millis();
        outletAutoStopped = false;

        Serial.println("OUTLET ON");

      } else {
        Serial.println("OUTLET BLOCKED");
      }
    } else {
      outlet_toggle = false;
      lastOutletPress = millis();
      outletAutoStopped = false;
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
        lastLoopPress = millis();
        Serial.println("LOOP ON");
      } else {
        Serial.println("LOOP BLOCKED");
      }
    } else {
      loop_toggle = false;
      lastLoopPress = millis();
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

// =====================================
// INLET
// =====================================

static bool inletCommandSent = false;

// Prevent immediate auto-stop after a button press
bool inletRecentlyPressed = (millis() - lastInletPress < 500);

// =====================================
// AUTO STOP WHEN TANK IS FULL
// =====================================

if (WATER_LEVEL == 2 && inlet_toggle == true && !inletRecentlyPressed) {

  inlet_toggle = false;
  inletAutoStopped = true;

  // Start 30-second countdown only once
  if (!inletDelayActive) {
    inletDelayActive = true;
    inletDelayStart = millis();
    Serial.println("INPUT PUMP STOP DELAY STARTED");
  }

  if (!inletCommandSent) {
    inletCommandSent = true;
    nextionSend("click INLET_TOGGLE,1");
    Serial.println("TANK FULL - AUTO STOP");
  }
}

// =====================================
// INLET ON
// =====================================

if (inlet_toggle == true) {

  // Cancel pending auto-stop
  inletDelayActive = false;
  inletAutoStopped = false;
  inletCommandSent = false;

  digitalWrite(INPUT_SOLENOID, LOW);
  digitalWrite(INLET_PUMP, LOW);

  Serial.println("FILLING MAIN TANK");
}

// =====================================
// AUTO STOP DELAY
// =====================================

else if (inletAutoStopped) {

  // Keep filling during delay
  digitalWrite(INPUT_SOLENOID, LOW);
  digitalWrite(INLET_PUMP, LOW);

  if (millis() - inletDelayStart >= 25000UL) {

    digitalWrite(INLET_PUMP, HIGH);
    digitalWrite(INPUT_SOLENOID, HIGH);

    inletDelayActive = false;
    inletAutoStopped = false;
    inletCommandSent = false;

    Serial.println("FILLING MAIN TANK FINISHED");
  }
}

// =====================================
// MANUAL OFF
// =====================================

else {

  inletDelayActive = false;
  inletCommandSent = false;

  digitalWrite(INLET_PUMP, HIGH);
  digitalWrite(INPUT_SOLENOID, HIGH);
}

 // =====================================
// OUTLET
// =====================================

static bool outletCommandSent = false;

// Check if button was pressed recently (within 500ms)
bool outletRecentlyPressed = (millis() - lastOutletPress < 500);

// =====================================
// OUTLET OFF
// =====================================

if (outlet_toggle == false || WATER_LEVEL == 1) {

  // Start delay only once
  if (!outletDelayActive) {
    outletDelayActive = true;
    outletDelayStart = millis();
    Serial.println("OUTPUT PUMP STOP DELAY STARTED");
  }

  // Wait 30 seconds before turning pump OFF
  if (millis() - outletDelayStart >= 40000UL) {

    digitalWrite(OUTPUT_SOLENOID, HIGH);
    digitalWrite(OUTPUT_PUMP, HIGH);   // Pump OFF (Active LOW)

    outletDelayActive = false;

    Serial.println("DRAINING MAIN TANK FINISHED");
  }

  // Only auto-stop if NOT recently pressed
  if (WATER_LEVEL == 1 && outlet_toggle == true && !outletRecentlyPressed) {

    outlet_toggle = false;
    outletAutoStopped = true;

    if (!outletCommandSent) {
      outletCommandSent = true;
      nextionSend("click OUTLET_TOGGLE,1");
      Serial.println("TANK EMPTY - AUTO STOP");
    }

  } else {

    outletCommandSent = false;

    if (WATER_LEVEL != 1) {
      outletAutoStopped = false;
    }
  }

}

// =====================================
// OUTLET ON
// =====================================

else if (outlet_toggle == true) {

  // Cancel any pending shutdown
  outletDelayActive = false;

  digitalWrite(OUTPUT_SOLENOID, LOW);
  digitalWrite(OUTPUT_PUMP, LOW);   // Pump ON (Active LOW)

  outletCommandSent = false;

  Serial.println("DRAINING MAIN TANK");
}
  
  // =====================================
  // LOOP
  // =====================================

  static bool prevLoopState = false;
  
  if (loop_toggle == true) {
    digitalWrite(LOOP_PUMP, LOW);
    
    if (!prevLoopState) {
      prevLoopState = true;
      Serial.println("CYCLING MAIN TANK");
    }
  } else if (loop_toggle == false) {
    digitalWrite(LOOP_PUMP, HIGH);
    
    if (prevLoopState) {
      prevLoopState = false;
      Serial.println("CYCLING MAIN TANK FINISHED");
    }
  }
}

void SAMPLE_TANK() {

  //=====================================
  // Detect state change
  //=====================================

  if (sample_toggle != prevSampleToggle) {

    prevSampleToggle = sample_toggle;

    sampleRunning = true;
    sampleStartTime = millis();

    digitalWrite(AIR_SOLENOID, HIGH);

    if (sample_toggle) {

      Serial.println("FORWARD START");
      digitalWrite(AIR_SOLENOID, HIGH);
      analogWrite(RPWM, MOTOR_SPEED);
      analogWrite(LPWM, 0);

      nextionSend("SAMPLE_FORWARD.txt=\"FILLING\"");
      nextionSend("SAMPLE_REVERSE.txt=\"OFFLINE\"");

    }
    else {

      Serial.println("REVERSE START");
      digitalWrite(AIR_SOLENOID, HIGH);
      analogWrite(RPWM, 0);
      analogWrite(LPWM, MOTOR_SPEED);

      nextionSend("SAMPLE_FORWARD.txt=\"OFFLINE\"");
      nextionSend("SAMPLE_REVERSE.txt=\"DRAINING\"");
    }
  }

  //=====================================
  // Stop after 30 seconds
  //=====================================

  if (sampleRunning) {

    if (millis() - sampleStartTime >= RUN_TIME) {

      stopMotor();

      digitalWrite(AIR_SOLENOID, LOW);

      nextionSend("SAMPLE_FORWARD.txt=\"OFFLINE\"");
      nextionSend("SAMPLE_REVERSE.txt=\"OFFLINE\"");

      sampleRunning = false;

      Serial.println("SAMPLE COMPLETE");
    }
  }
}

// =====================================
// STOP MOTOR
// =====================================

void stopMotor() {
  analogWrite(RPWM, 0);
  analogWrite(LPWM, 0);
}


// =====================================
// SETUP
// =====================================

void setup() {
  Serial.begin(115200);
  nextion.begin(115200);

  // OUTPUTS
  pinMode(AIR_SOLENOID, OUTPUT);
  pinMode(INPUT_SOLENOID, OUTPUT);
  pinMode(OUTPUT_PUMP, OUTPUT);
  pinMode(OUTPUT_SOLENOID, OUTPUT);
  pinMode(INLET_PUMP, OUTPUT);
  pinMode(LOOP_PUMP, OUTPUT);

  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);

  // INPUTS

  pinMode(WATER_SEN_LOW, INPUT);
  pinMode(WATER_SEN_HIGH, INPUT);

  // ACTIVE LOW RELAY DEFAULTS
  digitalWrite(INPUT_SOLENOID, HIGH);
  digitalWrite(INLET_PUMP, HIGH);
  digitalWrite(LOOP_PUMP, HIGH);
  digitalWrite(OUTPUT_SOLENOID, HIGH);
  digitalWrite(OUTPUT_PUMP, HIGH);
  digitalWrite(AIR_SOLENOID, LOW);   

  stopMotor();

  Serial.println("SYSTEM READY");
}

// =====================================
// MAIN LOOP
// =====================================

// =====================================
// MAIN LOOP
// =====================================

void loop() {

  unsigned long t;

  // Measure readNextion()
  t = micros();
  readNextion();
  unsigned long nextionTime = micros() - t;

  // Measure WATER_LEVEL_MONITORING()
  t = micros();
  WATER_LEVEL_MONITORING();
  unsigned long waterTime = micros() - t;

  // Measure controlOutputs()
  t = micros();
  controlOutputs();
  unsigned long controlTime = micros() - t;

  // Measure SAMPLE_TANK()
  t = micros();
  SAMPLE_TANK();
  unsigned long sampleTime = micros() - t;

  // Total loop time
  unsigned long totalTime = nextionTime + waterTime + controlTime + sampleTime;

  // Print once every second
  static unsigned long lastPrint = 0;

  if (millis() - lastPrint >= 1000) {

    Serial.println();
    Serial.println("========== LOOP PROFILE ==========");

    Serial.print("readNextion()          : ");
    Serial.print(nextionTime);
    Serial.println(" us");

    Serial.print("WATER_LEVEL_MONITORING(): ");
    Serial.print(waterTime);
    Serial.println(" us");

    Serial.print("controlOutputs()       : ");
    Serial.print(controlTime);
    Serial.println(" us");

    Serial.print("SAMPLE_TANK()          : ");
    Serial.print(sampleTime);
    Serial.println(" us");

    Serial.println("----------------------------------");

    Serial.print("TOTAL LOOP             : ");
    Serial.print(totalTime);
    Serial.println(" us");

    Serial.print("Approx Max Loops/sec   : ");
    Serial.println(1000000UL / max(1UL, totalTime));

    Serial.println("==================================");
    Serial.println();

    lastPrint = millis();
  }
}