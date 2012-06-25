#include <stdio.h>
#include <stdlib.h>
#include "sha1.h"

static unsigned char* key = NULL;
static unsigned int keySize = 0;

int readKey(const char* fileName)
{
    FILE* f = fopen(fileName, "r");
    if(!f)
    {
        fprintf(stderr, "Can't open key file\n");
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if(size > 0x10000)
    {
        fprintf(stderr, "Key file is too big\n");
        fclose(f);
        return -1;
    }

    key = (unsigned char*)malloc(size);
    if(fread(key, 1, size, f) != (size_t)size)
    {
        fprintf(stderr, "Error reading key file\n");
        fclose(f);
        return -1;
    }
    keySize = size;

    fclose(f);

    return 0;
}

void getDigest(void* digest, const void* seed, size_t seedSize)
{
    SHA1Context ctx;

    SHA1Reset(&ctx);
    SHA1Input(&ctx, key, keySize);
    SHA1Input(&ctx, (unsigned char*)seed, seedSize);
    SHA1Result(&ctx, (unsigned char*)digest);
}
