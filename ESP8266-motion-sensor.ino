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
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <WebSocketsServer.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <PubSubClient.h>
#include <Debounce.h>

#define LED_WIFI_GPIO 2     // номер пина светодиода GPIO2 (D4)
#define LED_RED_GPIO 15     // пин, красного светодиода 
#define LED_GREEN_GPIO 12   // пин, зеленого светодиода 
#define LED_BLUE_GPIO 13    // пин, синего светодиода
#define BUTTON_GPIO 4       // номер пина кнопки GPIO4 (D2)
#define SENSOR1_GPIO 14     // пин, вход датчика движения №1
#define SENSOR2_GPIO 5      // пин, вход датчика движения №2
#define RELAY_GPIO 16       // пин, выход управления реле
#define STAT_FILE "/stat.txt"
#define CONFIG_FILE "/config.txt"
#define DEVICE_TYPE "esplink_ms_"
#define TIME_ATTEMP_CON_MQTT 5000   //время между попытками установки соединения с MQTT сервером

bool wifiAP_mode = 0;
char *p_ssidAP = "AP";             //SSID-имя вашей сети
char *p_passwordAP = "12345678";
char *p_ssid = "lamp";
char *p_password = "1234567890lamp";
char* mqtt_server = "srv1.mqtt.4api.ru";
int mqtt_server_port = 9124;
String mqttUser = "user_889afb72";
String mqttPass = "pass_7c9ca39a";
String pubTopic;
bool static_IP = 0;
byte ip[4] = {192, 168, 1, 43};
byte sbnt[4] = {255, 255, 255, 0};
byte gtw[4] = {192, 168, 1, 1};

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

WebSocketsServer webSocket(81);
ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient mqtt(espClient);
Debounce Sensor1(SENSOR1_GPIO);
Debounce Sensor2(SENSOR2_GPIO);

void setup() {
  Serial.begin(115200);
  Serial.println("\n");
  pinMode(LED_WIFI_GPIO, OUTPUT);
  pinMode(LED_GREEN_GPIO, OUTPUT);
  pinMode(BUTTON_GPIO, INPUT_PULLUP);
  pinMode(SENSOR1_GPIO, INPUT_PULLUP);
  pinMode(SENSOR2_GPIO, INPUT_PULLUP);
  pinMode(RELAY_GPIO, OUTPUT);
  digitalWrite(LED_WIFI_GPIO, HIGH);
  digitalWrite(LED_GREEN_GPIO, LOW);
  digitalWrite(RELAY_GPIO, 1);
  printChipInfo();

  SPIFFS.begin();
  scanAllFile();
  printFile(CONFIG_FILE);
  printFile(STAT_FILE);
  loadFile(STAT_FILE);

  //Запуск точки доступа с параметрами поумолчанию
  if ( !loadFile(CONFIG_FILE) ||  digitalRead(BUTTON_GPIO) == 0)  startAp("ESP", "11111111");
  //Запуск точки доступа
  else if (digitalRead(BUTTON_GPIO) == 1 && wifiAP_mode == 1)   startAp(p_ssidAP, p_passwordAP);
  //Запуск подключения клиента к точке доступа
  else if (digitalRead(BUTTON_GPIO) == 1 && wifiAP_mode == 0) {
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


  if (mqtt.connected()) {
    mqtt.loop();
    //Переподключение к MQTT серверу, если мы подключены к WIFI и связь с MQTT отсутствует, каждые 5 сек
  } else if (!mqtt.connected() && WiFi.status() == WL_CONNECTED && millis() - startMqttReconnectTime > TIME_ATTEMP_CON_MQTT) {
    reconnect();
    startMqttReconnectTime = millis();
  }


  //Обработка состояния сенсоров
  int prevSensorState = sensor1State;
  //sensor1State = digitalRead(SENSOR1_GPIO);
  sensor1State = Sensor1.read();
  if (prevSensorState != sensor1State)  dataUpdateBit = 1;
  prevSensorState = sensor2State;
  //sensor2State = digitalRead(SENSOR2_GPIO);
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
  if (digitalRead(RELAY_GPIO) == 0 && relayState == 0) {
    digitalWrite(RELAY_GPIO, 1);
    int deltaTimeRelayOn = millis() - startTimeRelayOn;
    timeRelayOn += deltaTimeRelayOn;
    if (deltaTimeRelayOn > mdTimeRelayOn)  mdTimeRelayOn = deltaTimeRelayOn;
    digitalWrite(LED_GREEN_GPIO, 0);
    //digitalWrite(LED_WIFI_GPIO, 1);
    dataUpdateBit = 1;
  } else if (digitalRead(RELAY_GPIO) == 1 && relayState == 1) {
    digitalWrite(RELAY_GPIO, 0);
    startTimeRelayOn = millis();
    digitalWrite(LED_GREEN_GPIO, 1);
    //digitalWrite(LED_WIFI_GPIO, 0);
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
      if (T_broadcastTXT > 100000)  checkPing();
    }
    dataUpdateBit = 0;
  }

  //подсчет времени с момента включения устройства
  if (millis() - startTimeESPOn > 5000) {
    timeESPOn += (millis() - startTimeESPOn);
    startTimeESPOn = millis();
  }

  //Сохранение stat данных в файл с периодичностью timeSaveStat
  if (millis() - startTimeSaveStat > timeSaveStat) {
    saveFile(STAT_FILE);
    startTimeSaveStat = millis();
  }

  //Контроль переполнения таймера millis
  //Устанавливаем бит контроля переполнения таймера millis, при достижении им 4.200.000.000 из 4.294.967.296
  if (overflowControl == 0 && millis() > 4200000000) {
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


