/*
================================================================================
M.O.O - Mechanical Operations Orchestrator
Coordinating B.E.E.F. and C.A.L.F. systems for seamless bovine automation
├── B.E.E.F. (Bovine Extension Evaluation Framework)
└── C.A.L.F. (Cow Actuator Location Feedback)
================================================================================
Version: 4.0
Date: 7/24/25
Author: JonnyVermont

CHANGE LOG:
v3.0 - 7/22/25: MAJOR UPDATE - Implemented sensor-based overlapping movements:
                - Door opening triggers cow extension immediately upon door open sensor
                - Cow retraction triggers door closing immediately upon cow in sensor
                - Removed time-based transitions in favor of sensor-driven state changes
                - Added safety timeouts (original time + 2 seconds) to prevent stuck motors
                - Much faster overall operation with overlapping movements
                - ENHANCED DEBUGGING: Added detailed trigger source identification
v2.9 - 7/19/25: Added button debouncing (500ms) to prevent button spam/bouncing issues
v2.8 - 7/19/25: Added safety LED indicator - lights when cow head is unsafe (GPIO 2)
v2.7 - 7/19/25: FIXED corrupted safety limits issue - ResetLimits command working,
                added debug output for safety limit verification, improved limit logic

================================================================================
CRITICAL ISSUE FOR EVAN - BOTTANGO INTEGRATION PROBLEM
================================================================================

ISSUE DISCOVERED: GPIO 13 (Impulse Cow extend signal) is automatically going 
HIGH/LOW even when Bottango is NOT sending any commands. This causes the cow 
system to trigger extend sequences randomly without user input.

UPDATE: Moved from GPIO 13 to GPIO 15 - STILL HAPPENING! Problem may be broader
than just one pin. Enhanced debugging added to identify exact trigger sources.

SYMPTOMS OBSERVED:
- GPIO pins oscillating HIGH/LOW every ~2 seconds
- System responds by opening door and extending cow
- This happens even when Bottango software is not sending triggers
- Debug output shows: "DEBUG: Impulse Extend pin changed to HIGH" followed by 
  "Extend due to Impulse Cow 1&2 signal (debounced)"

WORK COMPLETED:
1. Fixed code to expect HIGH signals from Bottango (was expecting LOW)
2. Added 2-second debouncing to prevent rapid oscillation  
3. Added debug commands: EnableImpulse, DisableImpulse, DebugImpulse, CheckImpulse
4. Confirmed code now responds correctly to Bottango HIGH signals
5. BUT: GPIO pins are triggering automatically without Bottango commands
6. MOVED from GPIO 13 to GPIO 15 - problem persists
7. ADDED ENHANCED DEBUGGING to identify exact trigger sources

POTENTIAL HARDWARE CAUSES:
- Multiple ESP32 GPIO pins malfunctioning
- Electromagnetic interference from nearby equipment
- Power supply noise affecting multiple GPIO inputs
- Cross-talk between wires or loose connections
- Broader ESP32 board failure affecting multiple pins

TESTING NEEDED:
1. Physical disconnect ALL Impulse wires, check if ESP32 pins still oscillate
2. Try different power source for ESP32 (battery vs USB vs external supply)
3. Check for EMI sources near ESP32
4. Test ESP32 in different physical location away from motors/actuators
5. Try different ESP32 board to isolate hardware vs environmental issues

CURRENT WORKAROUND:
- DisableImpulse command disables GPIO monitoring (system safe)
- Physical buttons (GPIO 32/33) still work for manual operation
- Serial commands work for testing (CowOut, CowIn, etc.)
- GPIO 18 calibration trigger works correctly

BOTTANGO INTEGRATION STATUS:
- Code is ready and tested for proper Bottango HIGH signal integration
- Problem is hardware-level or environmental, not software-level
- Once hardware issue resolved, Bottango should work perfectly

FOR EVAN: Investigate power supply, EMI sources, and try different ESP32 board.

================================================================================
v2.6 - 7/17/25: Added Bottango calibration trigger system - waits for external trigger
                before gyroscope calibration, ensures heads are straight first
v2.5 - 7/17/25: Added gyroscope yaw calculation, removed roll from safety check,
                now checks PITCH (up/down) and YAW (left/right) only
v2.4 - 7/17/25: Added MPU-6050 cow head angle monitoring for Cow #1, updated button pins,
                added angle safety checking before cow extension
v2.3 - 7/17/25: Updated for Hall sensors (active LOW), correct timing values (8s/10s),
                added 100ms debouncing, verbose sensor monitoring
v2.2 - 7/16/25: [Missing version - gap in documentation]
v2.1 - 7/16/25: Updated variable names - la_ prefix for actuators, solar signals
v2.0 - 7/16/25: Renamed all variables for clarity, updated state names
v1.3 - 7/6/24:  Original working version with basic state machine

DESCRIPTION:
Controls linear actuators for animatronic cow system. Door opens first, 
then cow extends through window, then reverses for hide sequence.
NOW WITH: Cow head PITCH and YAW monitoring with Bottango-triggered calibration.
v3.0: Sensor-based overlapping movements for faster operation.

HARDWARE:
- ESP32 microcontroller (the main brain that controls everything)
- 2x Linear actuators (motors that push/pull the cow rig and door) via BTS7960 motor drivers  
- 2x Physical buttons (manual override buttons you can press)
- 4x A3144 Hall effect sensors (magnetic sensors that detect when things are fully extended/retracted)
- 1x MPU-6050 (motion sensor that detects cow head tilt and rotation) - TESTING PHASE
- Impulse Cow 1&2 board integration (Bottango control system signals)
- Bottango calibration trigger (signal from Bottango to start sensor calibration)

WIRING:
Cow Rig:       GPIO 26 (extend), GPIO 25 (retract)
Door:          GPIO 27 (open), GPIO 14 (close) 
Buttons:       GPIO 32 (open), GPIO 33 (close)
Impulse Cow:   GPIO 15 (extend - HIGH to trigger), GPIO 4 (retract - HIGH to trigger)
Sensors:       GPIO 21 (cow out), GPIO 19 (cow in), GPIO 23 (door open), GPIO 22 (door closed)
MPU-6050:      GPIO 16 (SDA), GPIO 17 (SCL), 3.3V, GND
Calibration:   GPIO 18 (trigger from Impulse Cow 3&4 Board Pin 32 - HIGH to trigger)
Safety LED:    GPIO 2 (lights when cow head unsafe)

SENSOR NOTES:
- A3144 Hall sensors are active LOW when magnet is present (pin reads LOW when magnet touches sensor)
- MPU-6050 measures cow head PITCH (up/down like nodding) and YAW (left/right like turning head)
- Safe extension requires: PITCH within your custom range AND YAW within your custom range
- ROLL (side tilt like confused head tilt) is ignored - not a clearance issue for door frame
- Calibration triggered by Bottango after positioning all heads straight forward
- When magnet contacts sensor: pin reads LOW, LED on sensor lights up
- 100ms debouncing prevents false triggering from electrical noise

v3.0 OPERATION SEQUENCE:
EXTEND: 
1. Start opening door
2. As soon as door open sensor triggers → immediately start extending cow (overlap)
3. Stop cow motor when cow out sensor triggers

RETRACT:
1. Start retracting cow  
2. As soon as cow in sensor triggers → immediately start closing door (overlap)
3. Stop door motor when door closed sensor triggers
================================================================================
*/

// Include libraries - these add extra functions to the code
#include <Arduino.h>        // Basic Arduino functions
#include <Wire.h>           // I2C communication (for talking to MPU-6050 sensor)
#include <Preferences.h>    // For storing settings permanently (remembers settings after power off)

// MPU-6050 sensor communication addresses - these are like phone numbers for talking to the sensor
#define MPU6050_ADDR 0x68           // Main address of the MPU-6050 sensor
#define MPU6050_PWR_MGMT_1 0x6B     // Address to wake up the sensor
#define MPU6050_ACCEL_XOUT_H 0x3B   // Address to read tilt data
#define MPU6050_GYRO_XOUT_H 0x43    // Address to read rotation data

// State machine states - these track what the system is currently doing
enum cowState
{
  state_Closed_In,    // Door closed, cow completely retracted (starting position)
  doorOpening,        // Door is currently opening (moving down)
  cowExtending,       // Cow is currently extending out through the door
  state_Open_Out,     // Door open, cow fully extended (show position)
  cowRetracting,      // Cow is currently retracting back in
  doorClosing,        // Door is currently closing (moving up)
};

// Timing settings - how long each movement takes (now used mainly for safety timeouts)
const unsigned int doorTransitionTime = 8000;  // 8 seconds for door to fully open or close
const unsigned int cowTransitionTime = 10000;  // 10 seconds for cow to fully extend or retract
unsigned long transitionStartTime = 0;         // Keeps track of when current movement started

// Debouncing settings - prevents false sensor readings from electrical noise
const unsigned int debounceDelay = 100;        // Wait 100ms between sensor readings
unsigned long lastSensorReadTime = 0;          // Keeps track of last sensor check time

// Current state tracking
cowState currentStateCow = state_Closed_In;     // Start in closed/retracted position

// GPIO pin assignments for linear actuator control
const int la_cowRigExtend_Out = 26;   // Pin to extend cow rig outward
const int la_cowRigRetract_In = 25;   // Pin to retract cow rig inward
const int la_doorOpen = 27;           // Pin to open door (move down)
const int la_doorClose = 14;          // Pin to close door (move up)

// GPIO pin assignments for input signals from Impulse Cow boards
const int impulseExtend = 15;     // Signal from Impulse Cow 1&2 board to extend (moved from GPIO 13)
const int impulseRetract = 4;     // Signal from Impulse Cow 1&2 board to retract

// GPIO pin assignments for physical manual override buttons
const int openButton = 32;        // Physical button to manually trigger extend
const int closeButton = 33;       // Physical button to manually trigger retract

// GPIO pin assignments for Hall effect position sensors
const int sensor_cowRigOut = 21;   // Sensor that detects when cow rig is fully extended
const int sensor_cowRigIn = 19;    // Sensor that detects when cow rig is fully retracted
const int sensor_doorOpen = 23;    // Sensor that detects when door is fully open
const int sensor_doorClosed = 22;  // Sensor that detects when door is fully closed

// GPIO pin assignments for MPU-6050 motion sensor
const int MPU_SDA = 16;            // I2C data line for communicating with MPU-6050
const int MPU_SCL = 17;            // I2C clock line for communicating with MPU-6050

// GPIO pin assignment for Bottango calibration trigger
const int calibrationTrigger = 18; // Pin that receives signal from Bottango to start calibration

// GPIO pin assignment for safety LED indicator
const int safetyLED = 2;           // Pin for LED that lights when cow head is unsafe

// Safety limit settings - these define how far the cow head can safely move
float maxSafePitchUp = 20.0;     // Maximum safe up angle (positive degrees)
float maxSafePitchDown = -20.0;  // Maximum safe down angle (negative degrees)
float maxSafeYawLeft = -20.0;    // Maximum safe left angle (negative degrees)  
float maxSafeYawRight = 20.0;    // Maximum safe right angle (positive degrees)

// System control flags - these turn features on/off
bool bypassMonitors = true;          // If true, ignores all safety sensors (v3.0: changed default to false)
bool verboseMode = false;             // If true, shows detailed sensor status constantly
bool eventOnlyMode = true;            // If true, only shows sensor events when they happen
bool disableImpulseCow = true;        // Set to true to disable by default (use EnableImpulse command to activate)
bool debugImpulseCow = true;          // Enable debugging of Impulse Cow signals

//FOR TESTING *****************************************************
//*****************************************************************
bool phantomMonitorMode = false;        // Quiet mode - only shows phantom triggers
//************************************************************************************

// Impulse Cow signal filtering variables
unsigned long lastImpulseExtendTime = 0;
unsigned long lastImpulseRetractTime = 0;
const unsigned long impulseDebounceTime = 2000; // 2 second minimum between triggers
bool lastImpulseExtendState = false;  // Track previous state (LOW = default, HIGH = triggered)
bool lastImpulseRetractState = false; // Track previous state (LOW = default, HIGH = triggered)

// Physical button debouncing variables
unsigned long lastButtonExtendTime = 0;
unsigned long lastButtonRetractTime = 0;
const unsigned long buttonDebounceTime = 500; // 500ms minimum between button presses

// Serial command processing variables
String inputCommand = "";             // Stores the command you type in the terminal
bool commandMode = false;             // If true, manual commands override automatic operation

// Sensor event tracking - prevents duplicate messages
bool lastCowOutState = false;         // Remembers last state of cow-out sensor
bool lastCowInState = false;          // Remembers last state of cow-in sensor
bool lastDoorOpenState = false;       // Remembers last state of door-open sensor
bool lastDoorClosedState = false;     // Remembers last state of door-closed sensor

// MPU-6050 sensor variables
bool mpu6050Available = false;        // True if MPU-6050 sensor is working
float cow1Pitch = 0.0;                // Current up/down tilt angle of cow head (like nodding)
float cow1Yaw = 0.0;                  // Current left/right rotation angle of cow head (like turning)
float cow1Roll = 0.0;                 // Current side tilt angle of cow head (ignored for safety)

// Gyroscope calibration variables - needed for accurate yaw (rotation) measurement
float gyroZOffset = 0.0;              // Calibration offset to remove sensor drift
float pitchOffset = 0.0;              // Calibration offset to zero pitch at straight position
unsigned long lastGyroTime = 0;       // Timestamp for calculating rotation changes
bool gyroCalibrated = false;          // True when gyroscope has been calibrated

// Permanent storage object - remembers settings even after power off
Preferences preferences;

void setup()
{
  // Initialize serial communication at 9600 baud rate
  Serial.begin(9600);

  // Initialize I2C for MPU-6050
  Wire.begin(MPU_SDA, MPU_SCL);
  
  // Set calibration trigger pin as input
  pinMode(calibrationTrigger, INPUT_PULLUP);
  
  // Set safety LED pin as output
  pinMode(safetyLED, OUTPUT);
  digitalWrite(safetyLED, LOW); // Start with LED off (safe)
  
  // Initialize MPU-6050
  // v3.0: MPU-6050 DISABLED - comment out the initialization
  /*
  if (initMPU6050()) {
    Serial.println("MPU-6050 Cow #1 initialized successfully");
    Serial.println("WAITING for Bottango calibration trigger...");
    Serial.println("1. Use Bottango to position all cow heads straight ahead");
    Serial.println("2. Trigger Impulse Cow 3&4 Board Pin 32 to start calibration");
    mpu6050Available = true;
  } else {
    Serial.println("WARNING: MPU-6050 Cow #1 not found - continuing without angle monitoring");
    mpu6050Available = false;
  }
  */
  
  // v3.0: Force MPU-6050 to be unavailable
  mpu6050Available = false;
  Serial.println("MPU-6050 angle monitoring DISABLED");

  // Set actuator pins as OUTPUT
  pinMode(la_cowRigExtend_Out, OUTPUT);
  pinMode(la_cowRigRetract_In, OUTPUT);
  pinMode(la_doorOpen, OUTPUT);
  pinMode(la_doorClose, OUTPUT);

  // Set Impulse Cow board signal pins as INPUT with internal pull-up resistors
  pinMode(impulseExtend, INPUT_PULLUP);
  pinMode(impulseRetract, INPUT_PULLUP);
  
  // Set physical button pins as INPUT with internal pull-up resistors
  pinMode(openButton, INPUT_PULLUP);
  pinMode(closeButton, INPUT_PULLUP);

  // Set Hall sensor pins as INPUT with internal pull-up resistors
  pinMode(sensor_cowRigOut, INPUT_PULLUP);
  pinMode(sensor_cowRigIn, INPUT_PULLUP);
  pinMode(sensor_doorOpen, INPUT_PULLUP);
  pinMode(sensor_doorClosed, INPUT_PULLUP);

  // Load saved safety limits from permanent storage
  loadSafetyLimits();

  // Print initial state
  Serial.println("System Initialized. Current States: Door - closed, Cow - completely in");
  Serial.println("v3.0 - Sensor-based overlapping movements for faster operation");
  Serial.println("ENHANCED DEBUGGING: Detailed trigger source identification");
  Serial.println("EVENT-ONLY MODE: Type commands - CowOut, CowIn, DoorOpen, DoorClosed, ReadAngle, Calibrate");
  Serial.println("Safety Check: PITCH (up/down) and YAW (left/right) within safe limits");
  Serial.println("Commands stop all automation - use buttons/Impulse Cow signals to resume normal operation");
  Serial.println("Ready for commands...");

  if (bypassMonitors)
  {
    Serial.println("WARNING: Monitors are bypassed");
  }
  
  if (disableImpulseCow)
  {
    Serial.println("NOTICE: Impulse Cow signals disabled - use physical buttons or commands only");
  } else {
    Serial.println("NOTICE: Impulse Cow signals ENABLED with 2-second debouncing");
    if (debugImpulseCow) {
      Serial.println("Enhanced debug mode: Will show detailed trigger source information");
    }
  }
}

// Check for Bottango calibration trigger
void checkCalibrationTrigger() {
  if (mpu6050Available && !gyroCalibrated) {
    if (digitalRead(calibrationTrigger) == HIGH) { // Active HIGH trigger from Bottango
      Serial.println("Bottango calibration trigger detected!");
      calibrateGyroscope();
    }
  }
}

void loop()
{

 
  // Handle serial commands
  handleSerialCommands();

  // Check for button/Solar inputs first (works in both auto and command modes)
  bool shouldExtend = false;
  bool shouldRetract = false;

  // Only check Impulse Cow signals if not disabled
  if (!disableImpulseCow) {
    bool currentExtendState = digitalRead(impulseExtend);
    bool currentRetractState = digitalRead(impulseRetract);
    
    // Debug: Show pin state changes with detailed info
    if (debugImpulseCow) {
      if (currentExtendState != lastImpulseExtendState) {
        Serial.println("━━━ IMPULSE EXTEND PIN CHANGE ━━━");
        Serial.println("  Pin: GPIO " + String(impulseExtend));
        Serial.println("  Changed to: " + String(currentExtendState ? "HIGH" : "LOW"));
        Serial.println("  Time: " + String(millis()) + "ms");
        Serial.println("  Last trigger: " + String(millis() - lastImpulseExtendTime) + "ms ago");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        lastImpulseExtendState = currentExtendState;
      }
      if (currentRetractState != lastImpulseRetractState) {
        Serial.println("━━━ IMPULSE RETRACT PIN CHANGE ━━━");
        Serial.println("  Pin: GPIO " + String(impulseRetract));
        Serial.println("  Changed to: " + String(currentRetractState ? "HIGH" : "LOW"));
        Serial.println("  Time: " + String(millis()) + "ms");
        Serial.println("  Last trigger: " + String(millis() - lastImpulseRetractTime) + "ms ago");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        lastImpulseRetractState = currentRetractState;
      }
    }
    
    // Check for extend signal with debouncing (Bottango sends HIGH when triggered)
    if (currentExtendState == HIGH && millis() - lastImpulseExtendTime > impulseDebounceTime)
    {
      Serial.println("🚀 EXTEND TRIGGERED 🚀");
      Serial.println("  SOURCE: Impulse Cow 1&2 Board");
      Serial.println("  PIN: GPIO " + String(impulseExtend));
      Serial.println("  SIGNAL: HIGH (Bottango active)");
      Serial.println("  DEBOUNCE: " + String(millis() - lastImpulseExtendTime) + "ms since last");
      Serial.println("  ACTION: Starting extend sequence");
      Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
      
      shouldExtend = true;
      commandMode = false; // Exit command mode for automation
      lastImpulseExtendTime = millis();
    }

    // Check for retract signal with debouncing (Bottango sends HIGH when triggered)
    if (currentRetractState == HIGH && millis() - lastImpulseRetractTime > impulseDebounceTime)
    {
      Serial.println("🔄 RETRACT TRIGGERED 🔄");
      Serial.println("  SOURCE: Impulse Cow 1&2 Board");
      Serial.println("  PIN: GPIO " + String(impulseRetract));
      Serial.println("  SIGNAL: HIGH (Bottango active)");
      Serial.println("  DEBOUNCE: " + String(millis() - lastImpulseRetractTime) + "ms since last");
      Serial.println("  ACTION: Starting retract sequence");
      Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
      
      shouldRetract = true;
      commandMode = false; // Exit command mode for automation
      lastImpulseRetractTime = millis();
    }
  }
//FOR TESTING***********************************************
//***********************************************************
// Physical buttons always work (with phantom monitoring)
bool openButtonPressed = digitalRead(openButton) == LOW;
bool closeButtonPressed = digitalRead(closeButton) == LOW;

///test****************
// Debug the close button logic
if (closeButtonPressed) {
  Serial.println("DEBUG: closeButtonPressed is TRUE");
  Serial.println("DEBUG: Current time: " + String(millis()));
  Serial.println("DEBUG: lastButtonRetractTime: " + String(lastButtonRetractTime));
  Serial.println("DEBUG: Time since last: " + String(millis() - lastButtonRetractTime));
  Serial.println("DEBUG: Debounce needed: " + String(buttonDebounceTime));
  Serial.println("DEBUG: Should trigger: " + String(millis() - lastButtonRetractTime > buttonDebounceTime ? "YES" : "NO"));
}

 // Temporary debug - add this in your main loop
static unsigned long lastDebug = 0;
if (millis() - lastDebug > 1000) {  // Every second
  Serial.println("GPIO 33 state: " + String(digitalRead(closeButton) ? "HIGH" : "LOW"));
  lastDebug = millis();
} //*********************

if (openButtonPressed && millis() - lastButtonExtendTime > buttonDebounceTime)
{
  if (!phantomMonitorMode) {
    Serial.println("🔘 EXTEND TRIGGERED 🔘");
    Serial.println("  SOURCE: Physical Button");
    Serial.println("  PIN: GPIO " + String(openButton));
    Serial.println("  SIGNAL: LOW (button pressed)");
    Serial.println("  DEBOUNCE: " + String(millis() - lastButtonExtendTime) + "ms since last");
    Serial.println("  ACTION: Starting extend sequence");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  } else {
    // In phantom monitor mode - only show phantom triggers
    Serial.println("⚠️ PHANTOM BUTTON TRIGGER ⚠️");
    Serial.println("  Time: " + String(millis()) + "ms");
    Serial.println("  GPIO 32 read LOW - If you didn't press button, THIS IS THE PROBLEM!");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  }
  
  shouldExtend = true;
  commandMode = false;
  lastButtonExtendTime = millis();
}
if (closeButtonPressed && millis() - lastButtonRetractTime > buttonDebounceTime)
{
  if (!phantomMonitorMode) {
    Serial.println("🔘 RETRACT TRIGGERED 🔘");
    Serial.println("  SOURCE: Physical Button");
    Serial.println("  PIN: GPIO " + String(closeButton));
    Serial.println("  SIGNAL: LOW (button pressed)");
    Serial.println("  DEBOUNCE: " + String(millis() - lastButtonRetractTime) + "ms since last");
    Serial.println("  ACTION: Starting retract sequence");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  } else {
    // In phantom monitor mode - only show phantom triggers
    Serial.println("⚠️ PHANTOM BUTTON TRIGGER ⚠️");
    Serial.println("  Time: " + String(millis()) + "ms");
    Serial.println("  GPIO 33 read LOW - If you didn't press button, THIS IS THE PROBLEM!");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  }
  
  shouldRetract = true;
  commandMode = false;
  lastButtonRetractTime = millis();
}
//*****************************************************************
//*** re-enable below when done ************************************
  // // Physical buttons always work (with enhanced debugging)
  // bool openButtonPressed = digitalRead(openButton) == LOW;
  // bool closeButtonPressed = digitalRead(closeButton) == LOW;
  
  // if (openButtonPressed && millis() - lastButtonExtendTime > buttonDebounceTime)
  // {
  //   Serial.println("🔘 EXTEND TRIGGERED 🔘");
  //   Serial.println("  SOURCE: Physical Button");
  //   Serial.println("  PIN: GPIO " + String(openButton));
  //   Serial.println("  SIGNAL: LOW (button pressed)");
  //   Serial.println("  DEBOUNCE: " + String(millis() - lastButtonExtendTime) + "ms since last");
  //   Serial.println("  ACTION: Starting extend sequence");
  //   Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
  //   shouldExtend = true;
  //   commandMode = false; // Exit command mode for automation
  //   lastButtonExtendTime = millis();
  // }
  
  // if (closeButtonPressed && millis() - lastButtonRetractTime > buttonDebounceTime)
  // {
  //   Serial.println("🔘 RETRACT TRIGGERED 🔘");
  //   Serial.println("  SOURCE: Physical Button");
  //   Serial.println("  PIN: GPIO " + String(closeButton));
  //   Serial.println("  SIGNAL: LOW (button pressed)");
  //   Serial.println("  DEBOUNCE: " + String(millis() - lastButtonRetractTime) + "ms since last");
  //   Serial.println("  ACTION: Starting retract sequence");
  //   Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
  //   shouldRetract = true;
  //   commandMode = false; // Exit command mode for automation
  //   lastButtonRetractTime = millis();
  // }

  // Enhanced debugging: Show current pin states periodically (every 5 seconds)
  //static unsigned long lastStatusReport = 0;
  //if (debugImpulseCow && millis() - lastStatusReport > 5000) {
  // Enhanced debugging: Show current pin states periodically (every 5 seconds) - NOT in phantom monitor mode
static unsigned long lastStatusReport = 0;
if (debugImpulseCow && !phantomMonitorMode && millis() - lastStatusReport > 5000) {
  
    Serial.println("📊 CURRENT PIN STATUS REPORT 📊");
    Serial.println("  GPIO " + String(impulseExtend) + " (extend): " + String(digitalRead(impulseExtend) ? "HIGH" : "LOW"));
    Serial.println("  GPIO " + String(impulseRetract) + " (retract): " + String(digitalRead(impulseRetract) ? "HIGH" : "LOW"));
    Serial.println("  GPIO " + String(openButton) + " (open btn): " + String(digitalRead(openButton) ? "HIGH" : "LOW"));
    Serial.println("  GPIO " + String(closeButton) + " (close btn): " + String(digitalRead(closeButton) ? "HIGH" : "LOW"));
    Serial.println("  Impulse signals: " + String(!disableImpulseCow ? "ENABLED" : "DISABLED"));
    Serial.println("  Debug mode: " + String(debugImpulseCow ? "ON" : "OFF"));
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    lastStatusReport = millis();
  }

  // If in command mode and no buttons pressed, skip automation but still monitor sensors
  if (commandMode) {
    checkSensorEvents();
    checkCalibrationTrigger(); // Check for Bottango calibration trigger
    return;
  }

  // Check for Bottango calibration trigger
  checkCalibrationTrigger();

  // CORRECTED: A3144 Hall sensors are actually active LOW when magnet present
  // Debounced sensor reading
  bool cowOutSensorActive = false;
  bool cowInSensorActive = false;
  bool doorOpenSensorActive = false;
  bool doorClosedSensorActive = false;

  // Read sensors with debouncing
  if (millis() - lastSensorReadTime >= debounceDelay)
  {
    cowOutSensorActive = !digitalRead(sensor_cowRigOut);      // Active LOW (magnet present)
    cowInSensorActive = !digitalRead(sensor_cowRigIn);        // Active LOW (magnet present)
    doorOpenSensorActive = !digitalRead(sensor_doorOpen);     // Active LOW (magnet present)
    doorClosedSensorActive = !digitalRead(sensor_doorClosed); // Active LOW (magnet present)
    
    lastSensorReadTime = millis();
    
    // Event-only sensor reporting - only report changes
    if (eventOnlyMode) {
      if (cowOutSensorActive && !lastCowOutState) {
        Serial.println("sensor_cowRigOut contacted");
        lastCowOutState = true;
      } else if (!cowOutSensorActive && lastCowOutState) {
        lastCowOutState = false;
      }
      
      if (cowInSensorActive && !lastCowInState) {
        Serial.println("sensor_cowRigIn contacted");
        lastCowInState = true;
      } else if (!cowInSensorActive && lastCowInState) {
        lastCowInState = false;
      }
      
      if (doorOpenSensorActive && !lastDoorOpenState) {
        Serial.println("sensor_doorOpen contacted");
        lastDoorOpenState = true;
      } else if (!doorOpenSensorActive && lastDoorOpenState) {
        lastDoorOpenState = false;
      }
      
      if (doorClosedSensorActive && !lastDoorClosedState) {
        Serial.println("sensor_doorClosed contacted");
        lastDoorClosedState = true;
      } else if (!doorClosedSensorActive && lastDoorClosedState) {
        lastDoorClosedState = false;
      }
    }
    
    // Verbose sensor monitoring (if enabled)
    if (verboseMode)
    {
      if (cowOutSensorActive) Serial.println("SENSOR: Cow Out - ACTIVE");
      if (cowInSensorActive) Serial.println("SENSOR: Cow In - ACTIVE");
      if (doorOpenSensorActive) Serial.println("SENSOR: Door Open - ACTIVE");
      if (doorClosedSensorActive) Serial.println("SENSOR: Door Closed - ACTIVE");
    }
  }

  // State Machine - v3.0: Now with sensor-based transitions for overlapping movements
  switch (currentStateCow)
  {
    case state_Closed_In:
      if (shouldExtend)
      {
        currentStateCow = doorOpening;
        Serial.println("Door transitioning down");
        transitionStartTime = millis();
        digitalWrite(la_doorOpen, HIGH);
        digitalWrite(la_doorClose, LOW);
        digitalWrite(la_cowRigExtend_Out, LOW);
        digitalWrite(la_cowRigRetract_In, LOW);
      }
      break;

    case doorOpening:
      // v3.0: Start extending cow immediately when door open sensor triggers
      if (doorOpenSensorActive)
      {
        // Check cow head angle before extending (if MPU available and calibrated)
        if (mpu6050Available && gyroCalibrated && !isCowHeadSafe()) {
          Serial.println("WARNING: Cow #1 head angle unsafe - aborting extension");
          Serial.println("Pitch: " + String(cow1Pitch) + "° Yaw: " + String(cow1Yaw) + "°");
          // Return to closed state
          currentStateCow = doorClosing;
          digitalWrite(la_doorOpen, LOW);
          digitalWrite(la_doorClose, HIGH); // Start closing door
          break;
        }
        
        currentStateCow = cowExtending;
        Serial.println("Door open detected - Cow transitioning out (overlapping)");
        transitionStartTime = millis();
        digitalWrite(la_doorOpen, LOW);  // Stop door motor
        digitalWrite(la_doorClose, LOW);
        digitalWrite(la_cowRigExtend_Out, HIGH);  // Start extending cow immediately
        digitalWrite(la_cowRigRetract_In, LOW);
      }
      // Safety timeout - if door doesn't open within expected time + 2 seconds
      else if (millis() - transitionStartTime >= doorTransitionTime + 2000)
      {
        Serial.println("ERROR: Door open timeout - stopping");
        currentStateCow = state_Closed_In;
        digitalWrite(la_doorOpen, LOW);
        digitalWrite(la_doorClose, LOW);
      }
      break;

    case cowExtending:
      // Check if cow is fully extended
      if (cowOutSensorActive)
      {
        currentStateCow = state_Open_Out;
        Serial.println("Cow fully extended");
        digitalWrite(la_cowRigExtend_Out, LOW);
      }
      // Safety timeout
      else if (millis() - transitionStartTime >= cowTransitionTime + 2000)
      {
        Serial.println("ERROR: Cow extend timeout - stopping");
        currentStateCow = state_Open_Out;
        digitalWrite(la_cowRigExtend_Out, LOW);
      }
      break;

    case state_Open_Out:
      if (shouldRetract)
      {
        currentStateCow = cowRetracting;
        Serial.println("Cow transitioning in");
        transitionStartTime = millis();
        digitalWrite(la_cowRigExtend_Out, LOW);
        digitalWrite(la_cowRigRetract_In, HIGH);
      }
      break;

    case cowRetracting:
      // v3.0: Start closing door immediately when cow in sensor triggers
      if (cowInSensorActive)
      {
        currentStateCow = doorClosing;
        Serial.println("Cow in detected - Door transitioning up (overlapping)");
        transitionStartTime = millis();
        digitalWrite(la_cowRigRetract_In, LOW);  // Stop cow motor
        digitalWrite(la_doorClose, HIGH);  // Start closing door immediately
      }
      // Safety timeout
      else if (millis() - transitionStartTime >= cowTransitionTime + 2000)
      {
        Serial.println("ERROR: Cow retract timeout - closing door anyway");
        currentStateCow = doorClosing;
        digitalWrite(la_cowRigRetract_In, LOW);
        digitalWrite(la_doorClose, HIGH);
        transitionStartTime = millis();
      }
      break;

    case doorClosing:
      // Check if door is fully closed
      if (doorClosedSensorActive)
      {
        currentStateCow = state_Closed_In;
        Serial.println("Door fully closed");
        digitalWrite(la_doorClose, LOW);
      }
      // Safety timeout
      else if (millis() - transitionStartTime >= doorTransitionTime + 2000)
      {
        Serial.println("ERROR: Door close timeout - stopping");
        currentStateCow = state_Closed_In;
        digitalWrite(la_doorClose, LOW);
      }
      break;
  }
}

// Handle serial commands for manual testing
void handleSerialCommands() {
  while (Serial.available() > 0) {
    char inChar = Serial.read();
    
    if (inChar == '\n' || inChar == '\r') {
      if (inputCommand.length() > 0) {
        processCommand(inputCommand);
        inputCommand = "";
      }
    } else {
      inputCommand += inChar;
    }
  }
}

// Process manual commands
void processCommand(String command) {
  command.trim();
  command.toLowerCase();
  
  // Stop all actuators first
  digitalWrite(la_cowRigExtend_Out, LOW);
  digitalWrite(la_cowRigRetract_In, LOW);
  digitalWrite(la_doorOpen, LOW);
  digitalWrite(la_doorClose, LOW);
  
  if (command == "cowout") {
    Serial.println("COMMAND: Cow extending out");
    digitalWrite(la_cowRigExtend_Out, HIGH);
    commandMode = true;
  }
  //FOR TESTING****************************
  //***************************************
  else if (command == "phantommonitor") {
  phantomMonitorMode = !phantomMonitorMode;
  Serial.println("Phantom Monitor Mode " + String(phantomMonitorMode ? "ENABLED" : "DISABLED"));
  if (phantomMonitorMode) {
    Serial.println("Quiet mode - only phantom triggers will show");
  }
}
//*****************************************************
//************************************************
  else if (command == "cowin") {
    Serial.println("COMMAND: Cow retracting in");
    digitalWrite(la_cowRigRetract_In, HIGH);
    commandMode = true;
  }
  else if (command == "dooropen") {
    Serial.println("COMMAND: Door opening (down)");
    digitalWrite(la_doorOpen, HIGH);
    commandMode = true;
  }
  else if (command == "doorclosed") {
    Serial.println("COMMAND: Door closing (up)");
    digitalWrite(la_doorClose, HIGH);
    commandMode = true;
  }
  else if (command == "readangle") {
    if (mpu6050Available) {
      if (gyroCalibrated) {
        updateCowAngles();
        Serial.println("Cow #1 Angles:");
        Serial.println("  Pitch: " + String(cow1Pitch) + "° (up/down tilt)");
        Serial.println("  Yaw: " + String(cow1Yaw) + "° (left/right rotation)");
        Serial.println("  Roll: " + String(cow1Roll) + "° (side tilt - ignored)");
        
        // Debug output for safety checking
        Serial.println("Safety Check Details:");
        Serial.println("  Pitch in range: " + String(cow1Pitch >= maxSafePitchDown && cow1Pitch <= maxSafePitchUp ? "YES" : "NO"));
        Serial.println("  Yaw in range: " + String(cow1Yaw >= maxSafeYawLeft && cow1Yaw <= maxSafeYawRight ? "YES" : "NO"));
        
        if (isCowHeadSafe()) {
          Serial.println("  Overall Status: SAFE to extend (LED OFF)");
        } else {
          Serial.println("  Overall Status: UNSAFE - pitch or yaw out of range (LED ON)");
        }
      } else {
        Serial.println("MPU-6050 not calibrated yet - waiting for Bottango trigger");
      }
    } else {
      Serial.println("MPU-6050 not available");
    }
  }
  else if (command == "calibrate") {
    if (mpu6050Available) {
      Serial.println("Manual calibration started - keep cow head straight!");
      calibrateGyroscope();
    } else {
      Serial.println("MPU-6050 not available");
    }
  }
  else if (command == "zeroyaw") {
    if (mpu6050Available) {
      cow1Yaw = 0.0;  // Reset yaw to zero at current position
      Serial.println("Yaw reset to 0° at current position");
    } else {
      Serial.println("MPU-6050 not available");
    }
  }
  else if (command == "setmaxup") {
    if (mpu6050Available) {
      updateCowAngles();
      maxSafePitchUp = cow1Pitch;
      saveSafetyLimits();  // Save to permanent storage
      Serial.println("Max safe UP position set to: " + String(maxSafePitchUp) + "° (SAVED)");
    } else {
      Serial.println("MPU-6050 not available");
    }
  }
  else if (command == "setmaxdown") {
    if (mpu6050Available) {
      updateCowAngles();
      maxSafePitchDown = cow1Pitch;
      saveSafetyLimits();  // Save to permanent storage
      Serial.println("Max safe DOWN position set to: " + String(maxSafePitchDown) + "° (SAVED)");
    } else {
      Serial.println("MPU-6050 not available");
    }
  }
  else if (command == "setmaxleft") {
    if (mpu6050Available) {
      updateCowAngles();
      maxSafeYawLeft = cow1Yaw;
      saveSafetyLimits();  // Save to permanent storage
      Serial.println("Max safe LEFT position set to: " + String(maxSafeYawLeft) + "° (SAVED)");
    } else {
      Serial.println("MPU-6050 not available");
    }
  }
  else if (command == "setmaxright") {
    if (mpu6050Available) {
      updateCowAngles();
      maxSafeYawRight = cow1Yaw;
      saveSafetyLimits();  // Save to permanent storage
      Serial.println("Max safe RIGHT position set to: " + String(maxSafeYawRight) + "° (SAVED)");
    } else {
      Serial.println("MPU-6050 not available");
    }
  }
  else if (command == "showlimits") {
    Serial.println("Current Safety Limits:");
    Serial.println("  Pitch UP max: " + String(maxSafePitchUp) + "°");
    Serial.println("  Pitch DOWN max: " + String(maxSafePitchDown) + "°");
    Serial.println("  Yaw LEFT max: " + String(maxSafeYawLeft) + "°");
    Serial.println("  Yaw RIGHT max: " + String(maxSafeYawRight) + "°");
    
    // Verify limits make sense
    if (maxSafePitchUp <= maxSafePitchDown) {
      Serial.println("  WARNING: Pitch limits corrupted! UP should be > DOWN");
    }
    if (maxSafeYawLeft >= maxSafeYawRight) {
      Serial.println("  WARNING: Yaw limits corrupted! LEFT should be < RIGHT");
    }
  }
  else if (command == "resetlimits") {
    Serial.println("Resetting safety limits to defaults...");
    
    // Clear any corrupted stored values first
    preferences.begin("cowlimits", false);
    preferences.clear();
    preferences.end();
    
    // Set correct default values
    maxSafePitchUp = 20.0;
    maxSafePitchDown = -20.0;
    maxSafeYawLeft = -20.0;
    maxSafeYawRight = 20.0;
    
    // Save the correct defaults
    saveSafetyLimits();
    
    Serial.println("Safety limits RESET and SAVED:");
    Serial.println("  Pitch UP: " + String(maxSafePitchUp) + "°");
    Serial.println("  Pitch DOWN: " + String(maxSafePitchDown) + "°");
    Serial.println("  Yaw LEFT: " + String(maxSafeYawLeft) + "°");
    Serial.println("  Yaw RIGHT: " + String(maxSafeYawRight) + "°");
    Serial.println("Limits cleared from storage and reset to working defaults.");
    
    // Test the safety system immediately if MPU is available
    if (mpu6050Available && gyroCalibrated) {
      updateCowAngles();
      Serial.println("Testing safety system with current head position:");
      Serial.println("  Current Pitch: " + String(cow1Pitch) + "°");
      Serial.println("  Current Yaw: " + String(cow1Yaw) + "°");
      if (isCowHeadSafe()) {
        Serial.println("  Status: SAFE - limits working correctly!");
      } else {
        Serial.println("  Status: UNSAFE - head outside new limits");
      }
    }
  }
  else if (command == "stop") {
    Serial.println("COMMAND: All actuators stopped");
    commandMode = false;
  }
  else if (command == "enableimpulse") {
    disableImpulseCow = false;
    Serial.println("Impulse Cow signals ENABLED with 2-second debouncing");
    Serial.println("Monitoring GPIO " + String(impulseExtend) + " (extend) and GPIO " + String(impulseRetract) + " (retract)");
    Serial.println("Enhanced debugging will show detailed trigger information");
  }
  else if (command == "disableimpulse") {
    disableImpulseCow = true;
    Serial.println("Impulse Cow signals DISABLED");
  }
  else if (command == "debugimpulse") {
    debugImpulseCow = !debugImpulseCow;
    Serial.println("Impulse Cow debug mode " + String(debugImpulseCow ? "ENABLED" : "DISABLED"));
    if (debugImpulseCow) {
      Serial.println("Will show detailed pin state changes and trigger sources");
      Serial.println("Status reports every 5 seconds when enabled");
    }
  }
  else if (command == "checkimpulse") {
    Serial.println("🔍 DETAILED IMPULSE SYSTEM STATUS 🔍");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("PIN ASSIGNMENTS:");
    Serial.println("  Extend pin: GPIO " + String(impulseExtend));
    Serial.println("  Retract pin: GPIO " + String(impulseRetract));
    Serial.println("");
    Serial.println("CURRENT PIN STATES:");
    Serial.println("  GPIO " + String(impulseExtend) + " (extend): " + String(digitalRead(impulseExtend) ? "HIGH" : "LOW"));
    Serial.println("  GPIO " + String(impulseRetract) + " (retract): " + String(digitalRead(impulseRetract) ? "HIGH" : "LOW"));
    Serial.println("");
    Serial.println("SYSTEM STATUS:");
    Serial.println("  Impulse signals: " + String(!disableImpulseCow ? "ENABLED" : "DISABLED"));
    Serial.println("  Debug mode: " + String(debugImpulseCow ? "ON" : "OFF"));
    Serial.println("  Debounce time: " + String(impulseDebounceTime) + "ms");
    Serial.println("");
    Serial.println("LAST TRIGGER TIMES:");
    Serial.println("  Last extend: " + String(millis() - lastImpulseExtendTime) + "ms ago");
    Serial.println("  Last retract: " + String(millis() - lastImpulseRetractTime) + "ms ago");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  }
  else if (command == "help") {
    Serial.println("Available commands:");
    Serial.println("  CowOut - Extend cow rig");
    Serial.println("  CowIn - Retract cow rig");
    Serial.println("  DoorOpen - Open door (down)");
    Serial.println("  DoorClosed - Close door (up)");
    Serial.println("  ReadAngle - Show Cow #1 head angles");
    Serial.println("  Calibrate - Manual gyroscope calibration");
    Serial.println("  ZeroYaw - Reset yaw to 0° at current position");
    Serial.println("  SetMaxUp/Down/Left/Right - Set safety limits at current position");
    Serial.println("  ResetLimits - Reset safety limits to ±20° defaults");
    Serial.println("  ShowLimits - Display current safety boundaries");
    Serial.println("  Stop - Stop all actuators");
    Serial.println("  EnableImpulse/DisableImpulse - Control Impulse Cow signals");
    Serial.println("  DebugImpulse - Toggle enhanced debug mode for Impulse signals");
    Serial.println("  CheckImpulse - Detailed Impulse pin status report");
  }
  else {
    Serial.println("Unknown command. Type 'help' for available commands.");
  }
}

// Check for sensor events (used in both auto and manual modes)
void checkSensorEvents() {
  // Read sensors with debouncing
  if (millis() - lastSensorReadTime >= debounceDelay) {
    bool cowOutSensorActive = !digitalRead(sensor_cowRigOut);      // Active LOW (magnet present)
    bool cowInSensorActive = !digitalRead(sensor_cowRigIn);        // Active LOW (magnet present)
    bool doorOpenSensorActive = !digitalRead(sensor_doorOpen);     // Active LOW (magnet present)
    bool doorClosedSensorActive = !digitalRead(sensor_doorClosed); // Active LOW (magnet present)
    
    lastSensorReadTime = millis();
    
    // Event-only sensor reporting - only report changes
    if (cowOutSensorActive && !lastCowOutState) {
      Serial.println("sensor_cowRigOut contacted");
      lastCowOutState = true;
    } else if (!cowOutSensorActive && lastCowOutState) {
      lastCowOutState = false;
    }
    
    if (cowInSensorActive && !lastCowInState) {
      Serial.println("sensor_cowRigIn contacted");
      lastCowInState = true;
    } else if (!cowInSensorActive && lastCowInState) {
      lastCowInState = false;
    }
    
    if (doorOpenSensorActive && !lastDoorOpenState) {
      Serial.println("sensor_doorOpen contacted");
      lastDoorOpenState = true;
    } else if (!doorOpenSensorActive && lastDoorOpenState) {
      lastDoorOpenState = false;
    }
    
    if (doorClosedSensorActive && !lastDoorClosedState) {
      Serial.println("sensor_doorClosed contacted");
      lastDoorClosedState = true;
    } else if (!doorClosedSensorActive && lastDoorClosedState) {
      lastDoorClosedState = false;
    }
  }
}

// Initialize MPU-6050
bool initMPU6050() {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_PWR_MGMT_1);
  Wire.write(0); // Wake up MPU-6050
  byte error = Wire.endTransmission();
  
  if (error == 0) {
    delay(100); // Give MPU time to wake up
    lastGyroTime = millis(); // Initialize timing for yaw calculation
    return true;
  }
  return false;
}

// Calibrate gyroscope and zero angles at current position (with error handling)
void calibrateGyroscope() {
  if (!mpu6050Available) return;
  
  Serial.println("Calibrating gyroscope... keep cow head straight for 3 seconds");
  Serial.println("This will reset pitch and yaw to zero at current position");
  
  float gyroZSum = 0;
  float pitchSum = 0;
  int validSamples = 0;
  unsigned long startTime = millis();
  
  while (millis() - startTime < 3000) { // 3 second calibration
    bool readSuccess = true;
    
    // Try to read gyroscope for yaw calibration
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(MPU6050_GYRO_XOUT_H);
    if (Wire.endTransmission(false) != 0) {
      readSuccess = false;
    }
    
    int16_t gyroZ = 0;
    if (readSuccess) {
      Wire.requestFrom(MPU6050_ADDR, 6, true);
      if (Wire.available() >= 6) {
        Wire.read(); Wire.read(); // Skip gyroX
        Wire.read(); Wire.read(); // Skip gyroY
        gyroZ = Wire.read() << 8 | Wire.read();
      } else {
        readSuccess = false;
      }
    }
    
    // Try to read accelerometer for pitch calibration
    float currentPitch = 0;
    if (readSuccess) {
      Wire.beginTransmission(MPU6050_ADDR);
      Wire.write(MPU6050_ACCEL_XOUT_H);
      if (Wire.endTransmission(false) != 0) {
        readSuccess = false;
      }
      
      if (readSuccess) {
        Wire.requestFrom(MPU6050_ADDR, 6, true);
        if (Wire.available() >= 6) {
          int16_t accelX = Wire.read() << 8 | Wire.read();
          int16_t accelY = Wire.read() << 8 | Wire.read();
          int16_t accelZ = Wire.read() << 8 | Wire.read();
          currentPitch = atan2(accelY, sqrt(accelX * accelX + accelZ * accelZ)) * 180.0 / PI;
        } else {
          readSuccess = false;
        }
      }
    }
    
    // Only use valid readings
    if (readSuccess) {
      gyroZSum += gyroZ;
      pitchSum += currentPitch;
      validSamples++;
    }
    
    delay(10);
  }
  
  // Set calibration offsets if we got enough valid samples
  if (validSamples > 50) { // Need at least 50 good readings
    gyroZOffset = gyroZSum / validSamples;
    pitchOffset = pitchSum / validSamples;
    gyroCalibrated = true;
    
    // Reset angles to zero immediately
    cow1Pitch = 0.0;
    cow1Yaw = 0.0;
    
    Serial.println("Gyroscope calibration complete");
    Serial.println("Valid samples: " + String(validSamples));
    Serial.println("Pitch and Yaw reset to 0° at current straight position");
  } else {
    Serial.println("Calibration FAILED - too many I2C errors");
    Serial.println("Valid samples: " + String(validSamples) + " (need at least 50)");
    Serial.println("Check MPU-6050 wiring - loose connections?");
  }
}

// Read accelerometer and gyroscope data, calculate angles relative to calibrated zero
void updateCowAngles() {
  if (!mpu6050Available) return;
  
  // Read accelerometer data for pitch and roll
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_ACCEL_XOUT_H);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 6, true);
  
  int16_t accelX = Wire.read() << 8 | Wire.read();
  int16_t accelY = Wire.read() << 8 | Wire.read();
  int16_t accelZ = Wire.read() << 8 | Wire.read();
  
  // Calculate raw pitch and roll from accelerometer
  float rawPitch = atan2(accelY, sqrt(accelX * accelX + accelZ * accelZ)) * 180.0 / PI;
  cow1Roll = atan2(-accelX, accelZ) * 180.0 / PI;
  
  // Apply pitch offset to make calibrated position = 0°
  cow1Pitch = rawPitch - pitchOffset;
  
  // Read gyroscope data for yaw calculation
  if (gyroCalibrated) {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(MPU6050_GYRO_XOUT_H);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6050_ADDR, 6, true);
    
    // Skip X and Y gyro, read Z gyro for yaw
    Wire.read(); Wire.read(); // Skip gyroX
    Wire.read(); Wire.read(); // Skip gyroY
    int16_t gyroZ = Wire.read() << 8 | Wire.read();
    
    // Calculate time difference
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - lastGyroTime) / 1000.0; // Convert to seconds
    lastGyroTime = currentTime;
    
    // Calculate yaw rate and integrate (relative to calibrated zero)
    float gyroZRate = (gyroZ - gyroZOffset) / 131.0; // Convert to degrees/second
    cow1Yaw += gyroZRate * deltaTime;
    
    // Keep yaw within ±180° range
    if (cow1Yaw > 180) cow1Yaw -= 360;
    if (cow1Yaw < -180) cow1Yaw += 360;
  }
}

// Check if cow head is in safe position for extension
bool isCowHeadSafe() {
  if (!mpu6050Available || !gyroCalibrated) {
    digitalWrite(safetyLED, LOW); // LED off when no sensor (assume safe)
    return true; // If no sensor or not calibrated, assume safe
  }
  
  updateCowAngles();
  
  // Check if pitch is within safe range
  bool pitchSafe = (cow1Pitch <= maxSafePitchUp && cow1Pitch >= maxSafePitchDown);
  
  // Check if yaw is within safe range  
  bool yawSafe = (cow1Yaw >= maxSafeYawLeft && cow1Yaw <= maxSafeYawRight);
  
  bool headSafe = (pitchSafe && yawSafe);
  
  // Control safety LED - ON when UNSAFE, OFF when safe
  digitalWrite(safetyLED, !headSafe);
  
  return headSafe;
}

// Save safety limits to permanent storage
void saveSafetyLimits() {
  preferences.begin("cowlimits", false);
  preferences.putFloat("pitchUp", maxSafePitchUp);
  preferences.putFloat("pitchDown", maxSafePitchDown);
  preferences.putFloat("yawLeft", maxSafeYawLeft);
  preferences.putFloat("yawRight", maxSafeYawRight);
  preferences.end();
  
  Serial.println("Safety limits saved to permanent storage.");
}

// Load safety limits from permanent storage
void loadSafetyLimits() {
  preferences.begin("cowlimits", true);
  
  // Load values with proper defaults
  float loadedPitchUp = preferences.getFloat("pitchUp", 20.0);
  float loadedPitchDown = preferences.getFloat("pitchDown", -20.0);
  float loadedYawLeft = preferences.getFloat("yawLeft", -20.0);
  float loadedYawRight = preferences.getFloat("yawRight", 20.0);
  
  preferences.end();
  
  // Validate loaded limits before using them
  bool limitsValid = true;
  
  if (loadedPitchUp <= loadedPitchDown) {
    Serial.println("ERROR: Stored pitch limits corrupted (UP <= DOWN)");
    limitsValid = false;
  }
  
  if (loadedYawLeft >= loadedYawRight) {
    Serial.println("ERROR: Stored yaw limits corrupted (LEFT >= RIGHT)");
    limitsValid = false;
  }
  
  if (limitsValid) {
    // Use loaded values
    maxSafePitchUp = loadedPitchUp;
    maxSafePitchDown = loadedPitchDown;
    maxSafeYawLeft = loadedYawLeft;
    maxSafeYawRight = loadedYawRight;
    
    Serial.println("Loaded valid safety limits from memory:");
  } else {
    // Use safe defaults
    maxSafePitchUp = 20.0;
    maxSafePitchDown = -20.0;
    maxSafeYawLeft = -20.0;
    maxSafeYawRight = 20.0;
    
    Serial.println("Using default safety limits (stored values corrupted):");
  }
  
  Serial.println("  Pitch UP: " + String(maxSafePitchUp) + "°");
  Serial.println("  Pitch DOWN: " + String(maxSafePitchDown) + "°");
  Serial.println("  Yaw LEFT: " + String(maxSafeYawLeft) + "°");
  Serial.println("  Yaw RIGHT: " + String(maxSafeYawRight) + "°");
  
  if (!limitsValid) {
    Serial.println("Type 'ResetLimits' to save correct defaults to storage.");
  }
}
