#ifndef sha256_h
#define sha256_h

#include <string.h>
#include <stdio.h>
#include <stdint.h>

void sha256(const unsigned char *data, int len, unsigned char *result);

#endif
