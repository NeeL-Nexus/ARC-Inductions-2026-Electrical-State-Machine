#include <Wire.h>
#include <Servo.h>

#define SLAVE_ADDRESS 8

// Sensors
#define LDR_PIN A0
#define GAS_PIN A1
#define TEMP_PIN A2

// Outputs
#define BUZZER_PIN 8
#define SERVO_PIN 9

Servo ventServo;

// States
#define STANDBY 0
#define MONITORING 1
#define GAS_ALERT 2
#define BLACKOUT 3
#define TEMP_EMERGENCY 4

int state = STANDBY;

int lightValue = 0;
int gasValue = 0;

float temperature = 0;

// Previous light value
int previousLight = 0;

// Blackout threshold
// Adjust this after testing your LDR
const int BLACKOUT_DROP = 250;

// I2C data structure
struct SensorData {
  byte state;
  int light;
  int gas;
  int temperature;
};

SensorData data;

void setup() {

  Serial.begin(9600);

  Wire.begin();

  pinMode(BUZZER_PIN, OUTPUT);

  ventServo.attach(SERVO_PIN);
  ventServo.write(0);

  digitalWrite(BUZZER_PIN, LOW);

  previousLight = analogRead(LDR_PIN);

  data.state = STANDBY;

  Serial.println("MASTER READY");
}


void loop() {

  readSensors();

  // --------------------------------
  // TEMPERATURE HAS HIGHEST PRIORITY
  // --------------------------------

  if (temperature > 45.0 && state != TEMP_EMERGENCY) {

    state = TEMP_EMERGENCY;

    ventServo.write(180);

    Serial.println("TEMPERATURE EMERGENCY");
  }


  // --------------------------------
  // TEMP EMERGENCY
  // --------------------------------

  if (state == TEMP_EMERGENCY) {

    // Keep servo at emergency position
    ventServo.write(180);

    digitalWrite(BUZZER_PIN, LOW);

    sendData();

    delay(200);

    return;
  }


  // --------------------------------
  // STANDBY
  // --------------------------------

  if (state == STANDBY) {

    digitalWrite(BUZZER_PIN, LOW);

    sendData();

    delay(200);

    return;
  }


  // --------------------------------
  // CHECK GAS
  // --------------------------------

  bool gasFault = false;

  if (gasValue > 180) {
    gasFault = true;
  }


  // --------------------------------
  // CHECK BLACKOUT
  // --------------------------------

  bool blackoutFault = false;

  if ((previousLight - lightValue) > BLACKOUT_DROP) {
    blackoutFault = true;
  }


  // --------------------------------
  // MULTI-FAULT
  // --------------------------------

  if (gasFault && blackoutFault) {

    state = 5; // MULTI-FAULT

    digitalWrite(BUZZER_PIN, HIGH);

  }


  // --------------------------------
  // GAS ALERT
  // --------------------------------

  else if (gasValue > 180) {

    state = GAS_ALERT;

    digitalWrite(BUZZER_PIN, LOW);
  }


  // --------------------------------
  // BLACKOUT
  // --------------------------------

  else if (blackoutFault) {

    state = BLACKOUT;

    digitalWrite(BUZZER_PIN, LOW);
  }


  // --------------------------------
  // GAS ALERT RESOLUTION
  // --------------------------------

  else if (state == GAS_ALERT && gasValue < 130) {

    state = MONITORING;

    digitalWrite(BUZZER_PIN, LOW);
  }


  // --------------------------------
  // BLACKOUT RESOLUTION
  // --------------------------------

  else if (state == BLACKOUT) {

    if (lightValue >= previousLight - BLACKOUT_DROP) {

      state = MONITORING;
    }

    digitalWrite(BUZZER_PIN, LOW);
  }


  // --------------------------------
  // MULTI-FAULT RESOLUTION
  // --------------------------------

  else if (state == 5) {

    digitalWrite(BUZZER_PIN, LOW);

    if (gasValue <= 180 && lightValue >= previousLight - BLACKOUT_DROP) {

      state = MONITORING;
    }
    else if (gasValue > 180) {

      state = GAS_ALERT;
    }
    else if (lightValue < previousLight - BLACKOUT_DROP) {

      state = BLACKOUT;
    }
  }


  sendData();

  previousLight = lightValue;

  delay(200);
}


// ====================================
// READ SENSORS
// ====================================

void readSensors() {

  lightValue = analogRead(LDR_PIN);

  gasValue = analogRead(GAS_PIN);

  int tempRaw = analogRead(TEMP_PIN);

  float voltage = tempRaw * (5.0 / 1023.0);

  temperature = voltage * 100.0;

  Serial.print("Light: ");
  Serial.print(lightValue);

  Serial.print(" | Gas: ");
  Serial.print(gasValue);

  Serial.print(" | Temp: ");
  Serial.println(temperature);
}


// ====================================
// SEND DATA TO SLAVE
// ====================================

void sendData() {

  data.state = state;
  data.light = lightValue;
  data.gas = gasValue;
  data.temperature = temperature;

  Wire.beginTransmission(SLAVE_ADDRESS);

  Wire.write((byte*)&data, sizeof(data));

  Wire.endTransmission();
}
