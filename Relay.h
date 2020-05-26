/////////////////////////////////////////////////////////////////////////
///////////////////////// КЛАСС РЕЛЕ ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

//Состояние реле
#define RELAY_ON  true
#define RELAY_OFF false

//Режимы работы реле
#define MODE_OFF  0     // - реле отключено
#define MODE_ON   1     // - реле включено
#define MODE_AUTO 2     // - работа с сенсорами(в автоматическом режиме)

#define SERIALYZE_TYPE_CONFIG 0
#define SERIALYZE_TYPE_STAT   1


class Relay {
  private:
    unsigned int ancillary_relay_work_time_total;   //вспом. переменная для счетчик полного времени работы реле
    unsigned int ancillary_realay_delay_off;        //вспом. переменная для задержка отключения для датчика движения
    PirSensor *pirSensor0Ptr = nullptr;
    PirSensor *pirSensor1Ptr = nullptr;

  private:
    void on();
    void off();

  public:
    int relay_pin;
    int relay_mode = MODE_OFF;
    bool relay_state = RELAY_OFF;
    int relay_number_on_total = 0;               //счетчик общего количества включений реле
    unsigned int relay_work_time_total = 0;      //счетчик полного времени работы реле, мсек
    unsigned int max_delay_between_on_off = 0;   //максимальное время между включение и отключением реле, мсек
    unsigned int relay_delay_off = 4000;         //задержка отключения для датчика движения, мсек

    Relay(int setPin, int setMode);
    void setMode(int setMode);
    int getMode();
    void update();
    void atachPirSensor(int number, PirSensor *pirSensorPtr);
    void detachPirSensor(int number);
    bool readPirSensor(int number);
    void serialize(DynamicJsonDocument *doc, int serializeDataType);
};


//* Relay Class Constructor
Relay::Relay(int setPin, int setMode) {
  relay_pin = setPin;
  relay_mode = setMode;
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, relay_state);
}


//Включение реле
void Relay::on() {
  if (relay_state != RELAY_ON) {
    digitalWrite(relay_pin, relay_state);
    relay_state = RELAY_ON;
    ancillary_relay_work_time_total = millis();
    relay_number_on_total ++;
  }
}


//Отключение реле
void Relay::off() {
  if (relay_state != RELAY_OFF) {
    digitalWrite(relay_pin, relay_state);
    relay_state = RELAY_OFF;
    relay_work_time_total += (millis() - ancillary_relay_work_time_total);
    if (max_delay_between_on_off < (millis() - ancillary_relay_work_time_total)) {
      max_delay_between_on_off = (millis() - ancillary_relay_work_time_total);
    }
  }
}


//Установить режим работы реле
void Relay::setMode(int setMode) {
  relay_mode = setMode;
}


//Получить режим работы реле
int Relay::getMode() {
  return relay_mode;
}


void Relay::update() {
  if (relay_mode == MODE_OFF) {
    off();
  } else if (relay_mode == MODE_ON) {
    on();
  }
  else if (relay_mode == MODE_AUTO) {
    bool result = false;
    if (pirSensor0Ptr != nullptr)  result = result | pirSensor0Ptr->read();
    if (pirSensor1Ptr != nullptr)  result = result | pirSensor1Ptr->read();

    if (result==true) {
      on();
      ancillary_realay_delay_off = millis();
    }
    else if (millis() - ancillary_realay_delay_off > relay_delay_off) {
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


void  Relay::serialize(DynamicJsonDocument *doc, int serializeDataType) {
  JsonObject obj = doc->createNestedObject("relay");
  switch (serializeDataType) {
    case SERIALYZE_TYPE_CONFIG:
      obj["relay_mode"] = relay_mode;
      if (pirSensor0Ptr != nullptr)   obj["pirSensor0Used"] = true;
      else obj["pirSensor0Used"] = false;
      if (pirSensor1Ptr != nullptr)   obj["pirSensor1Used"] = true;
      else obj["pirSensor1Used"] = false;
      break;
    case SERIALYZE_TYPE_STAT:
      obj["relay_number_on_total"] = relay_number_on_total;
      obj["relay_work_time_total"] = relay_work_time_total;
      break;
  }
}
