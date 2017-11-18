#include "SF_s7s_hw.h"

// Send the clear display command (0x76)
//  This will clear the display and reset the cursor
void S7S::clearDisplay()
{
  Serial.write(0x76);  // Clear display command
}

// Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void S7S::setBrightness(byte value)
{
  Serial.write(0x7A);  // Set brightness command byte
  Serial.write(value);  // brightness data byte
}

// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal 
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void S7S::setDecimals(byte decimals)
{
  Serial.write(0x77);
  Serial.write(decimals);
}

void S7S::printTime(String ts) {
//  static uint8_t colon = 0;
  Serial.print(ts.c_str());
//  if (colon) {
    setDecimals(BIT_COLON);
//  } else {
//    setDecimals(0);
//  }
//  colon = 1 - colon;
}

void S7S::print04d(const int value) {
  static char s[20];
  setDecimals(0);
  sprintf(s,"%04d",value);    
  Serial.print(s);
}

void S7S::print4d(const int value) {
  static char s[20];
  setDecimals(0);
  sprintf(s,"%4d",value);    
  Serial.print(s);
}

void S7S::print41f(const float value) {
  static char s[20];
  setDecimals(BIT_DIGIT3);
  sprintf(s,"%4d",(int)(value * 10));    
  Serial.print(s);
}

void S7S::print(const char *str) {
  Serial.print(str);
}

