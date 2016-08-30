
#include <Arduino.h>
#include "Bigend.h"

uint16_t be2int(byte *buf) {
  return
    (((uint16_t)buf[0]) << 8) |
    (((uint16_t)buf[1]));
}

uint32_t be2long(byte *buf) {
  return
    (((uint32_t)buf[0]) << 24) |
    (((uint32_t)buf[1]) << 16) |
    (((uint32_t)buf[2]) << 8) |
    (((uint32_t)buf[3]));
}

void int2be(uint16_t n, byte *buf) {
  buf[0] = (byte)(n >> 8);
  buf[1] = (byte)(n);
}

void long2be(uint32_t n, byte *buf) {
  buf[0] = (byte)(n >> 24);
  buf[1] = (byte)(n >> 16);
  buf[2] = (byte)(n >> 8);
  buf[3] = (byte)(n);
}

