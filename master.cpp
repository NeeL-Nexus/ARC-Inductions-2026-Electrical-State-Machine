#include <Wire.h>
#include <Servo.h>

#define SLAVE 8
#define LDR A0
#define GAS A1
#define TEMP A2
#define BUZZ 8
#define SERVO 9

Servo s;
byte state=0;
int light,gas;
float temp;
int oldLight;

void setup(){
  Wire.begin();
  pinMode(BUZZ,OUTPUT);
  s.attach(SERVO);
  s.write(0);
  oldLight=analogRead(LDR);
}

void loop(){
  light=analogRead(LDR);
  gas=analogRead(GAS);
  temp=analogRead(TEMP)*500.0/1023.0;

  Wire.requestFrom(SLAVE,1);
  if(Wire.available()){
    byte c=Wire.read();

    if(c==1 && state==0) state=1;       // Activate
    if(c==3 && state==4){               // Reset
      state=1;
      s.write(0);
    }
  }

  // Temperature = highest priority
  if(temp>45){
    state=4;
    s.write(180);
    digitalWrite(BUZZ,LOW);
  }

  else if(state==4){
    s.write(180);
    digitalWrite(BUZZ,LOW);
  }

  else if(state==0){
    digitalWrite(BUZZ,LOW);
    s.write(0);
  }

  else{
    bool g=gas>180;
    bool b=(oldLight-light)>250;

    if(g && b){
      state=5;
      digitalWrite(BUZZ,HIGH);
    }
    else if(state==2 && gas<130){
      state=1;
      digitalWrite(BUZZ,LOW);
    }
    else if(state==3 && !b){
      state=1;
      digitalWrite(BUZZ,LOW);
    }
    else if(g){
      state=2;
      digitalWrite(BUZZ,LOW);
    }
    else if(b){
      state=3;
      digitalWrite(BUZZ,LOW);
    }
    else{
      digitalWrite(BUZZ,LOW);
      if(state==5) state=1;
    }
  }

  Wire.beginTransmission(SLAVE);
  Wire.write(state);
  Wire.endTransmission();

  oldLight=light;
  delay(200);
}
