#ifndef __BASE64_H__
#define __BASE64_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

int32_t base64_encode(const void *src, size_t srclength, void *dest, size_t targsize);
int32_t base64_decode(const void *src, void *dest, size_t targsize);

#endif