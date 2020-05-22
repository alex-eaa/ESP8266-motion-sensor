/////////////////////////////////////////////////////////////////////////
///////////////////////// КЛАСС РЕЛЕ ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

//Состояние реле
#define ON 1
#define OFF 0

//Режимы работы реле
#define mode_0 0    // - работа без сенсоров (в ручном режиме)
#define mode_1 1    // - от pirSensor


class Relay {
  private:
    bool relay_state = OFF;
    unsigned int ancillary_relay_work_time_total;   //вспом. переменная для счетчик полного времени работы реле
    unsigned int ancillary_realay_delay_off;        //вспом. переменная для задержка отключения для датчика движения

  public:
    int relay_pin;
    int relay_mode = 0;
    int relay_number_on_total = 0;               //счетчик общего количества включений реле
    unsigned int relay_work_time_total = 0;      //счетчик полного времени работы реле, cек
    unsigned int relay_delay_off = 4000;         //задержка отключения для датчика движения, мсек
    int ptrArrayPirSensors_size = 0;             //размер массива указателей на объекты PirSensor
    PirSensor **ptrArrayPirSensors;              //Динамический массив указателей на объекты PirSensor в этом объекте реле

    Relay(int);
    void setPin(int);
    void on();
    void off();
    void toggle();
    bool getState();
    void setMode(int);
    int getMode();
    void addPirSensor(int);
    void delPirSensor(int);
};


//* Relay Class Constructor
Relay::Relay(int pin) {
  setPin(pin);
}

//Установка номера GPIO и инициализация Relay
void Relay::setPin(int pin) {
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

void Relay::setMode(int mode) {
  relay_mode = mode;
}

int Relay::getMode() {
  return relay_mode;
}

void Relay::addPirSensor(int gpio) {
  if (ptrArrayPirSensors_size == 0) {
    ptrArrayPirSensors = new PirSensor* [1];
    ptrArrayPirSensors[0] = new PirSensor(gpio);
    Serial.print("Add new PirSensor, gpio = ");   Serial.println(ptrArrayPirSensors[0]->sensor_pin);
    ptrArrayPirSensors_size ++;
  }
}

