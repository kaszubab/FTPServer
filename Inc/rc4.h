#ifndef RC4_H_
#define RC4_H_

#include "stdint.h"

#define N 256

uint8_t RC4(char *key, char *plaintext, unsigned char *ciphertext);

#endif
