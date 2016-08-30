#include <Arduino.h>
#include <SoftwareSerial.h>

#include "BTComms.h"

void SimpleChecksum::clear() {
  checksumComputed = 0xDEBB1E;
}

void SimpleChecksum::add(char c) {
  checksumComputed = ((checksumComputed << 19) ^ (checksumComputed >> 5) ^ c) & 0xFFFFFF;
}


BtReader::BtReader(Stream &in, Callback &callback) : in(in), callback(callback) {
  transition_to_start();
}

void  BtReader::setup() {
  mostRecentHeartbeatMs = millis();
}

void  BtReader::loop() {
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


void BtReader::transition_to_start() {
  bufCt = 0;
  checksum.clear();
  transitionTo(BT_START);
}

void BtReader::transitionTo(BtStreamState newstate) {
  BtStreamState stateWas = state;
  state = newstate;
  callback.transition(stateWas, state);

}

boolean BtReader::isBase64(byte ch) {
  return
    (ch >= 'A' && ch <= 'Z') ||
    (ch >= 'a' && ch <= 'z') ||
    (ch >= '0' && ch <= '9') ||
    (ch == '+') || (ch == '/') || (ch == '=');
}

byte BtReader::sixBit(byte c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return  62;
  if (c == '/') return 63;
  return 0;
}

boolean BtReader::handleValidChunk() {
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

void BtWriter::setup() {
  mostRecentHeartbeatMs = millis();
}

void BtWriter::loop() {
  if (millis() - mostRecentHeartbeatMs > 10000) {
    mostRecentHeartbeatMs = millis();
    out.write('*');
  }
}


void BtWriter::write(char *bytes, int offs, int len) {
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


void BtWriter::put3(char *bytes) {
  checksum.add(bytes[0]);
  checksum.add(bytes[1]);
  checksum.add(bytes[2]);

  uint32_t stage = (((uint32_t)(byte)bytes[0]) << 16) | (((uint32_t)(byte)bytes[1]) << 8) | (((uint32_t)(byte)bytes[2]) << 0);

  putCh(to64((stage >> 18) & 0x3F));
  putCh(to64((stage >> 12) & 0x3F));
  putCh(to64((stage >> 6) & 0x3F));
  putCh(to64((stage >> 0) & 0x3F));
}

void BtWriter::put2(char *bytes) {
  checksum.add(bytes[0]);
  checksum.add(bytes[1]);

  uint32_t stage = (((uint32_t)(byte)bytes[0]) << 16) | (((uint32_t)(byte)bytes[1]) << 8) ;

  putCh(to64((stage >> 18) & 0x3F));
  putCh(to64((stage >> 12) & 0x3F));
  putCh(to64((stage >> 6) & 0x3F));
  putCh('=');
}

void BtWriter::put1(char *bytes) {
  checksum.add(*bytes);

  uint32_t stage = (((uint32_t)(byte)bytes[0]) << 16) ;

  putCh(to64((stage >> 18) & 0x3F));
  putCh(to64((stage >> 12) & 0x3F));
  putCh('=');
  putCh('=');
}

// for completeness
void BtWriter::put0(char *bytes) {
  putCh('=');
  putCh('=');
  putCh('=');
  putCh('=');
}


char BtWriter::to64(char in) {
  if (in < 26) return 'A' + in;
  else if (in < 52) return 'a' + in - 26;
  else if (in < 62) return '0' + in - 52;
  else if (in == 62) return '+';
  else if (in == 63) return '/';
  else return '?';
}

void BtWriter::putCh(char c) {
  if (bufCt >= sizeof(buf) - 1) {
    flush();
  }

  buf[bufCt++] = c;
}


void BtWriter::flush() {

  // my bluetooth connectin seems to be extremely screwed up, for some reason.
  // the only thing that woks is sending the charactrs one at a time

  if (bufCt > 0) {
    buf[bufCt] = '\0';

    for (int i = 0; i < bufCt; i++) {
      out.write(buf[i]);
      delay(100);
    }

    bufCt = 0;
  }
  out.flush();
}

