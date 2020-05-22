/////////////////////////////////////////////////////////////////////////
///////////////////////КЛАСС ДАТЧИКА ДВИЖЕНИЯ////////////////////////////
/////////////////////////////////////////////////////////////////////////

class PirSensor {
  private:
    bool sensor_state;
    unsigned int timeBounce = 50;   //время подавления дребезга, мс
    unsigned int prevTimeBounce;

  public:
    int sensor_pin;

    PirSensor(int pin);
    bool read();
    void setPin(int pin);
};


//* PirSensor Class Constructor
PirSensor::PirSensor(int pin) {
  setPin(pin);
}


//Установка номера GPIO и инициализация PirSensor 
void PirSensor::setPin(int pin) {
  sensor_pin = pin;
  pinMode(sensor_pin, INPUT_PULLUP);
  prevTimeBounce = millis();
}


//Считывание состояния датчика pirs с подавлением дребезга контактов
bool PirSensor::read() {
  bool newsensor_state = digitalRead(sensor_pin);
  if (newsensor_state != sensor_state) {
    if (millis() - prevTimeBounce > timeBounce) {
      sensor_state = newsensor_state;
      prevTimeBounce = millis();
    }
  } else {
    prevTimeBounce = millis();
  }
  return sensor_state;
}

