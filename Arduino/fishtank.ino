#define DEBUG

#ifdef DEBUG
#define logp(s) Serial.print(s)
#define logln(s) Serial.println(s)
#else
#define logp(s) 0
#define logln(s) 0
#endif


#include "Wire.h"
#include <Adafruit_NeoPixel.h>

const int DS3232_I2C_ADDRESS = 0x68;

const long SECSPERDAY = 60L * 60L * 24L;

class ClockStuff {
  public:
    uint32_t ms;
#ifdef DEBUG
    const uint32_t tickMs = 121L;
#else
    const uint32_t tickMs = 60L * 616L; // golden ratio
#endif

    long timeofdaySec; // 60 * 24

    inline byte decToBcd(byte val)
    {
      return ( (val / 10 * 16) + (val % 10) );
    }

    inline byte bcdToDec(byte val)
    {
      return ( (val / 16 * 10) + (val % 16) );
    }

    void setup() {
      Wire.begin();
      tick();
      ms = millis();
    }

    void loop() {
      if (millis() - ms > tickMs) {
        tick();
        ms = millis();
      }
    }

    void tick() {
      Wire.beginTransmission(DS3232_I2C_ADDRESS);
      Wire.write(0); // set DS3232 register pointer
      Wire.endTransmission();
      Wire.requestFrom(DS3232_I2C_ADDRESS, 3); // request 3 bytes of data from DS3232 starting from register 00h

      // A few of these need masks because certain bits are control bits
      byte second     = bcdToDec(Wire.read());
      byte minute     = bcdToDec(Wire.read());
      byte hour       = bcdToDec(Wire.read() & 0x3f);

      long prevTimeofdaySec = timeofdaySec;

      timeofdaySec = ((long)hour) * 60L * 60L + ((long)minute) * 60L + (long) second;

#ifdef DEBUG
      // compress a day into 30 seconds
      timeofdaySec = (timeofdaySec * 24L * 60L * 2L) % SECSPERDAY;
#endif

      if (timeofdaySec != prevTimeofdaySec) {
        newTime();
      }
    }

    virtual void newTime() = 0;
};

class MoonStuff {
  public:

    const byte pin;
    const Adafruit_NeoPixel pixels;
    long timeofdaySec = 0;
    float moonWidth;
    float r, g, b;
    long moonriseSec = 18L * 60L * 60L;
    long moonsetSec = 6L * 60L * 60L;
    long nightLenSec;

    MoonStuff(byte pin) : pin(pin), pixels(12, pin, NEO_GRB + NEO_KHZ800) {
      deriveNightlen();
    }

    void deriveNightlen() {
      nightLenSec = (moonsetSec - moonriseSec + SECSPERDAY * 2) % SECSPERDAY;
    }

    void setup() {
      setNumPixels(30 * 4); // I have 4m of neopixels
      setColor(0xE0, 0xE0, 0xFF);
      setWidth(9.25);
      setBrightness(8);
      pixels.begin();
      drawMoon();

      logp("moonriseSec  IS ");
      logln(moonriseSec);
      logp("moonsetSec IS ");
      logln(moonsetSec);
      logp("nightLenSec IS ");
      logln(nightLenSec);

    }

    void loop() {
    }

    void setNumPixels(int n) {
      if (n == pixels.numPixels()) return;
      if (n < 1) return;
      if (n > 1000) return;

      pixels.updateLength(n);
      drawMoon();
    }

    void setColor(byte _r, byte _g, byte _b) {
      r = _r;
      g = _g;
      b = _b;
      drawMoon();
    }

    void setWidth(float newWidth) {
      moonWidth = newWidth;
      drawMoon();
    }

    void setBrightness(byte brightness) {
      pixels.setBrightness(brightness);
      drawMoon();
    }

    void setTimeSec(long s) {
      logp(s);
      logp(' ');
      logp(s / 60 / 60);
      logp(':');
      logp((s / 60) % 60);
      logp(':');
      logp(s % 60);
      logln();
      if (s != timeofdaySec) {
        timeofdaySec = s;
        drawMoon();
      }
    }

    void drawMoon() {
      pixels.clear();

      long tt = timeofdaySec;
      if (tt < moonriseSec) tt += SECSPERDAY;
      tt -= moonriseSec;
      if (tt <= nightLenSec) {
        float moonCenter = (float) tt / nightLenSec * pixels.numPixels();

        logp("moon at ");
        logp(moonCenter);

        for (int i = moonCenter - moonWidth ; i <= moonCenter + moonWidth ; i ++ ) {
          if (i < 0 || i >= pixels.numPixels()) continue;

          float pp = (i - moonCenter) / (moonWidth / 2);
          if (pp <= -1 || pp >= 1) continue;

          // scale by the width of the lunar disk, and don't take square root
          // so as to apply a gamma of .5
          float brite = 1 - pp * pp;

          pixels.setPixelColor(i, pixels.Color(r * brite, g * brite, b * brite));
        }
      }
      else {
        logp("moon below horizon");
      }

      logln(" SHOW");
      while(!pixels.canShow()) delay(1);
      pixels.show();
    }

};


class MoonClock : public ClockStuff {
  public:
    MoonStuff &moonStuff;

    MoonClock(MoonStuff &moonStuff) : moonStuff(moonStuff) {}

    void newTime() {
      moonStuff.setTimeSec(timeofdaySec);
    }
};

// PINOUT

MoonStuff moonStuff(6);
MoonClock moonClock(moonStuff);


void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#endif

  moonStuff.setup();
  moonClock.setup();

}

void loop() {
  moonStuff.loop();
  moonClock.loop();
}
