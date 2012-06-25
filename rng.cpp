#include "rng.h"
#include "rc4.h"
#include <stdio.h>
#include <stdlib.h>

static RC4 rc4;

int rngInitUrandom(int size)
{
    FILE* f = fopen("/dev/urandom", "r");
    if(!f)
    {
        fprintf(stderr, "Can't init RNG using /dev/urandom");
        return -1;
    }
    char* buf = (char*)malloc(size);
    fread(buf, 1, size, f);
    fclose(f);

    rc4 = RC4(buf, size);
    rc4.skip(0x1000);

    free(buf);
    
    return 0;
}

void rngInit(void* seed, unsigned int size)
{
    rc4 = RC4(seed, size);
    rc4.skip(0x1000);
}

void rngGet(void* dst, unsigned int size)
{
    unsigned char* ptr = (unsigned char*)dst;
    for(unsigned int i=0; i<size; i++)
    {
        *ptr = rc4.keyItem();
        ptr++;
    }
}
