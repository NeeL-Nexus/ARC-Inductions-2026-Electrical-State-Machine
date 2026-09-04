#include <Wire.h>
#include <IRremote.hpp>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

volatile byte currentState = 0;
volatile int currentValue = 0;
volatile bool newData = false;

volatile byte pendingCommand = 0; // holds one IR command until master asks for it
bool showGas = false;             // toggled by remote, only used here on the LCD

void setup() {
  Wire.begin(8); // this board is I2C slave, address 8
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  IrReceiver.begin(7);
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
}

void loop() {
  if (IrReceiver.decode()) {
    unsigned long raw = IrReceiver.decodedIRData.decodedRawData;
    if (raw == 0xFF00BF00) {
      pendingCommand = 1; // activate
    } else if (raw == 0xF30CBF00) {
      showGas = !showGas; // switch LCD between light/gas readout
    } else if (raw == 0xEF10BF00) {
      pendingCommand = 3; // reset
    }
    IrReceiver.resume();
  }

  if (newData) {
    updateLCD();
    newData = false;
  }

  delay(100);
}

void updateLCD() {
  static byte lastState = 255; // forces the very first draw
  static bool lastShowGas = true;

  // only clear+redraw the label when it actually needs to change
  if (currentState != lastState || (currentState == 1 && showGas != lastShowGas)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    if (currentState == 0) lcd.print("AWAITING RITUAL");
    else if (currentState == 1) lcd.print(showGas ? "Gas: " : "Light: ");
    else if (currentState == 2) lcd.print("TOXIC PURGE");
    else if (currentState == 3) lcd.print("NOCTIS PROTOCOL");
    else if (currentState == 4) lcd.print("COOKED");
    else if (currentState == 5) {
      lcd.print("MULTIPLE");
      lcd.setCursor(0, 1);
      lcd.print("PROBLEMS");
    }
    lastState = currentState;
    lastShowGas = showGas;
  }

  // the number next to "Light:"/"Gas:" needs refreshing every time
  if (currentState == 1) {
    lcd.setCursor(7, 0);
    lcd.print("    "); // clear old digits first
    lcd.setCursor(7, 0);
    lcd.print(currentValue);
  }
}

// runs automatically when the master sends data
void receiveEvent(int howMany) {
  if (howMany >= 5) {
    currentState = Wire.read();
    byte lightHi = Wire.read();
    byte lightLo = Wire.read();
    int light = (lightHi << 8) | lightLo;
    byte gasHi = Wire.read();
    byte gasLo = Wire.read();
    int gas = (gasHi << 8) | gasLo;
    currentValue = showGas ? gas : light;
    newData = true;
  }
}

// runs automatically when the master asks for a byte
void requestEvent() {
  Wire.write(pendingCommand);
  pendingCommand = 0; // clear it once it's been sent
}
