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
#include <Debounce.h>            //https://github.com/wkoch/Debounce
#include <NTPClient.h>           //https://github.com/arduino-libraries/NTPClient
#include <TimeLib.h>             //https://playground.arduino.cc/Code/Time/
#include <ArduinoJson.h>         /*https://github.com/bblanchon/ArduinoJson 
                                   https://arduinojson.org/?utm_source=meta&utm_medium=library.properties */
                                 

#define GPIO_LED_WIFI 2     // номер пина светодиода GPIO2 (D4)
#define GPIO_LED_RED 15     // пин, красного светодиода 
#define GPIO_LED_GREEN 12   // пин, зеленого светодиода 
#define GPIO_LED_BLUE 13    // пин, синего светодиода
#define GPIO_BUTTON 4       // номер пина кнопки GPIO4 (D2)
#define GPIO_SENSOR1 14     // пин, вход датчика движения №1
#define GPIO_SENSOR2 5      // пин, вход датчика движения №2
#define GPIO_RELAY 16       // пин, выход управления реле
#define FILE_STAT "/stat.txt"
#define FILE_CONFIG "/config.txt"
#define DEVICE_TYPE "esplink_ms_"
#define TIME_ATTEMP_CON_MQTT 5000       //время между попытками установки соединения с MQTT сервером, мс
#define TIMEOUT_T_broadcastTXT 100000   //таймаут отправки скоростных сообщений T_broadcastTXT, мкс
#define TIME_DELTA_timeESPOn 5000       //период прибавки времени к timeESPOn при подсчете timeESPOn
#define DEFAULT_AP_NAME "ESP"           //имя точки доступа запускаемой по кнопке
#define DEFAULT_AP_PASS "11111111"      //пароль для точки доступа запускаемой по кнопке
#define TIME_NTP_OFFSET 10800           //время смещения часового пояса, сек (10800 = +3 часа)
#define TIME_NTP_UPDATE_INTERVAL 300000 //время синхронизации времени с NTP, мс (300000 = 5 мин)
#define NTP_SERVER "pool.ntp.org"       //адрес NTP сервера

bool wifiAP_mode = 0;
char *mqtt_server = "srv1.mqtt.4api.ru";
int mqtt_server_port = 9124;
String mqttUser = "user_889afb72";
String mqttPass = "pass_7c9ca39a";
String pubTopic;
bool static_IP = 0;
byte ip[4] = {192, 168, 1, 43};
byte sbnt[4] = {255, 255, 255, 0};
byte gtw[4] = {192, 168, 1, 1};

char *p_ssid = new char[0];
char *p_password = new char[0];
char *p_ssidAP = new char[0];
char *p_passwordAP = new char[0];
//char *p_ssid = "lamp";
//char *p_password = "1234567890lamp";
//char *p_ssidAP = p_ssidAP = "AP_ESP8266";             //SSID-имя вашей сети
//char *p_passwordAP = "12345678";

bool conIndic = 0;  //бит работы индикатора соединения, 1-вкл. 0-откл., данные отправляются каждые 2000 мс

bool sendSpeedDataEnable[] = {0, 0, 0, 0, 0};
String ping = "ping";
unsigned int speedT = 200;  //период отправки данных, миллисек
bool dataUpdateBit = 0;     //бит обновления данных, устанавливается когда данные обновлены и нужно отправить их клиенту

int relayMode = 2;        //режим работы, 0-откл, 1-вкл, 2-авто
bool relayState = 0;      //состояние реле 0-off, 1-on
bool sensor1State = 0;    //состояние сенсора №1: 0-off, 1-on
bool sensor2State = 0;    //состояние сенсора №2: 0-off, 1-on
bool sensor1Use = 1;      //использование сенсора №1. 0-off, 1-on
bool sensor2Use = 0;      //использование сенсора №2. 0-off, 1-on
unsigned int delayOff = 20000;     //Задержка отключения, мс
unsigned int startDelayOff = 0;    //вспом. для delayOff

bool preRelayState;
bool overflowControl = 0;     //Бит включающий контроль переполнения millis()

//Statistic variables
unsigned int numbOn = 0;                //количество включений
double timeRelayOn = 0;                 //время в состоянии реле ВКЛ, мс
unsigned int startTimeRelayOn = 0;      //вспом. для timeRelayOn
unsigned int mdTimeRelayOn = 0;         //максимальный промежуток времени включеного реле, мс
double timeESPOn = 0;                   //время с момента включения устройства, мс
int startTimeESPOn = 0;                 //вспом. для timeESPOn
unsigned int timeSaveStat = 43200000;   //периодичность сохранения статистики, мс
unsigned int startTimeSaveStat = 0;     //вспом. для timeSaveStat
unsigned int startMqttReconnectTime = 0;  //вспом. для отсчета времени переподключения к mqtt

bool flagSaveRstFile = false;

WebSocketsServer webSocket(81);
ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient mqtt(espClient);
Debounce Sensor1(GPIO_SENSOR1);
Debounce Sensor2(GPIO_SENSOR2);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, TIME_NTP_OFFSET, TIME_NTP_UPDATE_INTERVAL);


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
  //saveFile(FILE_CONFIG);
  //scanAllFile();
  //printFile(FILE_CONFIG);
  //printFile(FILE_STAT);

  loadFile(FILE_STAT);

  //Запуск точки доступа с параметрами поумолчанию
  if ( !loadFile(FILE_CONFIG) ||  digitalRead(GPIO_BUTTON) == 0)  startAp(DEFAULT_AP_NAME, DEFAULT_AP_PASS);
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
}



void loop() {
  wifi_init();
  webSocket.loop();
  server.handleClient();
  MDNS.update();
  mqttConnect();
  if (WiFi.status() == WL_CONNECTED)  timeClient.update();

  //Сохраняем в файл rst время и причину перезапуска при запуске контроллера
  if (!flagSaveRstFile && timeClient.getEpochTime() > 100000000) {
    saveRstInfoToFile();
    flagSaveRstFile = true;
  }


  //Обработка состояния сенсоров
  int prevSensorState = sensor1State;
  //sensor1State = digitalRead(GPIO_SENSOR1);
  sensor1State = Sensor1.read();
  if (prevSensorState != sensor1State)  dataUpdateBit = 1;
  prevSensorState = sensor2State;
  //sensor2State = digitalRead(GPIO_SENSOR2);
  sensor2State = Sensor2.read();
  if (prevSensorState != sensor2State)  dataUpdateBit = 1;


  //Определение состояния реле
  switch (relayMode) {
    case 0:
      relayState = 0;
      break;
    case 1:
      relayState = 1;
      break;
    case 2:
      if (sensor1Use == 1 && sensor2Use == 0) {
        preRelayState = sensor1State;
      } else if (sensor1Use == 0 && sensor2Use == 1) {
        preRelayState = sensor2State;
      } else if (sensor1Use == 1 && sensor2Use == 1) {
        preRelayState = sensor1State | sensor2State;
      }

      if (relayState == 0 && preRelayState == 1) {
        relayState = 1;
        startDelayOff = millis();
      }
      else if (relayState == 1 && preRelayState == 0 && millis() - startDelayOff > delayOff) {
        relayState = 0;
      }
      else if (preRelayState == 1) {
        startDelayOff = millis();
      }
      break;
  }

  //Изменение состояния реле
  if (digitalRead(GPIO_RELAY) == 0 && relayState == 0) {
    digitalWrite(GPIO_RELAY, 1);
    int deltaTimeRelayOn = millis() - startTimeRelayOn;
    timeRelayOn += deltaTimeRelayOn;
    if (deltaTimeRelayOn > mdTimeRelayOn)  mdTimeRelayOn = deltaTimeRelayOn;
    //digitalWrite(GPIO_LED_GREEN, 0);
    //digitalWrite(GPIO_LED_WIFI, 1);
    dataUpdateBit = 1;
  } else if (digitalRead(GPIO_RELAY) == 1 && relayState == 1) {
    digitalWrite(GPIO_RELAY, 0);
    startTimeRelayOn = millis();
    saveTimeOnRelay();
    //digitalWrite(GPIO_LED_GREEN, 1);
    //digitalWrite(GPIO_LED_WIFI, 0);
    numbOn ++;
    dataUpdateBit = 1;
  }

  //Отправка Speed данных клиентам при условии что данныее обновились и клиенты подключены
  if (dataUpdateBit == 1) {
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
    dataUpdateBit = 0;
  }

  //подсчет времени с момента включения устройства
  if (millis() - startTimeESPOn > TIME_DELTA_timeESPOn) {
    timeESPOn += (millis() - startTimeESPOn);
    startTimeESPOn = millis();
  }

  //Сохранение stat данных в файл с периодичностью timeSaveStat
  if (millis() - startTimeSaveStat > timeSaveStat) {
    saveFile(FILE_STAT);
    startTimeSaveStat = millis();
  }

  //Контроль переполнения таймера millis
  //Устанавливаем бит контроля переполнения таймера millis, при достижении им 4.200.000.000 из 4.294.967.296
  if (overflowControl == 0 && millis() > 4294000000) {
    overflowControl = 1;
  }
  //Если произошло переполнение millis
  if (overflowControl == 1 && millis() > 0 && millis() < 10000) {
    overflowControl = 0;
    startTimeSaveStat = millis();
    startTimeESPOn = millis();
    startTimeRelayOn = millis();
  }

}
