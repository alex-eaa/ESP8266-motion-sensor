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
bool dataUpdateBit = 0;

int relayMode = 2;        //режим работы, 0-откл, 1-вкл, 2-авто
bool relayState = 0;      //состояние реле 0-off, 1-on
bool sensor1State = 0;    //состояние сенсора №1: 0-off, 1-on
bool sensor2State = 0;    //состояние сенсора №2: 0-off, 1-on
bool sensor1Use = 1;      //использование сенсора №1. 0-off, 1-on
bool sensor2Use = 0;      //использование сенсора №2. 0-off, 1-on
unsigned int delayOff = 20000;     //Задержка отключения, мс
unsigned int startDelayOff = 0;    //вспом. для delayOff

bool preRelayState;
bool overfloControl = 0;     //Бит включающий контроль переполнения millis()

//Statistic variables
unsigned int numbOn = 0;                //количество включений
double timeRelayOn = 0;                 //время в состоянии реле ВКЛ, мс
unsigned int startTimeRelayOn = 0;      //вспом. для timeRelayOn
unsigned int mdTimeRelayOn = 0;         //максимальный промежуток времени включеного реле, мс
double timeESPOn = 0;                   //время с момента включения устройства, мс
int startTimeESPOn = 0;                 //вспом. для timeESPOn
unsigned int timeSaveStat = 86400000;   //периодичность сохранения статистики, мс
unsigned int startTimeSaveStat = 0;     //вспом. для timeSaveStat

WebSocketsServer webSocket(81);
ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println("\n");
  pinMode(LED_WIFI_GPIO, OUTPUT);
  pinMode(LED_GREEN_GPIO, OUTPUT);
  pinMode(BUTTON_GPIO, INPUT_PULLUP);
  pinMode(SENSOR1_GPIO, INPUT);
  pinMode(SENSOR2_GPIO, INPUT);
  pinMode(RELAY_GPIO, OUTPUT);
  digitalWrite(LED_WIFI_GPIO, HIGH);
  digitalWrite(LED_GREEN_GPIO, LOW);
  digitalWrite(RELAY_GPIO, 1);
  //printChipInfo();

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
      set_staticIP();
      WiFi.begin(p_ssid, p_password);
    } else {
      set_staticIP();
    }
  }

  if (!MDNS.begin("esp8266")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }

  webServer_init();      //инициализация HTTP интерфейса
  webSocket_init();      //инициализация webSocket интерфейса

/*
  if (!MDNS.begin("esp8266")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(100);
    }
  }else{
      MDNS.addService("http", "tcp", 80);
  }
*/
  startTimeSaveStat = millis();
  startTimeESPOn = millis();
}


void loop() {
  wifi_init();
  webSocket.loop();
  server.handleClient();

  //Обработка состояния сенсоров
  int prevSensorState = sensor1State;
  sensor1State = digitalRead(SENSOR1_GPIO);
  if (prevSensorState != sensor1State)  dataUpdateBit = 1;
  prevSensorState = sensor2State;
  sensor2State = digitalRead(SENSOR2_GPIO);
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
    if (false) {
      Serial.print("relayState = ");
      Serial.println(relayState);
      Serial.print("sensor1State = ");
      Serial.println(sensor1State);
      Serial.print("sensor2State = ");
      Serial.println(sensor2State);
      Serial.print("numbOn = ");
      Serial.println(numbOn);
      Serial.print("timeRelayOn = ");
      Serial.println(timeRelayOn);
      Serial.print("timeESPOn = ");
      Serial.println(timeESPOn);
      Serial.print("mdTimeRelayOn = ");
      Serial.println(millis() - mdTimeRelayOn);
      Serial.println();
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
  if (overfloControl == 0 && millis() > 4200000000) {
    overfloControl = 1;
  }
  //Если произошло переполнение millis
  if (overfloControl == 1 && millis() > 0 && millis() < 10000) {
    overfloControl = 0;
    startTimeSaveStat = millis();
    startTimeESPOn = millis();
    startTimeRelayOn = millis();
  }

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
