#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.h>

#define IR 2

LiquidCrystal_I2C lcd(0x27,16,2);

byte state=0,cmd=0;
bool lightMode=true;

void setup(){
  Wire.begin(8);
  Wire.onReceive(receive);
  Wire.onRequest(send);

  lcd.init();
  lcd.backlight();

  IrReceiver.begin(IR);
}

void loop(){

  if(IrReceiver.decode()){

    byte c=IrReceiver.decodedIRData.command;

    if(c==0x45 && state==0) cmd=1;       // Activate
    else if(c==0x46 && state==1)         // Toggle
      lightMode=!lightMode;
    else if(c==0x47 && state==4) cmd=3;  // Reset

    IrReceiver.resume();
  }

  lcd.clear();

  if(state==0)
    lcd.print("AWAITING RITUAL");

  else if(state==1){
    if(lightMode){
      lcd.print("LIGHT:");
      lcd.print(analogRead(A0));
    }else{
      lcd.print("GAS:");
      lcd.print("MONITOR");
    }
  }

  else if(state==2)
    lcd.print("TOXIC PURGE");

  else if(state==3)
    lcd.print("NOCTIS PROTOCOL");

  else if(state==4)
    lcd.print("COOKED");

  else if(state==5){
    lcd.print("MULTIPLE");
    lcd.setCursor(0,1);
    lcd.print("PROBLEMS");
  }

  delay(150);
}

void receive(int n){
  if(Wire.available()) state=Wire.read();
}

void send(){
  Wire.write(cmd);
  cmd=0;
}
