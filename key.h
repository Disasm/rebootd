#ifndef KEY_H
#define KEY_H

#include <stdlib.h>

int readKey(const char* fileName);

void getDigest(void* digest, const void* seed, size_t seedSize); // 20-byte digest

#endif
