/*
   Добавил файловый менеджер
   Добавил обновление через вебстраницу "Системные настройки"
   Автозапуск АР Default, если не найден файл настроек
   Подключение внешних сигналов:
    - GPIO14 (D5) - вход датчика движения №1;
    - GPIO5 (D1) - вход датчика движения №2;
    - GPIO16 (D0) - выход управления реле.
   Подключение внутренних элементов (для платы типа ESP8266 Witty):
    - GPIO2 (D4) - голубой wifi светодиод;
    - GPIO4 (D2) - кнопка, подключена к пину (для платы типа ESP8266 Witty);
    - GPIO12 (D6) - зеленый цвет RGB-светодиода (для платы типа ESP8266 Witty);
    - GPIO13 (D7) - синий цвет RGB-светодиода (для платы типа ESP8266 Witty);
    - GPIO15 (D8) - красный цвет RGB-светодиода (для платы типа ESP8266 Witty).
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <WebSocketsServer.h>
#include <FS.h>
#include <ArduinoJson.h>

#define LED_WIFI 2     // номер пина светодиода GPIO2 (D4)
#define LED_RED 15     // пин, красного светодиода 
#define LED_GREEN 12   // пин, зеленого светодиода 
#define LED_BLUE 13    // пин, синего светодиода
#define BUTTON 4       // номер пина кнопки GPIO4 (D2)
#define SENSOR1 14     // пин, вход датчика движения №1
#define SENSOR2 5      // пин, вход датчика движения №2
#define RELAY 16       // пин, выход управления реле

bool wifiAP_mode = 0;
char *p_ssidAP = "AP";             //SSID-имя вашей сети
char *p_passwordAP = "12345678";
char *p_ssid = "lamp";
char *p_password = "1234567890lamp";
byte ip[4] = {192, 168, 1, 43};
byte sbnt[4] = {255, 255, 255, 0};
byte gtw[4] = {192, 168, 1, 1};

bool sendSpeedDataEnable[] = {0, 0, 0, 0, 0};
String ping = "ping";
unsigned int speedT = 200;  //период отправки данных, миллисек

int relayMode = 2;        //режим работы, 0-откл, 1-вкл, 2-авто
bool relayState = 0;      //состояние реле 0-off, 1-on
bool relayStatePrev = 0;
bool sensor1State = 0;    //состояние сенсора №1: 0-off, 1-on
bool sensor1StatePrev = 0;
bool sensor2State = 0;    //состояние сенсора №2: 0-off, 1-on
bool sensor2StatePrev = 0;
bool sensor1Use = 1;      //использование сенсора №1. 0-off, 1-on
bool sensor1UsePrev = 0;
bool sensor2Use = 0;      //использование сенсора №2. 0-off, 1-on
bool sensor2UsePrev = 0;


WebSocketsServer webSocket(81);
ESP8266WebServer server(80);
//int timeT1 = millis();


void setup() {
  Serial.begin(115200);
  Serial.println("");
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(SENSOR1, INPUT_PULLUP);
  pinMode(SENSOR2, INPUT_PULLUP);
  pinMode(RELAY, OUTPUT);
  digitalWrite(LED_WIFI, HIGH);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(RELAY, LOW);
  //printChipInfo();

  SPIFFS.begin();
  //scanAllFile();
  //saveConfiguration();
  //printFile("/config.txt");

  //Запуск точки доступа с параметрами поумолчанию
  if ( !loadConfiguration() ||  digitalRead(BUTTON) == 0)  startAp("ESP", "11111111");
  //Запуск точки доступа
  else if (digitalRead(BUTTON) == 1 && wifiAP_mode == 1)   startAp(p_ssidAP, p_passwordAP);
  //Запуск подключения клиента к точке доступа
  else if (digitalRead(BUTTON) == 1 && wifiAP_mode == 0) {
    if (WiFi.getPersistent() == true)    WiFi.persistent(false);
    WiFi.softAPdisconnect(true);
    WiFi.persistent(true);
    if (WiFi.SSID() != p_ssid || WiFi.psk() != p_password) {
      Serial.println(F("\nCHANGE password or ssid"));
      WiFi.disconnect();
      set_staticIP();
      WiFi.begin(p_ssid, p_password);
    } else {
      set_staticIP();
    }
  }

  webServer_init();      //инициализация HTTP интерфейса
  webSocket_init();      //инициализация webSocket интерфейса

  sensor1Use = 1;
  sensor2Use = 0;
}


void loop() {
  //wifi_init();
  webSocket.loop();
  server.handleClient();

  if (sensor1Use == 1 && sensor2Use == 0) {
    relayState = digitalRead(SENSOR1);
  } else if (sensor1Use == 0 && sensor2Use == 1) {
    relayState = digitalRead(SENSOR2);
  } else if (sensor1Use == 1 && sensor2Use == 1) {
    relayState = digitalRead(SENSOR1) && digitalRead(SENSOR2);
  }

  if (relayState == 1) {
    digitalWrite(RELAY, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_WIFI, 0);
  } else {
    digitalWrite(RELAY, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_WIFI, 1);
  }

  if (relayStatePrev != relayState ) {
    Serial.print("relayState = ");
    Serial.println(relayState);
  }

  //Serial.println(impulsFreq);
  //Отправка Speed данных клиентам каждые speedT миллисекунд, при условии что данныее обновились и клиенты подключены
  if (true) {
    if (sendSpeedDataEnable[0] || sendSpeedDataEnable[1] || sendSpeedDataEnable[2] || sendSpeedDataEnable[3] || sendSpeedDataEnable[4] ) {
      if (relayStatePrev != relayState ) {
        String data = "{\"relayState\":";
        data += relayState;
        data += "}";
        int startT_broadcastTXT = micros();
        webSocket.broadcastTXT(data);
        int T_broadcastTXT = micros() - startT_broadcastTXT;
        if (T_broadcastTXT > 100000)  checkPing();
      }
    }
  }
  relayStatePrev = relayState;
}


//Print configuration parameters
void printConfiguration () {
  Serial.println(F("Print ALL VARIABLE:"));
  Serial.print(F("wifiAP_mode="));  Serial.println(wifiAP_mode);
  Serial.print(F("p_ssidAP="));     Serial.println(p_ssidAP);
  Serial.print(F("p_passwordAP=")); Serial.println(p_passwordAP);
  Serial.print(F("p_ssid="));       Serial.println(p_ssid);
  Serial.print(F("p_password="));   Serial.println(p_password);
  Serial.print(F("ip="));    Serial.print(ip[0]);   Serial.print(":");  Serial.print(ip[1]);  Serial.print(":");  Serial.print(ip[2]);  Serial.print(":");  Serial.println(ip[3]);
  Serial.print(F("sbnt="));  Serial.print(sbnt[0]); Serial.print(":");  Serial.print(sbnt[1]);  Serial.print(":");  Serial.print(sbnt[2]);  Serial.print(":");  Serial.println(sbnt[3]);
  Serial.print(F("gtw="));   Serial.print(gtw[0]);  Serial.print(":");  Serial.print(gtw[1]);  Serial.print(":");  Serial.print(gtw[2]);  Serial.print(":");  Serial.println(gtw[3]);
}

void printChipInfo() {
  Serial.print(F("\n<-> LAST RESET REASON: "));  Serial.println(ESP.getResetReason());
  Serial.print(F("<-> ESP8266 CHIP ID: "));      Serial.println(ESP.getChipId());
  Serial.print(F("<-> CORE VERSION: "));         Serial.println(ESP.getCoreVersion());
  Serial.print(F("<-> SDK VERSION: "));          Serial.println(ESP.getSdkVersion());
  Serial.print(F("<-> CPU FREQ MHz: "));         Serial.println(ESP.getCpuFreqMHz());
  Serial.print(F("<-> FLASH CHIP ID: "));        Serial.println(ESP.getFlashChipId());
  Serial.print(F("<-> FLASH CHIP SIZE: "));      Serial.println(ESP.getFlashChipSize());
  Serial.print(F("<-> FLASH CHIP REAL SIZE: ")); Serial.println(ESP.getFlashChipRealSize());
  Serial.print(F("<-> FLASH CHIP SPEED: "));     Serial.println(ESP.getFlashChipSpeed());
  Serial.print(F("<-> FREE MEMORY: "));          Serial.println(ESP.getFreeHeap());
  Serial.print(F("<-> SKETCH SIZE: "));          Serial.println(ESP.getSketchSize());
  Serial.print(F("<-> FREE SKETCH SIZE: "));     Serial.println(ESP.getFreeSketchSpace());
  Serial.print(F("<-> CYCLE COUNTD: "));         Serial.println(ESP.getCycleCount());
}
