/////////////////////////////////////////////////////////////////////////
///////////////////////КЛАСС ДАТЧИКА ДВИЖЕНИЯ////////////////////////////
/////////////////////////////////////////////////////////////////////////

//Состояние реле
#define SENSOR_ON  1
#define SENSOR_OFF 0

class PirSensor {
  private:
    bool state = false;
    unsigned int timeBounce = 50;   //время подавления дребезга, мс
    unsigned int prevTimeBounce;          //вспом. для timeBounce

  public:
    int pin;
    bool dataUpdateBit = 0;    //флаг обновления данных, устанавливается когда состояние датчика изменилось и нужно отправить их клиенту

  public:
    PirSensor(int setPin);
    void update();
    bool read();
};


//* PirSensor Class Constructor
PirSensor::PirSensor(int setPin) {
  pin = setPin;
  pinMode(pin, INPUT_PULLUP);
  prevTimeBounce = millis();
  state = digitalRead(pin);
}


//Обновление состояния датчика pirs с подавлением дребезга контактов
void PirSensor::update() {
  bool newstate = digitalRead(pin);
  if (newstate != state) {
    if (millis() - prevTimeBounce > timeBounce) {
      state = newstate;
      dataUpdateBit = 1;
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
