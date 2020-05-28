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

#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <WebSocketsServer.h>    //https://github.com/Links2004/arduinoWebSockets
#include <StreamUtils.h>         //https://github.com/bblanchon/ArduinoStreamUtils
#include <PubSubClient.h>        //https://pubsubclient.knolleary.net/
#include <NTPClient.h>           //https://github.com/arduino-libraries/NTPClient
#include <TimeLib.h>             //https://playground.arduino.cc/Code/Time/
#include <ArduinoJson.h>         /*https://github.com/bblanchon/ArduinoJson 
                                   https://arduinojson.org/?utm_source=meta&utm_medium=library.properties */
#include "PirSensor.h";
#include "Relay.h";

#define GPIO_LED_WIFI 2     // номер пина светодиода GPIO2 (D4)
#define GPIO_LED_RED 15     // пин, красного светодиода 
#define GPIO_LED_GREEN 12   // пин, зеленого светодиода 
#define GPIO_LED_BLUE 13    // пин, синего светодиода
#define GPIO_BUTTON 4       // номер пина кнопки GPIO4 (D2)
#define GPIO_SENSOR1 14     // пин, вход датчика движения №1
#define GPIO_SENSOR2 5      // пин, вход датчика движения №2
#define GPIO_RELAY 16       // пин, выход управления реле

#define FILE_RELAY "/relay.txt"         //Имя файла для сохранения настроек и статистики РЕЛЕ
#define FILE_NETWORK "/net.txt"         //Имя файла для сохранения настроек сети

#define DEVICE_TYPE "esplink_ms_"
#define TIME_ATTEMP_CON_MQTT 5000       //время между попытками установки соединения с MQTT сервером, мс
#define TIMEOUT_T_broadcastTXT 100000   //таймаут отправки скоростных сообщений T_broadcastTXT, мкс
#define TIME_DELTA_timeESPOn 5000       //период прибавки времени к timeESPOn при подсчете timeESPOn

#define DEFAULT_AP_NAME "ESP"           //имя точки доступа запускаемой по кнопке
#define DEFAULT_AP_PASS "11111111"      //пароль для точки доступа запускаемой по кнопке

#define TIME_NTP_OFFSET 10800           //время смещения часового пояса, сек (10800 = +3 часа)
#define TIME_NTP_UPDATE_INTERVAL 300000 //время синхронизации времени с NTP, мс (300000 = 5 мин)
#define NTP_SERVER "pool.ntp.org"       //адрес NTP сервера

#define PERIOD_SAVE_STAT 43200000       //периодичность сохранения статистики, мс

char *mqtt_server = "srv1.mqtt.4api.ru";
int mqtt_server_port = 9124;
String mqttUser = "user_889afb72";
String mqttPass = "pass_7c9ca39a";
String pubTopic;
unsigned int startMqttReconnectTime = 0;  //вспом. для отсчета времени переподключения к mqtt

bool wifiAP_mode = 0;
bool static_IP = 0;
byte ip[4] = {192, 168, 1, 43};
byte sbnt[4] = {255, 255, 255, 0};
byte gtw[4] = {192, 168, 1, 1};
char *p_ssid = new char[0];
char *p_password = new char[0];
char *p_ssidAP = new char[0];
char *p_passwordAP = new char[0];

bool sendSpeedDataEnable[] = {0, 0, 0, 0, 0};
String ping = "ping";
unsigned int speedT = 200;  //период отправки данных, миллисек

//Statistic variables
double timeESPOn = 0;                   //время с момента включения устройства, мс
int startTimeESPOn = 0;                 //вспом. для timeESPOn
unsigned int startTimeSaveStat = 0;     //вспом. для timeSaveStat


int tm1;

WebSocketsServer webSocket(81);
ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient mqtt(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, TIME_NTP_OFFSET, TIME_NTP_UPDATE_INTERVAL);

PirSensor pirSensor0(GPIO_SENSOR1);
PirSensor pirSensor1(GPIO_SENSOR2);
Relay relay(GPIO_RELAY, MODE_AUTO);


void setup() {
  Serial.begin(115200);
  Serial.println("\n");
  pinMode(GPIO_LED_WIFI, OUTPUT);
  pinMode(GPIO_LED_GREEN, OUTPUT);
  pinMode(GPIO_BUTTON, INPUT_PULLUP);
  pinMode(GPIO_SENSOR1, INPUT_PULLUP);
  pinMode(GPIO_SENSOR2, INPUT_PULLUP);
  pinMode(GPIO_RELAY, OUTPUT);
  digitalWrite(GPIO_LED_WIFI, HIGH);
  digitalWrite(GPIO_LED_GREEN, LOW);
  digitalWrite(GPIO_RELAY, 1);

  //printChipInfo();

  SPIFFS.begin();
  scanAllFile();
  printFile(FILE_RELAY);
  printFile(FILE_NETWORK);

  loadFile(FILE_RELAY);

  //Запуск точки доступа с параметрами поумолчанию
  if ( !loadFile(FILE_NETWORK) ||  digitalRead(GPIO_BUTTON) == 0)  startAp(DEFAULT_AP_NAME, DEFAULT_AP_PASS);
  //Запуск точки доступа
  else if (digitalRead(GPIO_BUTTON) == 1 && wifiAP_mode == 1)   startAp(p_ssidAP, p_passwordAP);
  //Запуск подключения клиента к точке доступа
  else if (digitalRead(GPIO_BUTTON) == 1 && wifiAP_mode == 0) {
    if (WiFi.getPersistent() == true)    WiFi.persistent(false);
    WiFi.softAPdisconnect(true);
    WiFi.persistent(true);
    if (WiFi.SSID() != p_ssid || WiFi.psk() != p_password) {
      Serial.println(F("\nCHANGE password or ssid"));
      WiFi.disconnect();
      WiFi.begin(p_ssid, p_password);
    }
    if (static_IP == 1) {
      set_staticIP();
    }
  }

  webServer_init();      //инициализация HTTP сервера
  webSocket_init();      //инициализация webSocket сервера
  mqtt_init();           //инициализация mqtt клиента

  startTimeSaveStat = millis();
  startTimeESPOn = millis();
  startMqttReconnectTime = millis();

  tm1 = millis();
}



void loop() {
  if (millis() - tm1 > 1000) {
    //Serial.print("\nRELAY.readPirSensor(0) = ");          Serial.println(relay.readPirSensor(0));
    //Serial.print("RELAY.readPirSensor(1) = ");            Serial.println(relay.readPirSensor(1));
    //Serial.print("RELAY.relayState = ");                 Serial.println(relay.relayState);
    //Serial.print("RELAY.relayMode = ");                  Serial.println(relay.relayMode);
    //Serial.print("RELAY.sumSwitchingOn = ");       Serial.println(relay.sumSwitchingOn);
    //Serial.print("RELAY.totalTimeOn = ");       Serial.println(relay.totalTimeOn);
    tm1 = millis();
  }

  relay.update();
  wifi_init();
  webSocket.loop();
  server.handleClient();
  MDNS.update();
  mqttConnect();
  if (WiFi.status() == WL_CONNECTED)  timeClient.update();


   //Отправка Speed данных клиентам при условии что данныее обновились и клиенты подключены
  if (relay.dataUpdateBit == 1) {
    if (mqtt.connected()) {
      sendToMqttServer(serializationToJson_index());
    }
    if (sendSpeedDataEnable[0] || sendSpeedDataEnable[1] || sendSpeedDataEnable[2] || sendSpeedDataEnable[3] || sendSpeedDataEnable[4] ) {
      String data = serializationToJson_index();
      int startT_broadcastTXT = micros();
      webSocket.broadcastTXT(data);
      int T_broadcastTXT = micros() - startT_broadcastTXT;
      if (T_broadcastTXT > TIMEOUT_T_broadcastTXT)  checkPing();
    }
    relay.dataUpdateBit = 0;
  }

  //подсчет времени с момента включения устройства
  if (millis() - startTimeESPOn > TIME_DELTA_timeESPOn) {
    timeESPOn += (millis() - startTimeESPOn);
    startTimeESPOn = millis();
  }

  //Сохранение stat данных в файл с периодичностью timeSaveStat
  if (millis() - startTimeSaveStat > PERIOD_SAVE_STAT) {
    saveFile(FILE_RELAY);
    startTimeSaveStat = millis();
  }

}
