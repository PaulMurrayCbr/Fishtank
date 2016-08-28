#define DEBUG

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

const int DS3232_I2C_ADDRESS = 0x68;

const long SECSPERDAY = 60L * 60L * 24L;

class ClockStuff {
  public:

#ifdef DEBUG
    boolean fast = true;
#else
    boolean fast = false;
#endif


    uint32_t ms;

    const uint32_t fastTickMs = 121L;
    const uint32_t slowTickMs = 60L * 616L; // golden ratio

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
      if (millis() - ms > fast ? fastTickMs : slowTickMs) {
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

      if (fast) {
        // compress a day into 30 seconds
        timeofdaySec = (timeofdaySec * 24L * 60L * 2) % SECSPERDAY;
      }

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
    int moonWidth;
    float r, g, b;
    long moonriseSec = 16L * 60L * 60L;
    long moonsetSec = 4L * 60L * 60L;
    long nightLenSec;

    MoonStuff(byte pin) : pin(pin), pixels(12, pin, NEO_GRB + NEO_KHZ800) {
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

    void setWidth(int newWidth) {
      moonWidth = newWidth;
      drawMoon();
    }

    void setBrightness(byte brightness) {
      pixels.setBrightness(brightness);
      drawMoon();
    }

    void setMoonriseSec(int _moonriseSec) {
      moonriseSec = _moonriseSec;
      drawMoon();
    }

    void setMoonsetSec(int _moonsetSec) {
      moonsetSec = _moonsetSec;
      drawMoon();
    }

    void setTimeSec(long s) {
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

        for (int i = moonCenter - moonWidth ; i <= moonCenter + moonWidth ; i ++ ) {
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

class SimpleChecksum {
  public:
    uint32_t checksumComputed;

    void clear() {
      checksumComputed = 0xDEBB1E;
    }

    void add(char c) {
      checksumComputed = ((checksumComputed << 19) ^ (checksumComputed >> 5) ^ c) & 0xFFFFFF;
    }
};

class BtReader {
  public:
    Stream &in;
    uint32_t mostRecentHeartbeatMs;

    enum BtStreamState {
      BT_START = 0,
      BT_GOT_0 = 1,
      BT_GOT_1 = 2,
      BT_GOT_2 = 3,
      BT_GOT_3 = 4,
      BT_CHECKSUM = 5,
      BT_ERROR = 6
    } state;


    class Callback {
      public:
        virtual void transition(BtReader::BtStreamState from, BtReader::BtStreamState to) {}
        virtual void gotBytes(byte *buf, int ct) = 0;
        virtual void bufferExceeded(byte *buf, int ct) {}
        virtual void checksumMismatch(byte *buf, int ct, uint32_t expected, uint32_t received)  {}
    } &callback;


    byte base64Chunk[5];

    static const int BUFLEN = 100;

    // the chunk decoder overflows, so we need to make room
    byte buffer[BUFLEN + 4];
    int bufCt;

    SimpleChecksum checksum;
    uint32_t incomingChecksum;

    BtReader(Stream &in, Callback &callback) : in(in), callback(callback) {
      transition_to_start();
    }

    void setup() {
      mostRecentHeartbeatMs = millis();
    }

    void loop() {
      while (in.available() > 0) {
        int ch = in.read();

        if (ch == -1) return;
        if (ch <= ' ') return; // ignore white space

        // asterisks are a heartbeat signal
        if (ch == '*') {
          mostRecentHeartbeatMs = millis();
          return;
        }

        if (ch == '<') { // abort current packet on a '<'
          transition_to_start();
          // fall though to the switch
        }

        switch (state) {
          case BT_START:
            if (ch == '<') {
              transitionTo(BT_GOT_0);
            }
            break;

          case BT_GOT_0:
            if (isBase64(ch)) {
              base64Chunk[0] = ch;
              transitionTo(BT_GOT_1);
            }
            else if (ch == '#') {
              incomingChecksum = 0;
              transitionTo(BT_CHECKSUM);
            }
            else if (ch == '<') {
              // ignore multiple start-angle.
            }
            else {
              transitionTo(BT_ERROR);
            }
            break;

          case BT_GOT_1:
            if (isBase64(ch)) {
              base64Chunk[1] = ch;
              transitionTo(BT_GOT_2);
            }
            else {
              transitionTo(BT_ERROR);
            }
            break;

          case BT_GOT_2:
            if (isBase64(ch)) {
              base64Chunk[2] = ch;
              transitionTo(BT_GOT_3);
            }
            else {
              transitionTo(BT_ERROR);
            }
            break;

          case BT_GOT_3:
            if (isBase64(ch)) {
              base64Chunk[3] = ch;

              if (handleValidChunk()) {
                transitionTo(BT_GOT_0);
              }
              else {
                transitionTo(BT_ERROR);
              }
            }
            else
              transitionTo(BT_ERROR);
            break;

          case BT_CHECKSUM:
            if (isBase64(ch)) {
              incomingChecksum = (incomingChecksum << 6) | (((uint32_t)sixBit(ch)) & 0x3F);
            }
            else if (ch == '>') {
              buffer[bufCt] = 0;

              if (checksum.checksumComputed != incomingChecksum) {
                callback.checksumMismatch(buffer, bufCt, checksum.checksumComputed, incomingChecksum);
              }
              else {
                callback.gotBytes(buffer, bufCt);
              }
              transition_to_start();
            }
            else {
              transitionTo(BT_ERROR);
            }
            break;

          case BT_ERROR:
            if (ch == '>') transition_to_start();
            // else stay in error
            break;
        }
      }
    }


    void transition_to_start() {
      bufCt = 0;
      checksum.clear();
      transitionTo(BT_START);
    }

    void transitionTo(BtStreamState newstate) {
      BtStreamState stateWas = state;
      state = newstate;
      callback.transition(stateWas, state);

    }

    boolean isBase64(byte ch) {
      return
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= 'a' && ch <= 'z') ||
        (ch >= '0' && ch <= '9') ||
        (ch == '+') || (ch == '/') || (ch == '=');
    }

    byte sixBit(byte c) {
      if (c >= 'A' && c <= 'Z') return c - 'A';
      if (c >= 'a' && c <= 'z') return c - 'a' + 26;
      if (c >= '0' && c <= '9') return c - '0' + 52;
      if (c == '+') return  62;
      if (c == '/') return 63;
      return 0;
    }

    boolean handleValidChunk() {
      if (base64Chunk[0] == '=' && base64Chunk[1] != '=') return false;
      if (base64Chunk[1] == '=' && base64Chunk[2] != '=') return false;
      if (base64Chunk[2] == '=' && base64Chunk[3] != '=') return false;

      if (base64Chunk[0] == '=') return true; // ok chunk, empty
      if (base64Chunk[1] == '=') return false; // bad chunk

      // staging area.
      // this stuff relies on sixbit returning zero for '='

      uint32_t stage =
        ((uint32_t)sixBit(base64Chunk[0]) << 18) |
        ((uint32_t)sixBit(base64Chunk[1]) << 12) |
        ((uint32_t)sixBit(base64Chunk[2]) << 6) |
        ((uint32_t)sixBit(base64Chunk[3]) << 0);

      if (bufCt >= BUFLEN) {
        buffer[BUFLEN] = 0;
        callback.bufferExceeded(buffer, bufCt);
        return false;
      }

      checksum.add(buffer[bufCt++] = (stage >> 16));

      if (base64Chunk[2] == '=') return true;

      if (bufCt >= BUFLEN) {
        buffer[BUFLEN] = 0;
        callback.bufferExceeded(buffer, bufCt);
        return false;
      }

      checksum.add(buffer[bufCt++] = (stage >> 8));

      if (base64Chunk[3] == '=') return true;

      if (bufCt >= BUFLEN) {
        buffer[BUFLEN] = 0;
        callback.bufferExceeded(buffer, bufCt);
        return false;
      }

      checksum.add(buffer[bufCt++] = (stage >> 0));

      return true;
    }
};

// TODO
// class BtReaderStream : public Stream

class BtWriter {
  public:
    SoftwareSerial &out;
    uint32_t mostRecentHeartbeatMs;
    SimpleChecksum checksum;

    char buf[513]; // half the max bluetoth packet size.
    int bufCt = 0;

    BtWriter(SoftwareSerial &out) : out(out) {}

    void setup() {
      mostRecentHeartbeatMs = millis();
    }

    void loop() {
      if (millis() - mostRecentHeartbeatMs > 10000) {
        mostRecentHeartbeatMs = millis();
        out.write('*');
      }
    }

    void write(char *bytes, int offs, int len) {
      flush();
      checksum.clear();
      putCh('<');

      bytes += offs;

      while (len > 0) {
        if (len >= 3) {
          put3(bytes);
          bytes += 3;
          len -= 3;
        }
        else if (len == 2)  {
          put2(bytes);
          bytes += 2;
          len -= 2;
        }
        else if (len == 1)  {
          put1(bytes);
          bytes++;
          len --;
        }
      }

      putCh('#');

      putCh(to64((checksum.checksumComputed >> 18) & 0x3F));
      putCh(to64((checksum.checksumComputed >> 12) & 0x3F));
      putCh(to64((checksum.checksumComputed >> 6) & 0x3F));
      putCh(to64((checksum.checksumComputed >> 0) & 0x3F));

      putCh('>');

      flush();
    }

    void put3(char *bytes) {
      checksum.add(bytes[0]);
      checksum.add(bytes[1]);
      checksum.add(bytes[2]);

      uint32_t stage = (((uint32_t)(byte)bytes[0]) << 16) | (((uint32_t)(byte)bytes[1]) << 8) | (((uint32_t)(byte)bytes[2]) << 0);

      putCh(to64((stage >> 18) & 0x3F));
      putCh(to64((stage >> 12) & 0x3F));
      putCh(to64((stage >> 6) & 0x3F));
      putCh(to64((stage >> 0) & 0x3F));
    }

    void put2(char *bytes) {
      checksum.add(bytes[0]);
      checksum.add(bytes[1]);

      uint32_t stage = (((uint32_t)(byte)bytes[0]) << 16) | (((uint32_t)(byte)bytes[1]) << 8) ;

      putCh(to64((stage >> 18) & 0x3F));
      putCh(to64((stage >> 12) & 0x3F));
      putCh(to64((stage >> 6) & 0x3F));
      putCh('=');
    }

    void put1(char *bytes) {
      checksum.add(*bytes);

      uint32_t stage = (((uint32_t)(byte)bytes[0]) << 16) ;

      putCh(to64((stage >> 18) & 0x3F));
      putCh(to64((stage >> 12) & 0x3F));
      putCh('=');
      putCh('=');
    }

    // for completeness
    void put0(char *bytes) {
      putCh('=');
      putCh('=');
      putCh('=');
      putCh('=');
    }

    char to64(char in) {
      if (in < 26) return 'A' + in;
      else if (in < 52) return 'a' + in - 26;
      else if (in < 62) return '0' + in - 52;
      else if (in == 62) return '+';
      else if (in == 63) return '/';
      else return '?';
    }

    void putCh(char c) {
      if (bufCt >= sizeof(buf)) {
        flush();
      }

      buf[bufCt++] = c;
    }

    void flush() {

      // my bluetooth connectin seems to be extremely screwed up, for some reason.
      // the only thing that woks is sending the charactrs one at a time

      if (bufCt > 0) {
        buf[bufCt] = '\0';
        logp("writing to BT: ");

        for (int i = 0; i < bufCt; i++) {
          logp(buf[i]);
          out.write(buf[i]);
          delay(100);
        }

        bufCt = 0;
        logln();
      }
      out.flush();
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
    ClockStuff &clock;
    MoonStuff &moon;
    BtWriter &writer;

    MoonController(ClockStuff &clock,  MoonStuff &moon, BtWriter &writer) : clock(clock), moon(moon), writer(writer) {}

    void setup() {
    }

    void loop() {
    }

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

      Serial.println('>');
#endif

      if (ct == 0) return;

      switch (buf[0]) {
        case '?':
          transmitStatus();
          break;
        case '!' :
          clock.fast = !clock.fast;
          break;
        case 'C':
          moon.setColor(buf[1], buf[2], buf[3]);
          break;
        case 'T':
          //clock.setTime(((uint32_t)(buf[1]) << 24) | ((uint32_t)(buf[2]) << 16) | ((uint32_t)(buf[3]) << 8) | ((uint32_t)(buf[4]) << 0));
          break;
        case 'R':
          moon.setMoonriseSec(((uint32_t)(buf[1]) << 24) | ((uint32_t)(buf[2]) << 16) | ((uint32_t)(buf[3]) << 8) | ((uint32_t)(buf[4]) << 0));
          break;
        case 'S':
          moon.setMoonsetSec(((uint32_t)(buf[1]) << 24) | ((uint32_t)(buf[2]) << 16) | ((uint32_t)(buf[3]) << 8) | ((uint32_t)(buf[4]) << 0));
          break;
        case 'W':
          moon.setWidth(((uint16_t)(buf[1]) << 8) | ((uint16_t)(buf[2]) << 0));
          break;
        case 'B':
          moon.setBrightness(buf[1]);
          break;
        case 'N':
          moon.setNumPixels(((uint16_t)(buf[1]) << 8) | ((uint16_t)(buf[2]) << 0));
          break;
      }

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
      buf.numpixels[0] = moon.pixels.numPixels() >> 8;
      buf.numpixels[1] = moon.pixels.numPixels();
      buf.moonWidth[0] = moon.moonWidth >> 8;
      buf.moonWidth[1] = moon.moonWidth;
      buf.moonBright[0] = moon.pixels.getBrightness();
      buf.moonrise[0] = moon.moonriseSec >> 24;
      buf.moonrise[1] = moon.moonriseSec >> 16;
      buf.moonrise[2] = moon.moonriseSec >> 8;
      buf.moonrise[3] = moon.moonriseSec >> 0;
      buf.moonset[0] = moon.moonsetSec >> 24;
      buf.moonset[1] = moon.moonsetSec >> 16;
      buf.moonset[2] = moon.moonsetSec >> 8;
      buf.moonset[3] = moon.moonsetSec >> 0;
      buf.time[0] = clock.timeofdaySec >> 24;
      buf.time[1] = clock.timeofdaySec >> 16;
      buf.time[2] = clock.timeofdaySec >> 8;
      buf.time[3] = clock.timeofdaySec >> 0;

      buf.flags[0] = (clock.fast ? 1 : 0);

      writer.write((void *)&buf, 0, sizeof(buf));
    }
};

// PINOUT

const byte txPin = 9;
const byte rxPin = 8;
SoftwareSerial bt(rxPin, txPin);
MoonStuff moonStuff(6);
MoonClock moonClock(moonStuff);
BtWriter writer(bt);
MoonController moonController(moonClock, moonStuff, writer);

BtReader reader(bt, moonController);


void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#endif

  moonStuff.setup();
  moonClock.setup();

  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  bt.begin(9600);

  moonController.setup();
  reader.setup();
  writer.setup();

}

void loop() {
  moonStuff.loop();
  moonClock.loop();

  reader.loop();
  writer.loop();
  moonController.loop();

}
