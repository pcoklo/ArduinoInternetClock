#include <LiquidCrystal.h>
#include <Ethernet.h>
#include <RTClib.h>
#include <Wire.h>
#include <SPI.h>

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 177);

LiquidCrystal lcd(5,7,8,9,11,12);
EthernetServer server(80);
RTC_DS1307 rtc;

unsigned long tick, tack;

struct clock{
  DateTime time, old;
  bool display = false;
  int line;

  String data(){
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

    return dataString;
  }

  void printToLcd(int linet){
    line = linet;
    time = rtc.now();
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
  bool enable = 0;
  bool off = 0;
  int line;

  void begin(unsigned long time){
    timerTime = time;
  }

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

  void printToLcd(int linet){
    line = linet;
    if (display){
      if(old != timerTime){
        lcd.setCursor(0,line);
        lcd.print(data());
        old = timerTime;
      }
    }
  }
};

struct stopwatch{
  unsigned long startMillis, currentMillis;
  bool display = false;
  int line;

  String data(){
    String dataString;
    currentMillis = millis();
    TimeSpan time((currentMillis - startMillis)/1000);
    if(time.day() > 0){
      dataString += time.day(); dataString += ' ';
    }

    if(time.hour() > 0){
      dataString += time.hour(); dataString += ':';
    }

    if(time.minute() < 10) dataString += '0';
    dataString += time.minute(); dataString += ':';
    if(time.second() < 10) dataString += '0';
    dataString += time.second();dataString += ':';
    int milis = (currentMillis - startMillis)%1000;
    if(milis < 10) dataString += "0";
    if(milis < 100) dataString += "0";
    dataString += milis;

    return dataString;
  }

  void printToLcd(int linet){
    line = linet;
    if (display){
      lcd.setCursor(0,line);
      lcd.print(data());
    }
  }

  void start(){
    startMillis = millis();
  }
};

struct alarm{
  TimeSpan time;
  bool display = false;
  bool enable = 0;
  bool off = 0;
  int line;

  void begin(unsigned long h, unsigned long m, unsigned long s=0){
    TimeSpan set(s + m*60 + h*60*60);
    enable = 1;
    time = set;
  }

  String data(){
    String dataString = "ALARM: ";
    
    if(time.hour() < 10) dataString += '0';
    dataString += time.hour(); dataString += ':';
    if(time.minute() < 10) dataString += '0';
    dataString += time.minute();
    if(time.second()){
      dataString += ':';
      if(time.second() < 10) dataString += '0';
      dataString += time.second();
    }
    return dataString;
  }

  void printToLcd(int linet){
    line = linet;
    if (display){
      lcd.setCursor(0,line);
      lcd.print(data());
    }
  }
};

stopwatch stopwatch1;
timer timer1;
alarm alarm1;
clock clock1;

void setup(){
  Serial.begin(57600);
  Ethernet.begin(mac, ip);
  server.begin();
  lcd.begin(16, 2);
  Wire.begin();
  rtc.begin();
  pinMode(13,OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(A0, INPUT_PULLUP);
  digitalWrite(13,HIGH);

  if (!rtc.isrunning()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  clock1.display=1;
  clock1.printToLcd(0);
}

void loop(){
  tick=millis();
  if(tick >= (tack+1000)){
    if(timer1.timerTime){
      timer1.timerTime --;
      if(timer1.timerTime == 0) timer1.off =1;
    }
    tack = millis();tack -= millis()%10;
    clock1.printToLcd(0);

    if(alarm1.time.hour() == clock1.time.hour() && alarm1.time.minute() == clock1.time.minute() && clock1.time.second()==0){
      alarm1.off = 1;
    }
  }

  if(digitalRead(A0) == LOW){
    delay(10);
    if(alarm1.off){
      alarm1.off = 0;
      alarm1.display=0;
      lcd.setCursor(0,alarm1.line);
      lcd.print("                ");
    }
    if(timer1.off){
      timer1.off = 0;
      timer1.display=0;
      lcd.setCursor(0,timer1.line);
      lcd.print("                ");
    }
  }

  EthernetClient client = server.available();
  if (client) {
    // an http request ends with a blank line
    while (client.connected()) {
      if (client.available()) {
        client.readStringUntil(47);
        String dataString = client.readStringUntil(' ');
        client.readStringUntil('\n');
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println();
        client.println("<!DOCTYPE HTML>");
        client.println("<html>");
        client.println("Online Clock!");
        if(dataString.startsWith("alarm")){
          String tempString=dataString.substring(dataString.indexOf("(")+1,dataString.indexOf(":"));
          long hour = tempString.toInt();
          tempString=dataString.substring(dataString.indexOf(":")+1,dataString.indexOf(")"));
          long minute = tempString.toInt();
          alarm1.begin(hour,minute);
          alarm1.display=1;
          alarm1.printToLcd(1);
        }
        else if(dataString.startsWith("timer")){
          String tempString=dataString.substring(dataString.indexOf("(")+1,dataString.indexOf(")"));
          long time = tempString.toInt();
          timer1.begin(time);
          timer1.display=1;
          timer1.printToLcd(1);
        }
        else {
          client.print("Wrong input");
        }
        client.println("</html>");
        break;
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }

  timer1.printToLcd(1);
  stopwatch1.printToLcd(1);
  if(alarm1.off || timer1.off){
    if(millis()%1000 < 500){
      tone(4,5000);
      digitalWrite(13,0);
    }
    else {
      noTone(4);
      digitalWrite(13,1);
    }
  }
  else{
    noTone(4);
    digitalWrite(13,1);
  }
}
