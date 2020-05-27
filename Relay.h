/////////////////////////////////////////////////////////////////////////
///////////////////////// КЛАСС РЕЛЕ ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

//Состояние реле
#define RELAY_ON  1
#define RELAY_OFF 0

//Режимы работы реле
#define MODE_OFF  0     // - реле отключено
#define MODE_ON   1     // - реле включено
#define MODE_AUTO 2     // - работа с сенсорами(в автоматическом режиме)

#define SERIALYZE_TYPE_CONFIG 0
#define SERIALYZE_TYPE_STAT   1
#define SERIALYZE_TYPE_FOR_INDEX  2


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
    int mode = MODE_OFF;
    bool state = RELAY_OFF;
    int sumSwitchingOn = 0;                      //суммарное количество включений реле
    unsigned int totalTimeOn = 0;                //общее время работы реле, мсек
    unsigned int maxContinuousOn = 0;            //максимальная продолжительность непрерывной работы реле, мсек
    unsigned int delayOff = 4000;         //задержка отключения для датчика движения, мсек
    bool dataUpdateBit = 0;

    Relay(int setPin, int setMode);
    void setMode(int setMode);
    void atachPirSensor(int number, PirSensor *pirSensorPtr);
    void detachPirSensor(int number);
    bool readPirSensor(int number);
    void update();
    void serialize(DynamicJsonDocument *doc, int serializeDataType);
};


//* Relay Class Constructor
Relay::Relay(int setPin, int setMode) {
  pin = setPin;
  mode = setMode;
  pinMode(pin, OUTPUT);
  digitalWrite(pin, state);
}


//Включение реле
void Relay::on() {
  if (state != RELAY_ON) {
    digitalWrite(pin, state);
    state = RELAY_ON;
    ancillaryTotalTimeOn = millis();
    sumSwitchingOn ++;
  }
}


//Отключение реле
void Relay::off() {
  if (state != RELAY_OFF) {
    digitalWrite(pin, state);
    state = RELAY_OFF;
    totalTimeOn += (millis() - ancillaryTotalTimeOn);
    if (maxContinuousOn < (millis() - ancillaryTotalTimeOn)) {
      maxContinuousOn = (millis() - ancillaryTotalTimeOn);
    }
  }
}


void Relay::update() {
  if (mode == MODE_OFF) {
    off();
  } else if (mode == MODE_ON) {
    on();
  }
  else if (mode == MODE_AUTO) {
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
  switch (serializeDataType) {
    case SERIALYZE_TYPE_CONFIG:
      obj["mode"] = mode;
      obj["delayOff"] = delayOff;
      if (pirSensor0Ptr != nullptr)   obj["pirSensor0Used"] = true;
      else obj["pirSensor0Used"] = false;
      if (pirSensor1Ptr != nullptr)   obj["pirSensor1Used"] = true;
      else obj["pirSensor1Used"] = false;
      break;
    case SERIALYZE_TYPE_STAT:
      obj["sumSwitchingOn"] = sumSwitchingOn;
      obj["totalTimeOn"] = totalTimeOn;
      break;
    case SERIALYZE_TYPE_FOR_INDEX:
      obj["mode"] = mode;
      obj["delayOff"] = delayOff;
      if (pirSensor0Ptr != nullptr)   obj["pirSensor0Used"] = true;
      else obj["pirSensor0Used"] = false;
      if (pirSensor1Ptr != nullptr)   obj["pirSensor1Used"] = true;
      else obj["pirSensor1Used"] = false;
      obj["state"] = state;
      obj["sumSwitchingOn"] = sumSwitchingOn;
      obj["totalTimeOn"] = totalTimeOn;
      obj["maxContinuousOn"] = maxContinuousOn;
      break;
  }
}
