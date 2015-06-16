#include <LiquidCrystal.h>
#include <EthernetUdp.h>
#include <Ethernet.h>
#include <RTClib.h>
#include <Wire.h>
#include <SPI.h>

LiquidCrystal lcd(6,7,8,9,11,12);
RTC_DS1307 rtc;

unsigned long tick, tack;

struct clock{
  DateTime time, old;
  bool display = false;
  int utcOfset = 0;

  String data(){
    String dataString;
    if(time.hour() < 10) dataString += '0';
    dataString += time.hour()+utcOfset; dataString += ':';
    if(time.minute() < 10) dataString += '0';
    dataString += time.minute(); dataString += ':';
    if(time.second() < 10) dataString += '0';
    dataString += time.second();

    if(time.second()%10 >= 0 && time.second()%10 <= 7){
      dataString += "   ";
      if(time.day() < 10) dataString += '0';
      dataString += time.day(); dataString += '/';
      if(time.month() < 10) dataString += '0';
      dataString += time.month();
    }
    else{
      dataString += "    ";
      dataString += time.year();
    }

    return dataString;
  }

  void printToLcd(int line){
    if (display){
      if(old.second() != time.second()){
        lcd.setCursor(0,line);
        lcd.print(data());
        old = time;
      }
    }
  }
};

struct timer{
  unsigned long timerTime, old;
  bool display = false;
  bool set;

  String data(){
    String dataString;
    TimeSpan time(timerTime);
    if(time.day() > 0){
      dataString += time.day(); dataString += ' ';
    }

    if(time.hour() > 0){
      dataString += time.hour(); dataString += ':';
    }

    if(time.minute() < 10) dataString += '0';
    dataString += time.minute(); dataString += ':';
    if(time.second() < 10) dataString += '0';
    dataString += time.second();dataString += "  ";

    return dataString;
  }

  void printToLcd(int line){
    if (display){
      if(old != timerTime){
        lcd.setCursor(0,line);
        lcd.print(data());
        old = timerTime;
      }
    }
  }
};

timer timer1;
clock clock1;

void setup(){
  Serial.begin(57600);
  lcd.begin(16, 2);
  Wire.begin();
  rtc.begin();
  pinMode(13,OUTPUT);
  digitalWrite(13,HIGH);

  if (!rtc.isrunning()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  timer1.display=1;
  timer1.timerTime = 100;
  clock1.display=1;
}

void loop(){
  tick=millis();
  if(tick >= (tack+1000)){
    if(timer1.timerTime)timer1.timerTime --;
    tack = millis();tack -= millis()%10;
  }

  
  DateTime now, then;
  clock1.time = rtc.now();
  clock1.printToLcd(0);

  timer1.printToLcd(1);
}
