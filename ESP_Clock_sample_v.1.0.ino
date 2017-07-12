/**
* Часы и информер погоды на матричном дисплее 16x32
* 
* Контроллер ESP8266F (4Мбайт)
* Графический экран 16x32 на 8 матрицах MAX8219
* 
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/
#include <arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <SPI.h>
// https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_GFX.h>
// https://github.com/markruys/arduino-Max72xxPanel
#include <Max72xxPanel.h>
https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>

const unsigned char logo2 [] PROGMEM = {
0xff, 0xff, 0xdf, 0xfd, 0xcf, 0xf9, 0xc7, 0xf1, 0xc0, 0x01, 0xe0, 0x03, 0xe0, 0x03, 0xc2, 0x11, 
0xc7, 0x39, 0xc2, 0x11, 0x80, 0x01, 0x00, 0xc1, 0x00, 0x03, 0x00, 0x03, 0x00, 0x07, 0x00, 0x0f };

const unsigned char wifi_icon [] PROGMEM = {0x07, 0xfb, 0xfd, 0x1e, 0xee, 0xf6, 0x36, 0xb6 };

const unsigned char digit0 [] PROGMEM = {  0x83,0x01,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x01,0x83 };
const unsigned char digit1 [] PROGMEM = {  0xF7,0xE7,0xC7,0x87,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0xE7,0x81 };
const unsigned char digit2 [] PROGMEM = {  0x83,0x01,0x39,0x39,0xF9,0xF9,0xF3,0xE3,0xC7,0x8F,0x1F,0x3F,0x01,0x01 };
const unsigned char digit3 [] PROGMEM = {  0x83,0x01,0x39,0xF9,0xF9,0xF9,0xE3,0xE3,0xF9,0xF9,0xF9,0x39,0x01,0x83 };
const unsigned char digit4 [] PROGMEM = {  0xF3,0xE3,0xC3,0xC3,0x93,0x93,0x33,0x31,0x01,0x01,0xF3,0xF3,0xF3,0xF3 };
const unsigned char digit5 [] PROGMEM = {  0x01,0x01,0x3F,0x3F,0x3F,0x03,0x01,0xF9,0xF9,0xF9,0xF9,0x39,0x01,0x83 };
const unsigned char digit6 [] PROGMEM = {  0x83,0x01,0x39,0x3F,0x3F,0x03,0x01,0x39,0x39,0x39,0x39,0x39,0x01,0x83};
const unsigned char digit7 [] PROGMEM = {  0x01,0x01,0xF9,0xF9,0xF9,0xF1,0xF3,0xE3,0xE7,0xCF,0xCF,0x9F,0x9F,0x9F };
const unsigned char digit8 [] PROGMEM = {  0x83,0x01,0x39,0x39,0x39,0x83,0x83,0x39,0x39,0x39,0x39,0x39,0x01,0x83 };
const unsigned char digit9 [] PROGMEM = {  0x83,0x01,0x39,0x39,0x39,0x39,0x39,0x01,0x81,0xF9,0xF9,0x39,0x01,0x83 };

// Параметры доступа к WiFi
const char W_SSID[] = "Ваш WiFi";
const char W_PASS[] = "Пароль WiFi";
// Параметры погодного сервера
String     W_URL    = "http://api.openweathermap.org/data/2.5/weather";
//IPPID. Получить бесплатно: http://openweathermap.org/appid#get
String     W_API    = "Нлюч нужно получить на сайте";
// Код города перми на сервере openweathermap.org
// Москва = 524901
// Остальные города можно посмотреть http://bulk.openweathermap.org/sample/city.list.json.gz
String     W_ID     = "511196";
String     W_NAME   = "В Перми ";

WiFiUDP udp;
const int NTP_PACKET_SIZE = 48; 
byte packetBuffer[ NTP_PACKET_SIZE]; 
const char NTP_SERVER[]   = "0.ru.pool.ntp.org";          
int TZ                    = 5;//Таймзона для Перми
uint32_t NTP_TIMEOUT      = 600000; //10 минут





int pinCS = 16; // Attach CS to this pin, DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI )
int numberOfHorizontalDisplays = 4;
int numberOfVerticalDisplays   = 2;

Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);


String tape = "";
int wait = 20; // In milliseconds

int spacer = 1;
int width = 5 + spacer; // The font width is 5 pixels

uint32_t ms, ms0=0, ms1=0, ms2=0, ms3=0, ms_mode=0;
uint32_t tm         = 0;
uint32_t t_cur      = 0;    
long  t_correct     = 0;
bool  pp = false;
int   mode = 0;

void setup() {
   Serial.begin(115200);
 
// Настройка дисплея
    matrix.setIntensity(7); // Use a value between 0 and 15 for brightness

// Порядок матриц
    matrix.setPosition(0, 3, 1); // The first display is at <0, 0>
    matrix.setPosition(1, 2, 1); // The first display is at <0, 0>
    matrix.setPosition(2, 1, 1); // The first display is at <0, 0>
    matrix.setPosition(3, 0, 1); // The first display is at <0, 0>
    matrix.setPosition(4, 3, 0); // The first display is at <0, 0>
    matrix.setPosition(5, 2, 0); // The first display is at <0, 0>
    matrix.setPosition(6, 1, 0); // The first display is at <0, 0>
    matrix.setPosition(7, 0, 0); // The first display is at <0, 0>
// Ориентация каждой матрицы
   matrix.setRotation(0, 3);    // The first display is position upside down
   matrix.setRotation(1, 3);    // The first display is position upside down
   matrix.setRotation(2, 3);    // The first display is position upside down
   matrix.setRotation(3, 3);    // The first display is position upside down
   matrix.setRotation(4, 3);    // The first display is position upside down
   matrix.setRotation(5, 3);    // The first display is position upside down
   matrix.setRotation(6, 3);    // The first display is position upside down
   matrix.setRotation(7, 3);    // The first display is position upside down

   matrix.fillScreen(LOW);

   
   matrix.drawBitmap(0, 0,  logo2, 16, 16, 0, 1); 

   matrix.write();
   delay(5000); 
// Содиняемся с WiFi
  Serial.print("\nConnecting to ");
  Serial.println(W_SSID);

  WiFi.begin(W_SSID, W_PASS);
  for (int i=0;WiFi.status() != WL_CONNECTED&&i<150; i++) {
     Serial.print(".");
// Минаем значком WiFi
     matrix.drawBitmap(20, 4,  wifi_icon, 8, 8, 0, 1); 
     matrix.write();
     delay(500);
     matrix.fillRect(20, 4, 8, 8, LOW);  
     matrix.write();
     delay(500);
  }
/*
// Содиняемся с WiFi
  Serial.print("\nConnecting to ");
  Serial.println(W_SSID);

  WiFi.begin(W_SSID1, W_PASS1);
  for (int i=0;WiFi.status() != WL_CONNECTED&&i<15; i++) {
     Serial.print(".");
// Минаем значком WiFi
     matrix.drawBitmap(20, 4,  wifi_icon, 8, 8, 0, 1); 
     matrix.write();
     delay(500);
     matrix.fillRect(20, 4, 8, 8, LOW);  
     matrix.write();
     delay(500);
  }
*/
// Выдаем значек WiFi
   matrix.drawBitmap(20, 4,  wifi_icon, 8, 8, 0, 1); 
   matrix.write();

  Serial.println("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
   
   ms_mode = millis();
// Cоединение для NTP   
   udp.begin(2390);
}

int i_ms0 = 0;
void loop() {
   ms = millis();

// Цикл бегущей строки   

   if( ms0 == 0 || ms < ms0 || (ms - ms0)>wait ){
       ms0 = ms;
       switch(mode){
           case 0:
           case 1:
           case 4:              
           case 5:              
              break;
           default:
              PrintTicker();
       }
       
   }
   if( ms1 == 0 || ms < ms1 || (ms - ms1)>60000 ){
       ms1 = ms;
       GetWeather();
   }
// Показ часов
  if( ms2 == 0 || ms < ms2 || (ms - ms2)>500 ){
       ms2 = ms;
       t_cur  = ms/1000;
       tm     = t_cur + t_correct;
       switch(mode){
           case 0:
           case 1:
           case 4:              
           case 5:              
              PrintBigTime();
              break;
           default:
              PrintTime();
       }
  
       
  }
   
// Опрос NTP сервера
   if(  ms3 == 0 || ms < ms3 || (ms - ms3)>NTP_TIMEOUT ){
       uint32_t t = GetNTP();
       if( t!=0 ){
          ms3 = ms;
          t_correct = t - t_cur;
       }
       Serial.println(t);
   }

   
}

/**
 * Печать времени
 */
void PrintTime(){
    char s[20];
    matrix.fillRect(0, 0, 32, 8, LOW);  
//    matrix.setFont(&Picopixel);     
    if( pp )sprintf(s, "%02d:%02d", (int)( tm/3600 )%24, (int)( tm/60 )%60);
    else sprintf(s, "%02d %02d", (int)( tm/3600 )%24, (int)( tm/60 )%60);
    pp = !pp;
    matrix.setCursor(1, 0);
    matrix.print(s);
    ms_mode = ms;
    
}

/**
 * Печать времени большими цифрами
 */
void PrintBigTime(){
    char s[20];
    matrix.fillScreen(LOW);  
    int h = (int)( tm/3600 )%24;
    int m = (int)( tm/60 )%60;
    PrintBigDigit(0,1,h/10);
    PrintBigDigit(8,1,h%10);
    PrintBigDigit(17,1,m/10);
    PrintBigDigit(25,1,m%10);
    if( pp ){
      matrix.fillRect(15, 4,  2, 2, HIGH);
      matrix.fillRect(15, 9, 2, 2, HIGH);
    }
    
    pp = !pp;
    matrix.write();
    if( ms -ms_mode > 5000){
         mode++;
         if( mode >= 8 )mode = 0;
         ms_mode = ms;
    }
    
}

/**
 * Печать большой цифры
 */
 void PrintBigDigit(int x, int y, int dig){
    switch(dig){
      case 0  : matrix.drawBitmap(x, y,  digit0, 7, 14, 0, 1); break;
      case 1  : matrix.drawBitmap(x, y,  digit1, 7, 14, 0, 1); break;
      case 2  : matrix.drawBitmap(x, y,  digit2, 7, 14, 0, 1); break;
      case 3  : matrix.drawBitmap(x, y,  digit3, 7, 14, 0, 1); break;
      case 4  : matrix.drawBitmap(x, y,  digit4, 7, 14, 0, 1); break;
      case 5  : matrix.drawBitmap(x, y,  digit5, 7, 14, 0, 1); break;
      case 6  : matrix.drawBitmap(x, y,  digit6, 7, 14, 0, 1); break;
      case 7  : matrix.drawBitmap(x, y,  digit7, 7, 14, 0, 1); break;
      case 8  : matrix.drawBitmap(x, y,  digit8, 7, 14, 0, 1); break;
      case 9  : matrix.drawBitmap(x, y,  digit9, 7, 14, 0, 1); break;
    }
 }

/**'
 * Один такт бегущей строки
 */
void PrintTicker(){
//     matrix.setFont();
     if( i_ms0 >= width * tape.length() + matrix.width() - 1 - spacer ){
         i_ms0 = 0;
         mode++;
         ms_mode = ms;
         if( mode >=8 )mode=0;
     }

//       for ( int i = 0 ; i < width * tape.length() + matrix.width() - 1 - spacer; i++ ) {
//         matrix.fillScreen(LOW);
         matrix.fillRect(0, 8, 32, 8, LOW);

         int letter = i_ms0 / width;
         int x = (matrix.width() - 1) - i_ms0 % width;
         int y = 8; // center the text vertically

         while ( x + width - spacer >= 0 && letter >= 0 ) {
            if ( letter < tape.length() ) {
               matrix.drawChar(x, y, tape[letter], HIGH, LOW, 1);
           }

         letter--;
         x -= width;
      }

      matrix.write(); // Send bitmap to display

//      delay(wait);
//      }
     i_ms0++;

  
}

/*
 * Перекодировка UTF8 в Windows-1251
 */
String utf8rus(String source)
{
  int i,k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };

  k = source.length(); i = 0;

  while (i < k) {
    n = source[i]; i++;

    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
          n = source[i]; i++;
          if (n == 0x81) { n = 0xA8; break; }
//          if (n >= 0x90 && n <= 0xBF) n = n + 0x30;
          if (n >= 0x90 && n <= 0xBF) n = n + 0x2F;
          break;
        }
        case 0xD1: {
          n = source[i]; i++;
          if (n == 0x91) { n = 0xB8; break; }
//          if (n >= 0x80 && n <= 0x8F) n = n + 0x70;
          if (n >= 0x80 && n <= 0x8F) n = n + 0x6F;
          break;
        }
      }
    }
    m[0] = n; target = target + String(m);
  }
return target;
}

/**
 * Получаем погоду с погодного сервера
 */
void GetWeather(){
   HTTPClient client;
   String url = W_URL;
   url += "?id=";
   url += W_ID;
   url += "&APPID=";
   url += W_API;
   url += "&units=metric&lang=ru";
   Serial.println(url);
   client.begin(url);
   int httpCode = client.GET();
   if( httpCode == HTTP_CODE_OK ){
       String httpString = client.getString(); 
       ParseWeather(httpString);
   }
   client.end();

}

/**
 * Парсим погоду
 */
void ParseWeather(String s){
   DynamicJsonBuffer jsonBuffer;
   JsonObject& root = jsonBuffer.parseObject(s);

   if (!root.success()) {
      Serial.println("Json parsing failed!");
      return;
   }
// Погода   
   tape = "В Перми ";
   tape += root["weather"][0]["description"].as<String>();
// Температура   
   tape += ". Температура ";
   int t = root["main"]["temp"].as<int>(); 
   tape += String(t);
// Влажность   
   tape += "С. Влажность ";
   tape += root["main"]["humidity"].as<String>();
// Давление   
   tape += "%. Давление ";
   double p = root["main"]["pressure"].as<double>()/1.33322;
   tape += String((int)p);
// Ветер   
   tape += "мм. Ветер ";
   double deg = root["wind"]["deg"];
   if( deg >22.5 && deg <=67.5 )tape += "северо-восточный ";
   else if( deg >67.5 && deg <=112.5 )tape += "восточный ";
   else if( deg >112.5 && deg <=157.5 )tape += "юго-восточный ";
   else if( deg >157.5 && deg <=202.5 )tape += "южный ";
   else if( deg >202.5 && deg <=247.5 )tape += "юго-западный ";
   else if( deg >247.5 && deg <=292.5 )tape += "западный ";
   else if( deg >292.5 && deg <=337.5 )tape += "северо-западный ";
   else tape += "северный ";
   tape += root["wind"]["speed"].as<String>();
   tape += " м/с.    ";
// Перекодируем из UNICODE   
   tape = utf8rus(tape);
   s = ""; 
}

/**
 * Посылаем и парсим запрос к NTP серверу
 */
time_t GetNTP(void) {
  IPAddress ntpIP;
  time_t tm = 0;
  WiFi.hostByName(NTP_SERVER, ntpIP); 
  sendNTPpacket(ntpIP); 
  delay(1000);
 
  int cb = udp.parsePacket();
  if (!cb) {
    return tm;
  }
  else {
// Читаем пакет в буфер    
    udp.read(packetBuffer, NTP_PACKET_SIZE); 
// 4 байта начиная с 40-го сождержат таймстамп времени - число секунд 
// от 01.01.1900   
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
// Конвертируем два слова в переменную long
    unsigned long secsSince1900 = highWord << 16 | lowWord;
// Конвертируем в UNIX-таймстамп (число секунд от 01.01.1970
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;
// Делаем поправку на местную тайм-зону
    tm = epoch + TZ*3600;    
  }
  return tm;
}

/**
 * Посылаем запрос NTP серверу на заданный адрес
 */
unsigned long sendNTPpacket(IPAddress& address)
{
// Очистка буфера в 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
// Формируем строку зыпроса NTP сервера
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
// Посылаем запрос на NTP сервер (123 порт)
  udp.beginPacket(address, 123); 
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}



