#ifndef RC4_H
#define RC4_H

class RC4
{
private:
    unsigned char s[256];
    unsigned int counter1;
    unsigned int counter2;

private:
    void init(unsigned char* key, unsigned int keyLength);

public:
    RC4();
    RC4(void* key, unsigned int keyLength);

    void encode(void* m, unsigned int length);
    void decode(void* c, unsigned int length);
    void skip(unsigned int size);
    unsigned char keyItem();
};

#endif
