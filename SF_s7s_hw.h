#ifndef SF_S7S_HW_H
#define SF_S7S_HW_H

#include "Arduino.h"

#define BIT_APOS   0b100000
#define BIT_COLON  0b010000
#define BIT_DIGIT4 0b001000
#define BIT_DIGIT3 0b000100
#define BIT_DIGIT2 0b000010
#define BIT_DIGIT1 0b000001

class S7S {
  public:
    void clearDisplay();
    void setDecimals(byte b);
    void setBrightness(byte b);
    void print(const char *str);
    void printTime(String ts);
    void print4d(const int value);
    void print04d(const int value);
    void print41f(const float value);
};


#endif
