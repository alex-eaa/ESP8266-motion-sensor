/*
   ВНИМАНИЕ!!! При отсутствии интернет-соединения, из-за невозможности установить соединение
               с NTP-сервером, связь с контроллером через веб сильно тормозит
   
      Контроллер управления реле от двух PIR датчиков движения.

      Функционал контроллера:
   - Автоматический режим работы:
        - автоматическое включение реле при срабатывании одного из датчиков движения;
        - автоматическое отключение реле по истечении заданного времени при отсутствии движения.
   - Режим ручного управления, через вебинтерфейс.
   - Управление через вебинтерфейс управления реле позволяет:
        - выбирать режим работы автоматический или ручной,
        - включать/отключать реле,
        - задействовать один любой из датчиков движения или оба одновременно,
        - изменять время выдержки до автоматического отключения реле.
   - Ведение и сохранение статистики работы реле:
        - суммарное количество включений реле,
        - суммарное время работы реле в состоянии включено,
        - продолжительность непрерывной работы контроллера, с момента подачи на него напряжения,
        - максимальное время непрерывной работы реле в состоянии включено, с момента подачи на него напряжения.
   - Логирование в файл времени включения реле с фиксацией времени и номера датчика движения по которому произошло срабатывание.

   - Работа в режиме WIFI-точки доступа;
   - Работа в режиме WIFI-клиента для подключения к точке доступа;
   - автозапуск в режиме WIFI-точки доступа "По умолчанию" с настройками по умолчанию (для первичной настройки устройства);
   - mDNS-сервис на устройстве для автоматического определения IP-адреса устройства в сети;
   - NTP-клиент для синхронизации времени с сервером времени через интернет;
   - MQTT-клиент для удаленного облачного управления с помощью приложения Android;
   - Web сервер (порт: 80) на устройстве для подключения к устройству по сети из любого web-браузера;
   - Websocket сервер (порт: 81) на устройстве для обмена коммандами и данными между страницей в web-браузере и контроллером;
   - Страница управления и настройки реле (/index.htm);
   - Страница сетевых настроек (/setup.htm);
   - Страница файлового менеджера для просмотра, удаления и загрузки файлов (/edit.htm);
   - Страница обновления прошивки контроллера (/update.htm);

      Подключение PIR-датчиков движения и реле:
   - GPIO14 (D5) - Датчик №1
   - GPIO5 (D1)  - Датчик №2
   - GPIO16 (D0) - Управление реле

      Подключение других элементов (для платы типа ESP8266 Witty):
    - GPIO2 (D4) - голубой wifi светодиод;
    - GPIO4 (D2) - кнопка, подключена к пину (для платы типа ESP8266 Witty);
    - GPIO12 (D6) - зеленый цвет RGB-светодиода (для платы типа ESP8266 Witty);
    - GPIO13 (D7) - синий цвет RGB-светодиода (для платы типа ESP8266 Witty);
    - GPIO15 (D8) - красный цвет RGB-светодиода (для платы типа ESP8266 Witty).
*/

#define DEBUG 1

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
#define TIME_ATTEMP_CON_MQTT 30000      //время между попытками установки соединения с MQTT сервером, мс
#define TIMEOUT_T_broadcastTXT 100000   //таймаут отправки скоростных сообщений T_broadcastTXT, мкс
#define TIME_DELTA_timeESPOn 5000       //период прибавки времени к timeESPOn при подсчете timeESPOn

#define DEFAULT_AP_NAME "ESP"           //имя точки доступа запускаемой по кнопке
#define DEFAULT_AP_PASS "11111111"      //пароль для точки доступа запускаемой по кнопке

#define TIME_NTP_OFFSET 10800           //время смещения часового пояса, сек (10800 = +3 часа)
#define TIME_NTP_UPDATE_INTERVAL 600000 //время синхронизации времени с NTP, мс (600000 = 10 мин)
#define NTP_SERVER "pool.ntp.org"       //адрес NTP сервера

#define PERIOD_SAVE_STAT 43200000       //периодичность сохранения статистики, мс


bool flagMQTT = 0;
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

bool flagLog = 0;

bool sendSpeedDataEnable[] = {0, 0, 0, 0, 0};
String ping = "ping";
unsigned int speedT = 200;  //период отправки данных, миллисек

//Statistic variables
double timeESPOn = 0;                   //время с момента включения устройства, мс
int startTimeESPOn = 0;                 //вспом. для timeESPOn
unsigned int startTimeSaveStat = 0;     //вспом. для timeSaveStat


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
  pinMode(GPIO_BUTTON, INPUT_PULLUP);
  pinMode(GPIO_SENSOR1, INPUT_PULLUP);
  pinMode(GPIO_SENSOR2, INPUT_PULLUP);
  pinMode(GPIO_RELAY, OUTPUT);
  digitalWrite(GPIO_LED_WIFI, HIGH);
  digitalWrite(GPIO_RELAY, 1);

  SPIFFS.begin();
#ifdef DEBUG
  printChipInfo();
  scanAllFile();
  printFile(FILE_RELAY);
  printFile(FILE_NETWORK);
#endif

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
  if (flagMQTT == 1)    mqtt_init();           //инициализация mqtt клиента

  relay.addFun(saveTimeOnRelay);

  startTimeSaveStat = millis();
  startTimeESPOn = millis();
  startMqttReconnectTime = millis();
}



void loop() {
  pirSensor0.update();
  pirSensor1.update();
  relay.update();
  wifi_init();
  webSocket.loop();
  server.handleClient();
  MDNS.update();
  if (flagMQTT == 1)   mqttConnect();
  if (WiFi.status() == WL_CONNECTED && flagLog == 1)   timeClient.update();


  //Отправка Speed данных клиентам при условии что данныее обновились и клиенты подключены
  if (updateDataUpdateBits() == 1) {
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
    resetDataUpdateBits();
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


//Проверка флага изменения состояния объектов
bool updateDataUpdateBits() {
  return relay.dataUpdateBit | pirSensor0.dataUpdateBit | pirSensor1.dataUpdateBit;
}


//Сброс флагов изменения состояния объектов
void resetDataUpdateBits() {
  relay.dataUpdateBit = 0;
  pirSensor0.dataUpdateBit = 0;
  pirSensor1.dataUpdateBit = 0;
}
