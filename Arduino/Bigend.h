/**
 * Simple utility functions to covert byte arrays into 
 * primitive types big-endian
 */

#ifndef pmurray_at_bigpond_dot_com_Bigend
#define pmurray_at_bigpond_fot_com_Bigend

#include <Arduino.h>

uint16_t be2int(byte *buf);
uint32_t be2long(byte *buf);

void int2be(uint16_t n, byte *buf);
void long2be(uint32_t n, byte *buf);

#endif
