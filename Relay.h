/////////////////////////////////////////////////////////////////////////
///////////////////////// КЛАСС РЕЛЕ ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

//Состояние реле
#define ON 1
#define OFF 0

//Режимы работы реле
#define MODE_OFF 0    // - реле отключено
#define MODE_ON 1    // - реле включено
#define MODE_AUTO 2    // - работа с сенсорами(в автоматическом режиме)


class Relay {
  private:
    bool relay_state = OFF;
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
    int relay_number_on_total = 0;               //счетчик общего количества включений реле
    unsigned int relay_work_time_total = 0;      //счетчик полного времени работы реле, cек
    unsigned int relay_delay_off = 4000;         //задержка отключения для датчика движения, мсек

    Relay(int);
    void toggle();
    bool getState();
    void setMode(int);
    int getMode();
    void update();
    void atachPirSensor(int number, PirSensor *pirSensorPtr);
    void detachPirSensor(int number);
    bool readPirSensor(int number);
};


//* Relay Class Constructor
Relay::Relay(int pin) {
  relay_pin = pin;
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, relay_state);
}


//Включение реле
void Relay::on() {
  if (relay_state != ON) {
    relay_state = ON;
    digitalWrite(relay_pin, relay_state);
    ancillary_relay_work_time_total = millis();
    relay_number_on_total ++;
  }
}

//Отключение реле
void Relay::off() {
  if (relay_state != OFF) {
    relay_state = OFF;
    digitalWrite(relay_pin, relay_state);
    relay_work_time_total += (millis() - ancillary_relay_work_time_total) / 1000;
  }
}

//Инвертировать состояние реле
void Relay::toggle() {
  if (relay_state == ON)    off();
  else if (relay_state == OFF)    on();
}

//Получение состояния реле
bool Relay::getState() {
  return relay_state;
}

//Установить режим работы реле
void Relay::setMode(int mode) {
  relay_mode = mode;
}

//Получить режим работы реле
int Relay::getMode() {
  return relay_mode;
}


void Relay::update() {
  if (relay_mode == 0) {
    off();
  } else if (relay_mode == 1) {
    on();
  }
  else {
    bool result = false;
    if (pirSensor0Ptr != nullptr)  result = result | pirSensor0Ptr->read();
    if (pirSensor1Ptr != nullptr)  result = result | pirSensor1Ptr->read();

    if (result) on();
    else off();
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
    case 1:
      if (pirSensor1Ptr != nullptr)   return pirSensor1Ptr->read();
  }
}
