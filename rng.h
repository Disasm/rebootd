#ifndef RNG_H
#define RNG_H

int rngInitUrandom(int size);

void rngInit(void* seed, unsigned int size);

void rngGet(void* dst, unsigned int size);

#endif
