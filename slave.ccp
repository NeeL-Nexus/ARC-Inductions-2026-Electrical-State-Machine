#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.h>

#define SLAVE_ADDRESS 8

#define IR_PIN 2

LiquidCrystal_I2C lcd(0x27, 16, 2);


// --------------------------------
// STATES
// --------------------------------

#define STANDBY 0
#define MONITORING 1
#define GAS_ALERT 2
#define BLACKOUT 3
#define TEMP_EMERGENCY 4
#define MULTI_FAULT 5


// --------------------------------
// SENSOR DATA
// --------------------------------

struct SensorData {

  byte state;

  int light;

  int gas;

  int temperature;
};

SensorData data;


// --------------------------------
// DISPLAY MODE
// --------------------------------

bool showLight = true;


// --------------------------------
// IR BUTTON CODES
// --------------------------------
// Change these after checking
// your remote's actual codes.

#define ACTIVATE_BUTTON 0x45
#define TOGGLE_BUTTON   0x46
#define RESET_BUTTON    0x47


void setup() {

  Serial.begin(9600);

  // I2C slave
  Wire.begin(SLAVE_ADDRESS);

  Wire.onReceive(receiveData);

  // LCD
  lcd.init();
  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("AWAITING");

  lcd.setCursor(0, 1);
  lcd.print("RITUAL");


  // IR
  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

  Serial.println("SLAVE READY");
}


void loop() {

  checkIR();

  displayState();

  delay(100);
}


// ====================================
// RECEIVE DATA FROM MASTER
// ====================================

void receiveData(int bytes) {

  if (bytes >= sizeof(data)) {

    Wire.readBytes((byte*)&data, sizeof(data));
  }
}


// ====================================
// IR CONTROL
// ====================================

void checkIR() {

  if (IrReceiver.decode()) {

    unsigned long command =
      IrReceiver.decodedIRData.command;

    Serial.print("IR Command: ");
    Serial.println(command, HEX);


    // -----------------------------
    // ACTIVATE
    // -----------------------------

    if (command == ACTIVATE_BUTTON) {

      if (data.state == STANDBY) {

        data.state = MONITORING;

        // Tell master to activate
        Wire.beginTransmission(8);

        Wire.write(100);

        Wire.endTransmission();
      }
    }


    // -----------------------------
    // TOGGLE DISPLAY
    // -----------------------------

    if (command == TOGGLE_BUTTON) {

      if (data.state == MONITORING) {

        showLight = !showLight;
      }
    }


    // -----------------------------
    // MANUAL RESET
    // -----------------------------

    if (command == RESET_BUTTON) {

      if (data.state == TEMP_EMERGENCY) {

        Wire.beginTransmission(8);

        Wire.write(101);

        Wire.endTransmission();
      }
    }


    IrReceiver.resume();
  }
}


// ====================================
// LCD DISPLAY
// ====================================

void displayState() {

  lcd.clear();


  // -----------------------------
  // STATE 0
  // -----------------------------

  if (data.state == STANDBY) {

    lcd.setCursor(0, 0);
    lcd.print("AWAITING");

    lcd.setCursor(0, 1);
    lcd.print("RITUAL");

    return;
  }


  // -----------------------------
  // STATE 1
  // -----------------------------

  if (data.state == MONITORING) {

    if (showLight) {

      lcd.setCursor(0, 0);
      lcd.print("LIGHT:");

      lcd.print(data.light);

      lcd.setCursor(0, 1);
      lcd.print("MONITORING");
    }

    else {

      lcd.setCursor(0, 0);
      lcd.print("GAS:");

      lcd.print(data.gas);

      lcd.setCursor(0, 1);
      lcd.print("AIR PURITY");
    }

    return;
  }


  // -----------------------------
  // STATE 2
  // -----------------------------

  if (data.state == GAS_ALERT) {

    lcd.setCursor(0, 0);
    lcd.print("TOXIC PURGE");

    lcd.setCursor(0, 1);

    lcd.print("GAS:");
    lcd.print(data.gas);

    return;
  }


  // -----------------------------
  // STATE 3
  // -----------------------------

  if (data.state == BLACKOUT) {

    lcd.setCursor(0, 0);
    lcd.print("NOCTIS");

    lcd.setCursor(0, 1);
    lcd.print("PROTOCOL");

    return;
  }


  // -----------------------------
  // STATE 4
  // -----------------------------

  if (data.state == TEMP_EMERGENCY) {

    lcd.setCursor(0, 0);
    lcd.print("COOKED");

    lcd.setCursor(0, 1);
    lcd.print("RESET REQUIRED");

    return;
  }


  // -----------------------------
  // MULTI FAULT
  // -----------------------------

  if (data.state == MULTI_FAULT) {

    lcd.setCursor(0, 0);
    lcd.print("MULTIPLE");

    lcd.setCursor(0, 1);
    lcd.print("PROBLEMS");

    return;
  }
}
