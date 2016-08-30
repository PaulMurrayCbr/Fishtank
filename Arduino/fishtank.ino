//#define DEBUG

#ifdef DEBUG
#define logp(s) Serial.print(s)
#define logln(s) Serial.println(s)
#else
#define logp(s) 0
#define logln(s) 0
#endif


#include <Wire.h>
#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include "BTComms.h"
#include "SimpleClock.h"
#include "Bigend.h"

class MoonDrawer {
  public:

    const byte pin;
    const Adafruit_NeoPixel pixels;
    int moonWidth;
    float r, g, b;
    long moonriseSec = 16L * 60L * 60L;
    long moonsetSec = 4L * 60L * 60L;
    long nightLenSec;
    uint32_t timeOfDaySec;

    MoonDrawer(byte pin) : pin(pin), pixels(12, pin, NEO_GRB + NEO_KHZ800) {
      deriveNightlen();
    }

    void deriveNightlen() {
      nightLenSec = (moonsetSec - moonriseSec + SECSPERDAY * 2) % SECSPERDAY;
    }

    void setup() {
      setNumPixels(30 * 2); // 2m fishtank
      setColor(0xE0, 0xE0, 0xFF);
      setWidth(4.25);
      setBrightness(4);
      pixels.begin();
      drawMoon();
    }

    void loop() {
    }

    void setNumPixels(int n) {
      if (n == pixels.numPixels()) return;
      if (n < 1) return;
      if (n > 1000) return;

      pixels.clear();
      pixels.show();
      pixels.updateLength(n);
      drawMoon();
    }

    void setColor(byte _r, byte _g, byte _b) {
      r = _r;
      g = _g;
      b = _b;
      drawMoon();
    }

    void setWidth(int newWidth) {
      moonWidth = newWidth;
      drawMoon();
    }

    void setBrightness(byte brightness) {
      pixels.setBrightness(brightness);
      drawMoon();
    }

    void setMoonriseSec(long _moonriseSec) {
      moonriseSec = _moonriseSec;
      deriveNightlen();
      drawMoon();
    }

    void setMoonsetSec(long _moonsetSec) {
      moonsetSec = _moonsetSec;
      deriveNightlen();
      drawMoon();
    }

    void tick(uint32_t _timeOfDaySec) {
      timeOfDaySec = _timeOfDaySec;
      drawMoon();
    }

    void drawMoon() {
      pixels.clear();

      long tt = (timeOfDaySec - moonriseSec + SECSPERDAY * 2) % SECSPERDAY;

      if (tt <= nightLenSec) {
        float moonCenter = (float) tt / nightLenSec * pixels.numPixels();

        for (int i = moonCenter - moonWidth - 2; i <= moonCenter + moonWidth + 2; i ++ ) {
          if (i < 0 || i >= pixels.numPixels()) continue;

          float pp = (i - moonCenter) / ((float)moonWidth / 2);
          if (pp <= -1 || pp >= 1) continue;

          // scale by the width of the lunar disk, and don't take square root
          // so as to apply a gamma of .5
          float brite = 1 - pp * pp;

          pixels.setPixelColor(i, pixels.Color(r * brite, g * brite, b * brite));
        }
      }

      while (!pixels.canShow()) delay(1);
      pixels.show();
    }

};


class MoonClock : public SimpleClock {
  public:
    MoonDrawer &moonDrawer;

    MoonClock(MoonDrawer &moonDrawer) : moonDrawer(moonDrawer) {}

    void newTime(long calculatedTimeSec) {
      moonDrawer.tick(calculatedTimeSec);
    }
};

struct StatusBuffer {
  byte messageMark[1];
  byte rgb[3];
  byte numpixels[2];
  byte moonWidth[2];
  byte moonBright[1];
  byte moonrise[4];
  byte moonset[4];
  byte time[4];
  byte flags[1];
};

class MoonController : public BtReader::Callback {
    struct StatusBuffer buf;

  public:
    SimpleClock &clock;
    MoonDrawer &moon;
    BtWriter &writer;

    MoonController(SimpleClock &clock,  MoonDrawer &moon, BtWriter &writer) : clock(clock), moon(moon), writer(writer) {}

    void setup() {
    }

    void loop() {
    }
/*
    uint8_t asByte(byte *buf) {
      return be2byte(buf+1);
    }

    uint16_t asInt(byte *buf) {
      return be2int(buf+1);
    }

    uint32_t asLong(byte *buf) {
      return be2long(buf+1);
    }
*/
    void gotBytes(byte *buf, int ct) {
      buf[ct] = 0;
#ifdef DEBUG
      Serial.println();
      Serial.print("GOT STRING ");
      Serial.print('"');

      for (int i = 0; i < ct; i++) {
        if (buf[i] >= ' ' && buf[i] < 127) {
          Serial.print((char)buf[i]);
        }
        else {
          Serial.print('?');
        }
      }

      Serial.print('"');
      Serial.print(' ');
      Serial.print('<');

      for (int i = 0; i < ct; i++) {
        byte hex;
        hex = (buf[i] >> 4) & 0xF;
        Serial.print((char) ((hex < 10 ? '0' : 'A' - 10) + hex));
        hex = buf[i]  & 0xF;
        Serial.print((char) ((hex < 10 ? '0' : 'A' - 10) + hex));
        Serial.print(' ');
      }

      Serial.print('>');
      Serial.print(' ');
#endif

      if (ct == 0) return;

      switch (buf[0]) {
        case '?':
          transmitStatus();
          break;
        case '!':
          clock.setFast(buf[1] ? true : false);
          break;
        case 'C':
          moon.setColor(buf[1], buf[2], buf[3]);
          break;
        case 'T':
          clock.setTime(be2long(buf+1));
          break;
        case 'R':
          moon.setMoonriseSec(be2long(buf+1));
          break;
        case 'S':
          moon.setMoonsetSec(be2long(buf+1));
          break;
        case 'W':
          moon.setWidth(be2int(buf+1));
          break;
        case 'B':
          moon.setBrightness(buf[1]);
          break;
        case 'N':
          moon.setNumPixels(be2int(buf+1));
          break;
      }
#ifdef DEBUG
      Serial.println();
#endif

    }

    void checksumMismatch(byte *buf, int ct, uint32_t expected, uint32_t received)  {
      buf[ct] = 0;
#ifdef DEBUG
      Serial.println();
      Serial.print("CHECKSUM MISMATCH ");
      for (int i = 32 - 4; i >= 0; i -= 4) {
        int nybble = (expected >> i) & 0xF;
        Serial.print((char)(nybble + (nybble < 10 ? '0' : 'A' - 10)));
      }
      Serial.print(" =/= ");
      for (int i = 32 - 4; i >= 0; i -= 4) {
        int nybble = (received >> i) & 0xF;
        Serial.print((char)(nybble + (nybble < 10 ? '0' : 'A' - 10)));
      }
      Serial.print(" <");
      Serial.print((char *) buf);
      Serial.println('>');
#endif
    }

    void bufferExceeded(byte *buf, int ct) {
      buf[ct] = 0;
#ifdef DEBUG
      Serial.println();
      Serial.print("BUFFER EXCEEDED ");
      Serial.print('<');
      Serial.print((char *) buf);
      Serial.println('>');
#endif
    }



    void transmitStatus() {
      buf.messageMark[0] = '?';
      buf.rgb[0] = moon.r;
      buf.rgb[1] = moon.g;
      buf.rgb[2] = moon.b;
      int2be(moon.pixels.numPixels(), buf.numpixels);
      int2be(moon.moonWidth, buf.moonWidth);
      buf.moonBright[0] = moon.pixels.getBrightness();
      long2be(moon.moonriseSec, buf.moonrise);
      long2be(moon.moonsetSec, buf.moonset);
      long2be(clock.getTime(), buf.time);
      buf.flags[0] = (clock.isFast() ? 1 : 0);

      writer.write((void *)&buf, 0, sizeof(buf));
    }
};

// PINOUT

const byte txPin = 9;
const byte rxPin = 8;
SoftwareSerial bt(rxPin, txPin);
MoonDrawer moonDrawer(6);
MoonClock moonClock(moonDrawer);
BtWriter writer(bt);
MoonController moonController(moonClock, moonDrawer, writer);

BtReader reader(bt, moonController);


void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#endif

  moonDrawer.setup();
  moonClock.setup();

  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  bt.begin(9600);

  moonController.setup();
  reader.setup();
  writer.setup();

}

void loop() {
  moonDrawer.loop();
  moonClock.loop();

  reader.loop();
  writer.loop();
  moonController.loop();

}
