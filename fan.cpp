#include "fan.h"

fanCooler::fanCooler(uint8_t pin){
  pinMode(pin,OUTPUT);
  _pin = pin;
}

int fanCooler::value() {
  return _power;
}

void fanCooler::value(int v) {
  if (v > MAX_PWM_VALUE) {
    v = MAX_PWM_VALUE;
  } else if (v < 0) {
    v = 0;
  }
  analogWrite(_pin,v);
  _power = v;
}

int fanCooler::enable() {
  return _use_autofan;
}

int fanCooler::enableAutoFan() {
  _use_autofan = 1;
}

int fanCooler::disableAutoFan() {
  _use_autofan = 0;
}

void fanCooler::setLimits(float high, float low) {
  _high_limit = high;
  _low_limit = low;
}

void fanCooler::highLimit(float v) {
  _high_limit = v;
}

void fanCooler::lowLimit(float v) {
  _low_limit = v;
}

float fanCooler::highLimit() {
  return _high_limit;
}

float fanCooler::lowLimit() {
  return _low_limit;
}

void fanCooler::control(float t) {
  if (_use_autofan) {
#ifdef DEBUG
    Serial.print("T=");
    Serial.print(t,1);
    Serial.print(" V=");
    Serial.println(_value);
    Serial.print("H=");
    Serial.print(_high_limit,1);
    Serial.print(" L=");
    Serial.println(_low_limit,1);
#endif
    uint8_t v = value();
    if (t >= _high_limit && v == 0) {
      on();
    } else if (t <= _low_limit && v != 0) {
      off();
    }
  }
}

void fanCooler::on() {
  value(MAX_PWM_VALUE);
}

void fanCooler::off() {
  value(0);
}

uint8_t fanCooler::status() {
  return (_power > 0 ? 1 : 0);
}


