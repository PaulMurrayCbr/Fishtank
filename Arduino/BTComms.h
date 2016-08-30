/**
 * BTComms - checksummed tranfer of bytes over bluetooth.
 * 
 * This class implements a custom protocol for exchanging blocks of bytes with 
 * a peer - in my case, and Android app.
 * 
 * The protocol is:
 * 
 * 1 - whitespace is ignored
 * 2 - an asterisk is transmitted every 10 seconds and is used as a heartbeat
 * 3 - packets are of the form <DATA#CHECKSUM>
 * 
 * DATA is base64 encoded data
 * CHECKSUM is a base64 encoded 24-bit checksum.
 * 
 * The checksum is seeded with the constant 0xDEBB1E, and to add a byte you roll right by 5
 * and XOR.
 * 
 * A block may compriose several bluetooth packets. When the incoming data is unparseable,
 * the packet is dropped and the stream is consumed until the next '<'. So, packets may bve dropped,
 * but if the BtReader callback gets a packet, it is probably good.
 */


#ifndef pmurray_at_bigpond_dot_com_BTComms
#define pmurray_at_bigpond_dot_com_BTComms 1

#include <Arduino.h>
#include <SoftwareSerial.h>

class SimpleChecksum {
  public:
    uint32_t checksumComputed;
    void clear();
    void add(char c);
};


class BtReader {
  private:
    Stream &in;
    uint32_t mostRecentHeartbeatMs;
    byte base64Chunk[5];
    static const int BUFLEN = 100;
    // the chunk decoder overflows, so we need to make room
    byte buffer[BUFLEN + 4];
    int bufCt;
    SimpleChecksum checksum;
    uint32_t incomingChecksum;

  public:
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

    BtReader(Stream &in, Callback &callback);
    void setup();
    void loop();

  private:
    void transition_to_start();
    void transitionTo(BtStreamState newstate);
    boolean isBase64(byte ch);
    byte sixBit(byte c);
    boolean handleValidChunk();
};

class BtWriter {
  private:
    SoftwareSerial &out;
    uint32_t mostRecentHeartbeatMs;
    SimpleChecksum checksum;

    char buf[513]; // half the max bluetoth packet size.
    int bufCt = 0;

  public:
    BtWriter(SoftwareSerial &out) : out(out) {}
    void setup();
    void loop();
    void write(char *bytes, int offs, int len);
    void flush();

  private:
    void put3(char *bytes);
    void put2(char *bytes);
    void put1(char *bytes);
    void put0(char *bytes);

    char BtWriter::to64(char in);

    void putCh(char c);

};

#endif
