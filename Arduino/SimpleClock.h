/**
 * SimpleClock.h - talk to a Fretronics DS3232 module over I2C. 
 * 
 * The purpose of this class is to keep track of the time mod seconds per day with
 * a settable offset, notifying a subclass at intervals.
 * 
 * In fast mode, the subclass callback is notified about once per second. In slow mode,
 * about once per 37 seconds (1 minute / golden ratio).
 * 
 * This class does not write the time to the DS3232 - it simply maintains an offset.
 * The reason for this is that my android app permits the time to be set with a slider,
 * and sending hundreds of EEPROM writes to the DS3232 seems a little rude. All I really 
 * care about is that my target app (a neopixel strip with a mocing "moon") should cycle
 * once per day.
 */

#ifndef pmurray_at_bigpond_dot_com_SimpleClock
#define pmurray_at_bigpond_dot_com_SimpleClock 1

#include <Arduino.h>

#define SECSPERDAY (60L * 60L * 24L)

class SimpleClock {
  private:
    const int DS3232_I2C_ADDRESS = 0x68;
    long timeofdaySec;
    long prevTimeofdaySec;
    long timeofdayOffsetSec = 0;

    const uint32_t fastTickMs = 121L;
    const uint32_t slowTickMs = 60L * 616L; // one minute golden ratio

    inline byte decToBcd(byte val) {
      return ( (val / 10 * 16) + (val % 10) );
    }

    inline byte bcdToDec(byte val) {
      return ( (val / 16 * 10) + (val % 16) );
    }

    boolean fast = false;

  public:
    void setup();

    void loop();

    void setTime(long targetTime);

    uint32_t getTime();

    void setFast(boolean f);

    inline boolean isFast() {
      return fast;
    }

  protected:
    virtual void newTime(long calculatedTimeSec) = 0;
    
  private:
    void tick();

};

#endif
