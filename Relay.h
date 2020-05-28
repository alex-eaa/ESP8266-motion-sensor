/////////////////////////////////////////////////////////////////////////
///////////////////////// КЛАСС РЕЛЕ ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

//Состояние реле
#define RELAY_ON  0
#define RELAY_OFF 1

//Режимы работы реле
#define MODE_OFF  0     // - реле отключено
#define MODE_ON   1     // - реле включено
#define MODE_AUTO 2     // - работа с сенсорами(в автоматическом режиме)

#define SERIALYZE_FOR_FILE 0
#define SERIALYZE_ALL   1


class Relay {
  private:
    unsigned int ancillaryTotalTimeOn;   //вспом. переменная для счетчик полного времени работы реле
    unsigned int ancillaryDelayOff;        //вспом. переменная для задержка отключения для датчика движения
    PirSensor *pirSensor0Ptr = nullptr;
    PirSensor *pirSensor1Ptr = nullptr;

  private:
    void on();
    void off();

  public:
    int pin;
    int relayMode = MODE_OFF;
    bool relayState = RELAY_OFF;
    int sumSwitchingOn = 0;               //суммарное количество включений реле
    unsigned int totalTimeOn = 0;         //общее время работы реле, мсек
    unsigned int maxContinuousOn = 0;     //максимальная продолжительность непрерывной работы реле, мсек
    unsigned int delayOff = 4000;         //задержка отключения для датчика движения, мсек
    bool dataUpdateBit = 0;               //флаг обновления данных, устанавливается когда состояние реле изменилось и нужно отправить их клиенту

    Relay(int setPin, int setMode);
    void setMode(int setMode);
    void atachPirSensor(int number, PirSensor *pirSensorPtr);
    void detachPirSensor(int number);
    bool readPirSensor(int number);
    void update();
    void serialize(DynamicJsonDocument *doc, int serializeDataType);
    void resetStat();
};


//* Relay Class Constructor
Relay::Relay(int setPin, int setMode) {
  pin = setPin;
  relayMode = setMode;
  pinMode(pin, OUTPUT);
  digitalWrite(pin, relayState);
}


//Включение реле
void Relay::on() {
  if (relayState != RELAY_ON) {
    digitalWrite(pin, relayState);
    relayState = RELAY_ON;
    ancillaryTotalTimeOn = millis();
    sumSwitchingOn ++;
    dataUpdateBit = 1;
  }
}


//Отключение реле
void Relay::off() {
  if (relayState != RELAY_OFF) {
    digitalWrite(pin, relayState);
    relayState = RELAY_OFF;
    totalTimeOn += (millis() - ancillaryTotalTimeOn);
    if (maxContinuousOn < (millis() - ancillaryTotalTimeOn)) {
      maxContinuousOn = (millis() - ancillaryTotalTimeOn);
    }
    dataUpdateBit = 1;
  }
}


void Relay::update() {
  if (pirSensor0Ptr != nullptr && pirSensor0Ptr->dataUpdateBit==1) {
    dataUpdateBit = 1;
    pirSensor0Ptr->dataUpdateBit = 0;
  }
  if (pirSensor1Ptr != nullptr && pirSensor1Ptr->dataUpdateBit==1) {
    dataUpdateBit = 1;
    pirSensor1Ptr->dataUpdateBit = 0;
  }

  if (relayMode == MODE_OFF) {
    off();
  } else if (relayMode == MODE_ON) {
    on();
  }
  else if (relayMode == MODE_AUTO) {
    bool result = false;
    if (pirSensor0Ptr != nullptr)  result = result | pirSensor0Ptr->read();
    if (pirSensor1Ptr != nullptr)  result = result | pirSensor1Ptr->read();

    if (result == true) {
      on();
      ancillaryDelayOff = millis();
    }
    else if (millis() - ancillaryDelayOff > delayOff) {
      off();
    }
  }
}


void Relay::atachPirSensor(int number, PirSensor *pirSensorPtr) {
  switch (number) {
    case 0:
      pirSensor0Ptr = pirSensorPtr;
      break;
    case 1:
      pirSensor1Ptr = pirSensorPtr;
      break;
  }
  dataUpdateBit = 1;
}


void Relay::detachPirSensor(int number) {
  switch (number) {
    case 0:
      pirSensor0Ptr = nullptr;
      break;
    case 1:
      pirSensor1Ptr = nullptr;
      break;
  }
  dataUpdateBit = 1;
}


bool Relay::readPirSensor(int number) {
  switch (number) {
    case 0:
      if (pirSensor0Ptr != nullptr)   return pirSensor0Ptr->read();
      else return 0;
    case 1:
      if (pirSensor1Ptr != nullptr)   return pirSensor1Ptr->read();
      else return 0;
  }
}


void Relay::serialize(DynamicJsonDocument *doc, int serializeDataType) {
  JsonObject obj = doc->createNestedObject("relay");

  obj["relayMode"] = relayMode;
  obj["delayOff"] = delayOff;
  if (pirSensor0Ptr != nullptr)   obj["sensor0Use"] = true;
  else obj["sensor0Use"] = false;
  if (pirSensor1Ptr != nullptr)   obj["sensor1Use"] = true;
  else obj["sensor1Use"] = false;
  obj["sumSwitchingOn"] = sumSwitchingOn;
  obj["totalTimeOn"] = totalTimeOn;

  if (serializeDataType == SERIALYZE_ALL) {
    obj["relayState"] = relayState;
    obj["maxContinuousOn"] = maxContinuousOn;
  }
}


void Relay::resetStat() {
  sumSwitchingOn = 0;
  totalTimeOn = 0;
  dataUpdateBit = 1;
}

