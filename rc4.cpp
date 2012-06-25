#include "rc4.h"

RC4::RC4()
{
    init(0, 0);
}

RC4::RC4(void* key, unsigned int keyLength)
{
    init((unsigned char*)key, keyLength);
}

void RC4::init(unsigned char* key, unsigned int keyLength)
{
    for (int i = 0; i != 256; ++i)
    {
        s[i] = i;
    }

    counter1 = 0;
    counter2 = 0;

    if((!key)||(keyLength==0)) return;

    int j = 0;
    for (int i = 0; i != 256; ++i)
    {
        j = (j + s[i] + key[i % keyLength]) % 256;

        unsigned char temp = s[i];
        s[i] = s[j];
        s[j] = temp;
    }
}

unsigned char RC4::keyItem()
{
    unsigned char temp;

    counter1 = (counter1 + 1) % 256;
    counter2 = (counter2 + s[counter1]) % 256;

    temp = s[counter2];
    s[counter2] = s[counter1];
    s[counter1] = temp;

    return s[(temp + s[counter2]) % 256];
}

void RC4::encode(void* m, unsigned int length)
{
    unsigned char* ptr = (unsigned char*)m;

    for (unsigned int i = 0; i < length; i++)
    {
        *ptr ^= keyItem();
    }
}

void RC4::decode(void* c, unsigned int length)
{
    encode(c, length);
}

void RC4::skip(unsigned int size)
{
    for(unsigned int i=0; i<size; i++)
    {
        keyItem();
    }
}
