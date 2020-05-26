/////////////////////////////////////////////////////////////////////////
///////////////////////КЛАСС ДАТЧИКА ДВИЖЕНИЯ////////////////////////////
/////////////////////////////////////////////////////////////////////////

//Состояние реле
#define SENSOR_ON  0
#define SENSOR_OFF 1

class PirSensor {
  private:
    bool state;
    unsigned int timeBounce = 50;   //время подавления дребезга, мс
    unsigned int prevTimeBounce;          //вспом. для timeBounce

  public:
    int pin;

  public:
    PirSensor(int pin);
    void update();
    bool read();
};


//* PirSensor Class Constructor
PirSensor::PirSensor(int pin) {
  pin = pin;
  pinMode(pin, INPUT_PULLUP);
  prevTimeBounce = millis();
}

/*
  //Установка номера GPIO и инициализация PirSensor
  void PirSensor::setPin(int pin) {
  pin = pin;
  pinMode(pin, INPUT_PULLUP);
  prevTimeBounce = millis();
  }
*/

//Обновление состояния датчика pirs с подавлением дребезга контактов
void PirSensor::update() {
  bool newstate = digitalRead(pin);
  if (newstate != state) {
    if (millis() - prevTimeBounce > timeBounce) {
      state = newstate;
      prevTimeBounce = millis();
    }
  } else {
    prevTimeBounce = millis();
  }
}

//Считывание состояния датчика pirs
bool PirSensor::read() {
  update();
  if (state)   return SENSOR_ON;
  else   return SENSOR_OFF;
}
