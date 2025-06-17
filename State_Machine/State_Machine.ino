// Cow Externer COde 1.3  7/6/24 12:00PM
#include <Arduino.h>

// Define the states for the state machine

enum cowState
{
  closedAndIn,
  doorOpening,
  cowExtending,
  openAndOut,
  cowRetracting,
  doorClosing,
};

// time tracking
const unsigned int doorTransitionTime = 4000; // time it takes the door to fully open or close in milliseconds
const unsigned int cowTransitionTime = 4000; // time it takes to fully move the cows in milliseconds
unsigned long transitionStartTime = 0;        // variable to keep track of time

// Initialize the state
cowState currentStateCow = closedAndIn;

// Pin definitions for the actuators and buttons
const int cowExtendPin = 9;   // BTS7960 LPWM
const int cowRetractPin = 10; // BTS7960 RPWM
const int doorExtendPin = 11;
const int doorRetractPin = 12;

// Pin definitions for the buttons
const int cowExtendButton = 2;
const int cowRetractButton = 3;

// Pin definitions for the reed switch
const int cowExtendedMonitor = 5;
const int cowRetractedMonitor = 6;
const int doorExtendedMonitor = 7;
const int doorRetractedMonitor = 8;

// debug vars
bool bypassMonitors = true;

void setup()
{
  // Initialize serial communication at 9600 baud rate
  Serial.begin(9600);

  // Set actuator pins as OUTPUT
  pinMode(cowExtendPin, OUTPUT);
  pinMode(cowRetractPin, OUTPUT);
  pinMode(doorExtendPin, OUTPUT);
  pinMode(doorRetractPin, OUTPUT);

  // Set button pins as INPUT with internal pull-up resistors
  pinMode(cowExtendButton, INPUT_PULLUP);
  pinMode(cowRetractButton, INPUT_PULLUP);

  // Set reed switches pins as INPUT with internal pull-up resistors
  pinMode(cowExtendedMonitor, INPUT_PULLUP);
  pinMode(cowRetractedMonitor, INPUT_PULLUP);
  pinMode(doorExtendedMonitor, INPUT_PULLUP);
  pinMode(doorRetractedMonitor, INPUT_PULLUP);

  // Print initial state
  Serial.println("System Initialized. Current States: Door - closed, Cow - completely in");

  if (bypassMonitors)
  {
    Serial.println("WARNING: Monitors are bypassed");
  }
}

void loop()
{
  // Read button states (active LOW, so invert the reading)
  bool cowExtendButtonPressed = !digitalRead(cowExtendButton);
  bool cowRetractButtonPressed = !digitalRead(cowRetractButton);

  // Read monitor states (active LOW, so invert the reading)
  bool cowExtendMonitorActive = !digitalRead(cowExtendedMonitor);
  bool cowRetractedMonitorActive = !digitalRead(cowRetractedMonitor);
  bool doorExtendedMonitorActive = !digitalRead(doorExtendedMonitor);
  bool doorRetractedMonitorActive = !digitalRead(doorRetractedMonitor);

  // State machine for Door_Open_Shut
  switch (currentStateCow)
  {
  case closedAndIn:
    if (cowExtendButtonPressed)
    {
      // move to the next state
      currentStateCow = doorOpening;
      Serial.println("Cow doors state changed to: opening");

      // restart the stopwatch
      transitionStartTime = millis();

      // start opening the door
      digitalWrite(doorExtendPin, HIGH);
      digitalWrite(doorRetractPin, LOW);
      digitalWrite(cowExtendPin, LOW);
      digitalWrite(cowRetractPin, LOW);
    }
    break;

  case doorOpening:
    // enough time has passed?
    if (millis() - transitionStartTime >= doorTransitionTime)
    {
      if (bypassMonitors || doorExtendedMonitorActive)
      {
        if (bypassMonitors)
        {
          Serial.println("WARNING: Monitors are bypassed");
        }

        Serial.println("Cow doors transition time complete, and monitor active. Cows are now extending");
        // move to the next state
        currentStateCow = cowExtending;

        // restart the stopwatch
        transitionStartTime = millis();

        // start moving the cows
        digitalWrite(doorExtendPin, LOW);
        digitalWrite(doorRetractPin, LOW);
        digitalWrite(cowExtendPin, HIGH);
        digitalWrite(cowRetractPin, LOW);
      }
      else
      {
        // keep waiting for the monitor
        Serial.println("Cow doors open transition time complete, still waiting for monitor...");
      }
    }
    else
    {
      Serial.print("opening doors. ");
      Serial.print(millis() - transitionStartTime);
      Serial.print(" / ");
      Serial.println(doorTransitionTime);
    }
    break;

  case cowExtending:
    // enough time has passed?
    if (millis() - transitionStartTime >= cowTransitionTime)
    {
      if (bypassMonitors || cowExtendMonitorActive)
      {
        if (bypassMonitors)
        {
          Serial.println("WARNING: Monitors are bypassed");
        }

        Serial.println("Cow extension transition time complete, and monitor active. Cows are now fully extended");
        // move to the next state
        currentStateCow = openAndOut;

        // start moving the cows
        digitalWrite(doorExtendPin, LOW);
        digitalWrite(doorRetractPin, LOW);
        digitalWrite(cowExtendPin, LOW);
        digitalWrite(cowRetractPin, LOW);
      }
      else
      {
        // keep waiting for the monitor
        Serial.println("Cow extension transition time complete, still waiting for monitor...");
      }
    }
    else
    {
      Serial.print("extending cows. ");
      Serial.print(millis() - transitionStartTime);
      Serial.print(" / ");
      Serial.println(cowTransitionTime);
    }
    break;

  case openAndOut:
    if (cowRetractButtonPressed)
    {
      // move to the next state
      currentStateCow = cowRetracting;
      Serial.println("Cows are now retracting");

      // restart the stopwatch
      transitionStartTime = millis();

      // start retracting the cows
      digitalWrite(doorExtendPin, LOW);
      digitalWrite(doorRetractPin, LOW);
      digitalWrite(cowExtendPin, LOW);
      digitalWrite(cowRetractPin, HIGH);
    }
    break;

  case cowRetracting:
    // enough time has passed?
    if (millis() - transitionStartTime >= cowTransitionTime)
    {
      if (bypassMonitors || cowRetractedMonitorActive)
      {
        if (bypassMonitors)
        {
          Serial.println("WARNING: Monitors are bypassed");
        }

        Serial.println("Cow retraction transition time complete, and monitor active. Cows are now fully retracted");

        // move to the next state
        currentStateCow = doorClosing;

        // restart the stopwatch
        transitionStartTime = millis();

        // start closing the door
        digitalWrite(doorExtendPin, LOW);
        digitalWrite(doorRetractPin, HIGH);
        digitalWrite(cowExtendPin, LOW);
        digitalWrite(cowRetractPin, LOW);
      }
      else
      {
        // keep waiting for the monitor
        Serial.println("Cow retraction transition time complete, still waiting for monitor...");
      }
    }
    else
    {
      Serial.print("retracting cows. ");
      Serial.print(millis() - transitionStartTime);
      Serial.print(" / ");
      Serial.println(cowTransitionTime);
    }
    break;

  case doorClosing:
    // enough time has passed?
    if (millis() - transitionStartTime >= doorTransitionTime)
    {
      if (bypassMonitors || cowRetractedMonitorActive)
      {
        if (bypassMonitors)
        {
          Serial.println("WARNING: Monitors are bypassed");
        }

        Serial.println("Doors closed. Cows are in the barn");

        // move to the next state
        currentStateCow = closedAndIn;

        // turn all motors off
        digitalWrite(doorExtendPin, LOW);
        digitalWrite(doorRetractPin, LOW);
        digitalWrite(cowExtendPin, LOW);
        digitalWrite(cowRetractPin, LOW);
      }
      else
      {
        // keep waiting for the monitor
        Serial.println("Door close transition time complete, still waiting for monitor...");
      }
    }
    else
    {
      Serial.print("closing doors. ");
      Serial.print(millis() - transitionStartTime);
      Serial.print(" / ");
      Serial.println(doorTransitionTime);
    }
    break;
  }
}
