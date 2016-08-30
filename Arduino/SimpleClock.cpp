#include <Arduino.h>
#include <Wire.h>

#include "SimpleClock.h"

void SimpleClock::setup() {
  Wire.begin();
  tick();
}

void SimpleClock::loop() {
  static uint32_t ms;

  if ((millis()) - ms > (fast ? fastTickMs : slowTickMs)) {
    tick();
    ms = millis();
  }
}

void SimpleClock::setTime(long targetTime) {
  timeofdayOffsetSec = (SECSPERDAY * 2 + targetTime - timeofdaySec) % SECSPERDAY;
  newTime(getTime());
}

uint32_t SimpleClock::getTime() {
  return (timeofdaySec + timeofdayOffsetSec) % SECSPERDAY;
}

void SimpleClock::setFast(boolean f) {
  fast = f;
  tick();
}


void SimpleClock::tick() {
  Wire.beginTransmission(DS3232_I2C_ADDRESS);
  Wire.write(0); // set DS3232 register pointer
  Wire.endTransmission();
  Wire.requestFrom(DS3232_I2C_ADDRESS, 3); // request 3 bytes of data from DS3232 starting from register 00h

  // A few of these need masks because certain bits are control bits
  byte second     = bcdToDec(Wire.read());
  byte minute     = bcdToDec(Wire.read());
  byte hour       = bcdToDec(Wire.read() & 0x3f);

  timeofdaySec = ((long)hour) * 60L * 60L + ((long)minute) * 60L + (long) second;

  if (fast) {
    timeofdaySec = (timeofdaySec * 24 * 60 / 5) % SECSPERDAY; // compress one day into five minutes
  }

  if (timeofdaySec != prevTimeofdaySec) {
    newTime(getTime());
    prevTimeofdaySec = timeofdaySec;
  }
}
