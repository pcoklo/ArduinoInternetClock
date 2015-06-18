#include <LiquidCrystal.h>  //lib za lcd
#include <EthernetUdp.h>
#include <Ethernet.h>
#include <RTClib.h> 
#include <Wire.h> //komunikacija preko i2c sa rtc
#include <SPI.h>  //komunikacija sa eternet preko spi

byte mac[] = {  // bite array macadresa
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

LiquidCrystal lcd(2,3,4,5,7,8);
EthernetServer server(80);
RTC_DS1307 rtc;
bool clearIP=1; //za brisanje ip adrese sa lcda
int updateTime=0; //za update vremena preko ntp servera

unsigned long tick, tack; //variable provjeravaju jeli prošla cca sekunda
unsigned int localPort = 8888;  //port za ntp
char timeServer[] = "time.nist.gov";  //sa te stranice kupimo vrijeme
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];
unsigned long ntpOfset=0;

struct clock{ //struktura za jednostavnije korištenje rtca i ispisa na lcd
  DateTime time, old; //time = trenuno vrijeme, old je prošlo
  bool display = false; //dozvoli ispis
  int line; //na koju lijinu ispisuje, 0-prva, 1-druga

  String data(){  //funkcija vraća uređeni string znakova sata
    String dataString;
    if(time.hour() < 10) dataString += '0';
    dataString += time.hour(); dataString += ':';
    if(time.minute() < 10) dataString += '0';
    dataString += time.minute(); dataString += ':';
    if(time.second() < 10) dataString += '0';
    dataString += time.second();
    //dodaje 0 za lijepši prikaz...

    if(time.second()%10 >= 0 && time.second()%10 <= 7){
      //između 0 i 7 sekunde dodaj datum
      dataString += "   ";
      if(time.day() < 10) dataString += '0';
      dataString += time.day(); dataString += '/';
      if(time.month() < 10) dataString += '0';
      dataString += time.month();
    }
    else{
      //između 8 i nulte dodaj godinu 
      dataString += "    ";
      dataString += time.year();
    }

    return dataString;
  }

  void printToLcd(int linet){
    line = linet;
    time = rtc.now(); //osvježi trenutno vrijeme
    if (display){
      if(old.second() != time.second()){
        //ako se vrijeme promjenilo od zadnjeg ispisa, ispiši novo vrjeme
        lcd.setCursor(0,line);  //ma,kesto lursor na početak zadane linije
        lcd.print(data());  //ispiši vrijeme na display
        old = time; //novo vrijeme je staro vrijeme
      }
    }
  }
};

struct timer{
  unsigned long timerTime, old;
  bool display = false;
  bool enable = 0;  //1 ako je pozvan
  bool off = 0; //1 ako je istekao
  int line;

  void begin(unsigned long time){
    // poziva se prilikom pokretanja timera
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
    dataString += time.second();
    //popuni atrin praznimama do veličine retka dipleja
    //za čišći ispis
    while (dataString.length() <16) dataString+=' ';

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
  //štoperica
  unsigned long startMillis, currentMillis;
  TimeSpan time;
  bool display = false;
  bool enable = false;
  int line;

  String data(){
    String dataString;
    currentMillis = millis();
    TimeSpan set((currentMillis - startMillis)/1000);
    if(enable) time = set;
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
    while (dataString.length() <16) dataString+=' ';

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
    enable = 1;
  }

  void stop(){
    enable = 0;
    display = 0;
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
    //ispis alarm na lcd da znam da smo postavili alarm
    
    if(time.hour() < 10) dataString += '0';
    dataString += time.hour(); dataString += ':';
    if(time.minute() < 10) dataString += '0';
    dataString += time.minute();
    if(time.second()){
      dataString += ':';
      if(time.second() < 10) dataString += '0';
      dataString += time.second();
    }
    while (dataString.length() <16) dataString+=' ';
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

EthernetUDP Udp;  //za pozivanje postavljanja vremena
stopwatch stopwatch1;
timer timer1;
alarm alarm1;
clock clock1;

void setup(){
  lcd.begin(16, 2);
  Wire.begin(); 
  rtc.begin();
  pinMode(9,OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(A0, INPUT_PULLUP);
  digitalWrite(9,HIGH);

  if (!rtc.isrunning()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  lcd.print("DHCP...");
  
  lcd.setCursor(0,1);
  Ethernet.begin(mac);
  server.begin();
  lcd.print(Ethernet.localIP());
  clock1.display=1;
  clock1.printToLcd(0);
  //begin fuunkcije započinju rad sa stvarima
}

void loop(){
  tick=millis();
  //provjerava jeli prošla sekunda
  if(tick >= (tack+1000)){
    if(timer1.timerTime){
      timer1.timerTime --;
      if(timer1.timerTime == 0) timer1.off =1;
    }
    tack = millis();tack -= millis()%10;
    clock1.printToLcd(0);

    //pali alarm ako su minute i sati jednke i ako je 00 sekundi u satu sata 
    if(alarm1.time.hour() == clock1.time.hour() && alarm1.time.minute() == clock1.time.minute() && clock1.time.second()==0){
      alarm1.off = 1;
    }
  }

  if(digitalRead(A0) == LOW){
    //ako je tipka pritisnuta
    delay(10);
    if(alarm1.off){
      //ako je alarm uključen, gasi ga
      alarm1.off = 0;
      alarm1.display=0;
      lcd.setCursor(0,alarm1.line);
      lcd.print("                ");
    }
    if(timer1.off){
      //ako je timer uključen, gasi ga
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
        client.println("<br />");
        //http stvari koje nerazumijem, a trebaju
        if(updateTime) updateTime++; //ako je update time veći od nule povečaj ga još jednom zbog razloga
        if(clearIP){
          //ako piše ip adresa, obriši ju
          clearIP = 0;
          lcd.setCursor(0,1);
          lcd.print("                ");
        }
        if(dataString.startsWith("alarm")){
          String tempString=dataString.substring(dataString.indexOf("(")+1,dataString.indexOf(":"));
          long hour = tempString.toInt();
          tempString=dataString.substring(dataString.indexOf(":")+1,dataString.indexOf(")"));
          long minute = tempString.toInt();
          alarm1.begin(hour,minute);
          alarm1.display=1;
          alarm1.printToLcd(1);
          client.println(alarm1.data());
          client.println("<br />");
        } //uključivanje alarma putem weba
        else if(dataString.startsWith("timer")){
          String tempString=dataString.substring(dataString.indexOf("(")+1,dataString.indexOf(")"));
          long time = tempString.toInt();
          timer1.begin(time);
          timer1.display=1;
          timer1.printToLcd(1);
          client.println("TIMER: ");
          client.println(timer1.data());
          client.println("<br />");
        } //uključivanje alarma putem weba
        else if(dataString.startsWith("sw")){
          String tempString=dataString.substring(dataString.indexOf(".")+1,dataString.length());
          if(tempString == "start"){
            stopwatch1.start();
            stopwatch1.display=1;
          }
          else if (tempString == "stop"){
            stopwatch1.stop();
            client.println(stopwatch1.data());
            client.println("<br />");
          }
        }
        else if(dataString.startsWith("ntp")){
          String tempString=dataString.substring(dataString.indexOf("(")+1,dataString.indexOf(")"));
          ntpOfset = tempString.toInt();
          updateTime ++;
        }
        else if(dataString.length()){
          client.println("Wrong input: ");
          client.println(dataString);
          client.println("<br />");
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
      tone(6,5000);
      digitalWrite(9,0);
    }
    else {
      noTone(6);
      digitalWrite(9,1);
    }
  } // ako se aktivir alarm ili timer, svakih cca pol sec, toglaj backlight i pištanje
  else{
    noTone(6);
    digitalWrite(9,1);
  }
  if(updateTime == 2){
    updateTime=0;
  }
}
