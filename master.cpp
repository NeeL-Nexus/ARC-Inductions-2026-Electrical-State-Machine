#include <Wire.h>
#include <Servo.h>

// =====================================================
// I2C
// =====================================================

#define SLAVE_ADDRESS 8

// Commands received from Slave
#define NO_COMMAND       0
#define ACTIVATE_COMMAND 1
#define TOGGLE_COMMAND   2
#define RESET_COMMAND    3


// =====================================================
// MASTER PINS
// =====================================================

#define LDR_PIN     A0
#define GAS_PIN     A1
#define TEMP_PIN    A2

#define BUZZER_PIN  8
#define SERVO_PIN   9


// =====================================================
// STATES
// =====================================================

#define STATE_STANDBY       0
#define STATE_MONITORING    1
#define STATE_GAS_ALERT     2
#define STATE_BLACKOUT      3
#define STATE_TEMP_EMERGENCY 4
#define STATE_MULTI_FAULT   5


// =====================================================
// OBJECTS
// =====================================================

Servo ventServo;


// =====================================================
// VARIABLES
// =====================================================

byte currentState = STATE_STANDBY;

int lightValue = 0;
int gasValue = 0;

float temperature = 0.0;

// Used to detect sudden light changes
int previousLightValue = 0;

// Adjust this if required after Tinkercad testing
const int BLACKOUT_DROP = 250;


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(9600);

  // Start I2C as Master
  Wire.begin();

  // Sensor pins
  pinMode(LDR_PIN, INPUT);
  pinMode(GAS_PIN, INPUT);
  pinMode(TEMP_PIN, INPUT);

  // Output pins
  pinMode(BUZZER_PIN, OUTPUT);

  // Start buzzer OFF
  digitalWrite(BUZZER_PIN, LOW);

  // Start servo
  ventServo.attach(SERVO_PIN);

  // Normal position
  ventServo.write(0);

  // Get initial light reading
  previousLightValue = analogRead(LDR_PIN);

  Serial.println("================================");
  Serial.println("MASTER ARDUINO READY");
  Serial.println("STATE: STANDBY");
  Serial.println("================================");
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop()
{
  // ---------------------------------------------------
  // Read all sensors
  // ---------------------------------------------------

  readSensors();


  // ---------------------------------------------------
  // Check IR commands
  // ---------------------------------------------------

  checkSlaveCommand();


  // ---------------------------------------------------
  // TEMPERATURE HAS ABSOLUTE PRIORITY
  // ---------------------------------------------------

  if (temperature > 45.0)
  {
    currentState = STATE_TEMP_EMERGENCY;

    ventServo.write(180);

    digitalWrite(BUZZER_PIN, LOW);

    printState();

    delay(200);

    return;
  }


  // ---------------------------------------------------
  // STATE 4: TEMPERATURE EMERGENCY
  // ---------------------------------------------------

  if (currentState == STATE_TEMP_EMERGENCY)
  {
    ventServo.write(180);

    digitalWrite(BUZZER_PIN, LOW);

    delay(200);

    return;
  }


  // ---------------------------------------------------
  // STANDBY
  // ---------------------------------------------------

  if (currentState == STATE_STANDBY)
  {
    digitalWrite(BUZZER_PIN, LOW);

    ventServo.write(0);

    printState();

    delay(200);

    return;
  }


  // ---------------------------------------------------
  // Determine faults
  // ---------------------------------------------------

  bool gasFault = false;
  bool blackoutFault = false;


  // Gas alert threshold
  if (gasValue > 180)
  {
    gasFault = true;
  }


  // Sudden absolute light drop
  if ((previousLightValue - lightValue) >= BLACKOUT_DROP)
  {
    blackoutFault = true;
  }


  // ---------------------------------------------------
  // MULTI-FAULT
  // Gas + Blackout simultaneously
  // ---------------------------------------------------

  if (gasFault && blackoutFault)
  {
    currentState = STATE_MULTI_FAULT;

    // Continuous buzzer
    digitalWrite(BUZZER_PIN, HIGH);
  }


  // ---------------------------------------------------
  // GAS ALERT
  // ---------------------------------------------------

  else if (currentState == STATE_GAS_ALERT)
  {
    // Remain in gas alert until below 130
    if (gasValue < 130)
    {
      currentState = STATE_MONITORING;

      digitalWrite(BUZZER_PIN, LOW);
    }
    else
    {
      currentState = STATE_GAS_ALERT;

      digitalWrite(BUZZER_PIN, LOW);
    }
  }


  // ---------------------------------------------------
  // BLACKOUT
  // ---------------------------------------------------

  else if (currentState == STATE_BLACKOUT)
  {
    // Ignore normal IR commands while blackout exists

    if (lightValue >= previousLightValue - BLACKOUT_DROP)
    {
      currentState = STATE_MONITORING;
    }
    else
    {
      currentState = STATE_BLACKOUT;
    }

    digitalWrite(BUZZER_PIN, LOW);
  }


  // ---------------------------------------------------
  // MULTI-FAULT
  // ---------------------------------------------------

  else if (currentState == STATE_MULTI_FAULT)
  {
    digitalWrite(BUZZER_PIN, HIGH);

    // Both faults gone
    if (gasValue <= 180 &&
        lightValue >= previousLightValue - BLACKOUT_DROP)
    {
      currentState = STATE_MONITORING;

      digitalWrite(BUZZER_PIN, LOW);
    }

    // Only gas problem remains
    else if (gasValue > 180 &&
             lightValue >= previousLightValue - BLACKOUT_DROP)
    {
      currentState = STATE_GAS_ALERT;

      digitalWrite(BUZZER_PIN, LOW);
    }

    // Only blackout remains
    else if (gasValue <= 180 &&
             lightValue < previousLightValue - BLACKOUT_DROP)
    {
      currentState = STATE_BLACKOUT;

      digitalWrite(BUZZER_PIN, LOW);
    }
  }


  // ---------------------------------------------------
  // NORMAL MONITORING
  // ---------------------------------------------------

  else if (currentState == STATE_MONITORING)
  {
    if (gasFault && blackoutFault)
    {
      currentState = STATE_MULTI_FAULT;

      digitalWrite(BUZZER_PIN, HIGH);
    }

    else if (gasFault)
    {
      currentState = STATE_GAS_ALERT;

      digitalWrite(BUZZER_PIN, LOW);
    }

    else if (blackoutFault)
    {
      currentState = STATE_BLACKOUT;

      digitalWrite(BUZZER_PIN, LOW);
    }

    else
    {
      digitalWrite(BUZZER_PIN, LOW);
    }
  }


  // ---------------------------------------------------
  // Print state
  // ---------------------------------------------------

  printState();


  // ---------------------------------------------------
  // Save current light reading
  // ---------------------------------------------------

  previousLightValue = lightValue;


  delay(200);
}


// =====================================================
// READ SENSORS
// =====================================================

void readSensors()
{
  // LDR
  lightValue = analogRead(LDR_PIN);


  // Gas sensor
  gasValue = analogRead(GAS_PIN);


  // LM35
  int rawTemperature = analogRead(TEMP_PIN);


  // Convert ADC reading to voltage
  float voltage = rawTemperature * (5.0 / 1023.0);


  // LM35 = approximately 10mV per °C
  temperature = voltage * 100.0;


  // Serial monitor
  Serial.print("LIGHT = ");
  Serial.print(lightValue);

  Serial.print(" | GAS = ");
  Serial.print(gasValue);

  Serial.print(" | TEMP = ");
  Serial.print(temperature);

  Serial.println(" C");
}


// =====================================================
// ASK SLAVE FOR IR COMMAND
// =====================================================

void checkSlaveCommand()
{
  Wire.requestFrom(SLAVE_ADDRESS, 1);


  if (Wire.available())
  {
    byte command = Wire.read();


    // -----------------------------------------------
    // ACTIVATE
    // -----------------------------------------------

    if (command == ACTIVATE_COMMAND)
    {
      if (currentState == STATE_STANDBY)
      {
        currentState = STATE_MONITORING;

        Serial.println("IR: SYSTEM ACTIVATED");
      }
    }


    // -----------------------------------------------
    // TOGGLE DISPLAY
    // -----------------------------------------------

    else if (command == TOGGLE_COMMAND)
    {
      // Display toggle is handled by Slave.
      // Master doesn't need to change state.

      Serial.println("IR: DISPLAY TOGGLE");
    }


    // -----------------------------------------------
    // MANUAL TEMPERATURE RESET
    // -----------------------------------------------

    else if (command == RESET_COMMAND)
    {
      if (currentState == STATE_TEMP_EMERGENCY)
      {
        currentState = STATE_MONITORING;

        ventServo.write(0);

        digitalWrite(BUZZER_PIN, LOW);

        Serial.println("IR: EMERGENCY RESET");
      }
    }
  }
}


// =====================================================
// PRINT CURRENT STATE
// =====================================================

void printState()
{
  static byte previousState = 255;


  if (currentState != previousState)
  {
    Serial.println("--------------------------------");


    if (currentState == STATE_STANDBY)
    {
      Serial.println("STATE 0: AWAITING RITUAL");
    }


    else if (currentState == STATE_MONITORING)
    {
      Serial.println("STATE 1: ACTIVE MONITORING");
    }


    else if (currentState == STATE_GAS_ALERT)
    {
      Serial.println("STATE 2: TOXIC PURGE");
    }


    else if (currentState == STATE_BLACKOUT)
    {
      Serial.println("STATE 3: NOCTIS PROTOCOL");
    }


    else if (currentState == STATE_TEMP_EMERGENCY)
    {
      Serial.println("STATE 4: COOKED");
    }


    else if (currentState == STATE_MULTI_FAULT)
    {
      Serial.println("MULTIPLE PROBLEMS DETECTED");
    }


    Serial.println("--------------------------------");

    previousState = currentState;
  }
}
