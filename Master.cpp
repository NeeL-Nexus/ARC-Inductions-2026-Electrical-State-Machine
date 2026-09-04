#include <Wire.h>
#include <Servo.h>

Servo myServo;

// state numbers (instead of an enum)
#define STANDBY 0
#define ACTIVE 1
#define GAS_ALERT 2
#define BLACKOUT 3
#define TEMP_EMERGENCY 4
#define MULTI_FAULT 5

int currentState = STANDBY;
int previousLight = -1; // -1 means "no reading yet"

bool systemActivated = false; // turned on by remote (via slave)
bool tempLatched = false;     // cleared only by remote reset
bool blackoutLatched = false; // clears itself when bright again
bool gasLatched = false;      // clears itself when gas drops

void setup() {
  Wire.begin();       // join I2C bus as master
  Serial.begin(9600);
  myServo.attach(9);
}

void loop() {
  int lightLevel = analogRead(A2);
  int gasLevel = analogRead(A1);
  int rawTemp = analogRead(A3);

  // TMP36: 0.5V at 0C, +10mV per degree C
  float voltage = rawTemp * (5.0 / 1023.0);
  float tempC = (voltage - 0.5) * 100;

  updateState(lightLevel, gasLevel, tempC);

  // servo locks the "door" only during temp emergency
  if (currentState == TEMP_EMERGENCY) {
    myServo.write(180);
  } else {
    myServo.write(0);
  }

  // buzzer only for the worst case (gas + blackout together)
  if (currentState == MULTI_FAULT) {
    tone(2, 1000);
  } else {
    noTone(2);
  }

  // send state + raw sensor values to the slave board
  Wire.beginTransmission(8);
  Wire.write((byte)currentState);
  Wire.write(highByte(lightLevel));
  Wire.write(lowByte(lightLevel));
  Wire.write(highByte(gasLevel));
  Wire.write(lowByte(gasLevel));
  Wire.endTransmission();

  // ask the slave if a remote button was pressed
  Wire.requestFrom(8, 1);
  if (Wire.available()) {
    byte irCmd = Wire.read();
    if (irCmd == 1) {
      systemActivated = true;
    } else if (irCmd == 3) {
      tempLatched = false;
      if (!systemActivated) {
        currentState = STANDBY;
      }
    }
  }

  Serial.print("Light: "); Serial.print(lightLevel);
  Serial.print(" Gas: "); Serial.print(gasLevel);
  Serial.print(" Temp: "); Serial.print(tempC);
  Serial.print(" State: "); Serial.println(currentState);

  delay(500);
}

void updateState(int lightLevel, int gasLevel, float tempC) {
  // did the light suddenly drop a lot? (someone covered the sensor)
  bool suddenDrop = false;
  if (previousLight != -1) {
    suddenDrop = (previousLight - lightLevel) > 300;
  }
  if (suddenDrop) blackoutLatched = true;
  if (lightLevel > 400) blackoutLatched = false; // bright again = clear it

  // gas check
  if (gasLevel > 180) gasLatched = true;
  if (gasLevel < 130) gasLatched = false;

  // temperature check (highest priority, needs remote reset to clear)
  if (tempC > 45) tempLatched = true;

  if (tempLatched) {
    currentState = TEMP_EMERGENCY;
    previousLight = lightLevel;
    return;
  }

  if (systemActivated) {
    if (gasLatched && blackoutLatched) {
      currentState = MULTI_FAULT;
    } else if (gasLatched) {
      currentState = GAS_ALERT;
    } else if (blackoutLatched) {
      currentState = BLACKOUT;
    } else {
      currentState = ACTIVE;
    }
    previousLight = lightLevel;
  }
}
