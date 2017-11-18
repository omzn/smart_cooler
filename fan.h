#ifndef FAN_H
#define FAN_H

#include "Arduino.h"

#define MAX_PWM_VALUE 1023

class fanCooler {
  public:
    fanCooler(uint8_t pin);

    void value(int v);
    int value();
    int enable();
    int enableAutoFan();
    int disableAutoFan();
    void control(float t);
    void on();
    void off();
    uint8_t status();
    float highLimit();
    float lowLimit();
    void highLimit(float v);
    void lowLimit(float v);
    void setLimits(float high, float low);
  protected:
    int _pin;
    int _power;
    int _use_autofan;
    float _high_limit;
    float _low_limit;
};

#endif
