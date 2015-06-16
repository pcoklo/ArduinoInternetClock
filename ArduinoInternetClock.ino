#include <LiquidCrystal.h>
#include <EthernetUdp.h>
#include <Ethernet.h>
#include <RTClib.h>
#include <Wire.h>
#include <SPI.h>

LiquidCrystal lcd(6,7,8,9,11,12);
RTC_DS1307 rtc;

DateTime now, old;

unsigned long stopWatchStart, stopWatchStop;

uint32_t timerTime=0, oldTimer;

bool timerFlag=0, stopWatchFlag=0;

void setup(){
  Serial.begin(57600);
  lcd.begin(16, 2);
  Wire.begin();
  rtc.begin();
  pinMode(13,OUTPUT);
  digitalWrite(13,HIGH);

  if (!rtc.isrunning())
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop(){
  now = rtc.now();
  if(old.second() != now.second()) lcdPrintDateTime(now, 0);


  if(timerFlag){
    TimeSpan timer(timerTime);
    if(timerTime != oldTimer) lcdPrintTimeSpan(timer, 1);
    timerTime--;
  }

  if(Serial.available()){
    String dataString = Serial.readStringUntil('\n');
    if(dataString.startsWith("kreni")){
      stopWatchStart = millis();
      stopWatchFlag = 1;
    }
    else if(dataString.startsWith("stani")){
      stopWatchFlag = 0;
    }
  }

  if(stopWatchFlag){
    TimeSpan stopWatch((millis() - stopWatchStart)/1000);
    lcdPrintTimeSpan (stopWatch, 1);
  }
}

void lcdPrintDateTime(DateTime &time, int line){
  old = time;
  lcd.setCursor(0,line);
  String dataString;

  if(time.hour() < 10) dataString += '0';
  dataString += time.hour(); dataString += ':';
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
  lcd.print(dataString);
}

void lcdPrintTimeSpan(TimeSpan &time, int line){
  lcd.setCursor(0,line);
  String dataString;

  if(time.day() > 0){
    dataString += time.day(); dataString += ' ';
  }
  if(time.hour() > 0){
    dataString += time.hour(); dataString += ':';
  }
  
  if(time.minute() < 10) dataString += '0';
  dataString += time.minute(); dataString += ':';
  if(time.second() < 10) dataString += '0';
  dataString += time.second();
  if(stopWatchFlag){
    dataString += ':';
    dataString += (millis() - stopWatchStart)%1000;
  }
  dataString += "  ";
  lcd.print(dataString);
}
