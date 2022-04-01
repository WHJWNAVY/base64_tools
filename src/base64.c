#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <libgen.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>

#include "base64.h"

static const char base64_enc_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char base64_enc_pad = '=';

/* (From RFC1521 and draft-ietf-dnssec-secext-03.txt)
   The following encoding technique is taken from RFC 1521 by Borenstein
   and Freed.  It is reproduced here in a slightly edited form for
   convenience.

   A 65-character subset of US-ASCII is used, enabling 6 bits to be
   represented per printable character. (The extra 65th character, "=",
   is used to signify a special processing function.)

   The encoding process represents 24-bit groups of input bits as output
   strings of 4 encoded characters. Proceeding from left to right, a
   24-bit input group is formed by concatenating 3 8-bit input groups.
   These 24 bits are then treated as 4 concatenated 6-bit groups, each
   of which is translated into a single digit in the base64 alphabet.

   Each 6-bit group is used as an index into an array of 64 printable
   characters. The character referenced by the index is placed in the
   output string.

                         Table 1: The Base64 Alphabet

      Value Encoding  Value Encoding  Value Encoding  Value Encoding
          0 A            17 R            34 i            51 z
          1 B            18 S            35 j            52 0
          2 C            19 T            36 k            53 1
          3 D            20 U            37 l            54 2
          4 E            21 V            38 m            55 3
          5 F            22 W            39 n            56 4
          6 G            23 X            40 o            57 5
          7 H            24 Y            41 p            58 6
          8 I            25 Z            42 q            59 7
          9 J            26 a            43 r            60 8
         10 K            27 b            44 s            61 9
         11 L            28 c            45 t            62 +
         12 M            29 d            46 u            63 /
         13 N            30 e            47 v
         14 O            31 f            48 w         (pad) =
         15 P            32 g            49 x
         16 Q            33 h            50 y

   Special processing is performed if fewer than 24 bits are available
   at the end of the data being encoded.  A full encoding quantum is
   always completed at the end of a quantity.  When fewer than 24 input
   bits are available in an input group, zero bits are added (on the
   right) to form an integral number of 6-bit groups.  Padding at the
   end of the data is performed using the '=' character.

   Since all base64 input is an integral number of octets, only the
         -------------------------------------------------
   following cases can arise:

       (1) the final quantum of encoding input is an integral
           multiple of 24 bits; here, the final unit of encoded
       output will be an integral multiple of 4 characters
       with no "=" padding,
       (2) the final quantum of encoding input is exactly 8 bits;
           here, the final unit of encoded output will be two
       characters followed by two "=" padding characters, or
       (3) the final quantum of encoding input is exactly 16 bits;
           here, the final unit of encoded output will be three
       characters followed by one "=" padding character.
   */

int32_t base64_encode(const void *src, size_t srclength, void *dest, size_t targsize) {
    const uint8_t *_src_ = src;
    char *target = dest;
    size_t datalength = 0;
    uint8_t input[3] = {0};
    uint8_t output[4] = {0};
    int32_t i = 0;

    while (2 < srclength) {
        input[0] = *_src_++;
        input[1] = *_src_++;
        input[2] = *_src_++;
        srclength -= 3;

        output[0] = input[0] >> 2;
        output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
        output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
        output[3] = input[2] & 0x3f;

        if (datalength + 4 > targsize)
            return (-1);
        target[datalength++] = base64_enc_map[output[0]];
        target[datalength++] = base64_enc_map[output[1]];
        target[datalength++] = base64_enc_map[output[2]];
        target[datalength++] = base64_enc_map[output[3]];
    }

    /* Now we worry about padding. */
    if (0 != srclength) {
        /* Get what's left. */
        input[0] = input[1] = input[2] = '\0';
        for (i = 0; i < srclength; i++)
            input[i] = *_src_++;

        output[0] = input[0] >> 2;
        output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
        output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);

        if (datalength + 4 > targsize)
            return (-1);
        target[datalength++] = base64_enc_map[output[0]];
        target[datalength++] = base64_enc_map[output[1]];
        if (srclength == 1)
            target[datalength++] = base64_enc_pad;
        else
            target[datalength++] = base64_enc_map[output[2]];
        target[datalength++] = base64_enc_pad;
    }
    if (datalength >= targsize)
        return (-1);
    target[datalength] = '\0'; /* Returned value doesn't count \0. */
    return (datalength);
}

/* skips all whitespace anywhere.
   converts characters, four at a time, starting at (or after)
   src from base - 64 numbers into three 8 bit bytes in the target area.
   it returns the number of data bytes stored at the target, or -1 on error.
 */
int32_t base64_decode(const void *src, void *dest, size_t targsize) {
    const char *_src_ = src;
    uint8_t *target = dest;
    int32_t tarindex = 0, state = 0, ch = 0;
    uint8_t nextbyte = 0;
    char *pos = NULL;

    state = 0;
    tarindex = 0;

    while ((ch = (uint8_t)*_src_++) != '\0') {
        if (isspace(ch)) /* Skip whitespace anywhere. */
            continue;

        if (ch == base64_enc_pad)
            break;

        pos = strchr(base64_enc_map, ch);
        if (pos == 0) /* A non-base64 character. */
            return (-1);

        switch (state) {
            case 0:
                if (target) {
                    if (tarindex >= targsize)
                        return (-1);
                    target[tarindex] = (pos - base64_enc_map) << 2;
                }
                state = 1;
                break;
            case 1:
                if (target) {
                    if (tarindex >= targsize)
                        return (-1);
                    target[tarindex] |= (pos - base64_enc_map) >> 4;
                    nextbyte = ((pos - base64_enc_map) & 0x0f) << 4;
                    if (tarindex + 1 < targsize)
                        target[tarindex + 1] = nextbyte;
                    else if (nextbyte)
                        return (-1);
                }
                tarindex++;
                state = 2;
                break;
            case 2:
                if (target) {
                    if (tarindex >= targsize)
                        return (-1);
                    target[tarindex] |= (pos - base64_enc_map) >> 2;
                    nextbyte = ((pos - base64_enc_map) & 0x03) << 6;
                    if (tarindex + 1 < targsize)
                        target[tarindex + 1] = nextbyte;
                    else if (nextbyte)
                        return (-1);
                }
                tarindex++;
                state = 3;
                break;
            case 3:
                if (target) {
                    if (tarindex >= targsize)
                        return (-1);
                    target[tarindex] |= (pos - base64_enc_map);
                }
                tarindex++;
                state = 0;
                break;
        }
    }

    /*
     * We are done decoding Base-64 chars.  Let's see if we ended
     * on a byte boundary, and/or with erroneous trailing characters.
     */

    if (ch == base64_enc_pad) { /* We got a pad char. */
        ch = (uint8_t)*_src_++; /* Skip it, get next. */
        switch (state) {
            case 0: /* Invalid = in first position */
            case 1: /* Invalid = in second position */
                return (-1);

            case 2: /* Valid, means one byte of info */
                /* Skip any number of spaces. */
                for (; ch != '\0'; ch = (uint8_t)*_src_++)
                    if (!isspace(ch))
                        break;
                /* Make sure there is another trailing = sign. */
                if (ch != base64_enc_pad)
                    return (-1);
                ch = (uint8_t)*_src_++; /* Skip the = */
                                        /* Fall through to "single trailing =" case. */
                                        /* FALLTHROUGH */

            case 3: /* Valid, means two bytes of info */
                /*
             * We know this char is an =.  Is there anything but
             * whitespace after it?
             */
                for (; ch != '\0'; ch = (uint8_t)*_src_++)
                    if (!isspace(ch))
                        return (-1);

                /*
             * Now make sure for cases 2 and 3 that the "extra"
             * bits that slopped past the last full byte were
             * zeros.  If we don't check them, they become a
             * subliminal channel.
             */
                if (target && tarindex < targsize && target[tarindex] != 0)
                    return (-1);
        }
    } else {
        /*
         * We ended by seeing the end of the string.  Make sure we
         * have no partial bytes lying around.
         */
        if (state != 0)
            return (-1);
    }

    /* Null-terminate if we have room left */
    if (tarindex < targsize)
        target[tarindex] = 0;

    return (tarindex);
}

#if 0
static const uint8_t base64_test_dec[] = {"QmFzZTY0IGVuY29kaW5nIHRlc3QgcGFzc2VkIVFtRnpaVFkwSUdWdVkyOWthVzVuSUhSbGMzUWdjR0Z6YzJWa0lRPT1RbUZ6WlRZMElHVnVZMj"}; // don't include '\0'
static const uint8_t base64_test_enc[] = {"UW1GelpUWTBJR1Z1WTI5a2FXNW5JSFJsYzNRZ2NHRnpjMlZrSVZGdFJucGFWRmt3U1VkV2RWa3lPV3RoVnpWdVNVaFNiR016VVdkalIwWjZZekpXYTBsUlBUMVJiVVo2V2xSWk1FbEhWblZaTWo="};

/*
 * Checkup routine
 */
int32_t base64_self_test(void)
{
    int32_t ret = 0;
    size_t len = 0;
    const uint8_t *src = NULL;
    uint8_t buffer[1024] = {0};

    len = strlen(base64_test_dec);
    src = base64_test_dec;

    ret = base64_encode(src, len, buffer, sizeof(buffer));
    if (ret <= 0)
    {
        printf("Base64 encoding failed!\n");
        return (1);
    }
    printf("Base64 encoding [%s]\n", buffer);
    if ((ret != strlen(base64_test_enc)) || (memcmp(base64_test_enc, buffer, strlen(base64_test_enc)) != 0))
    {
        printf("Base64 encoding test failed!\n");
        return (1);
    }

    printf("Base64 encoding test passed!\n");

    len = sizeof(buffer);
    src = base64_test_enc;
    memset(buffer, 0, len);
    ret = base64_decode(src, buffer, len);
    if (ret <= 0)
    {
        printf("Base64 decoding failed!\n");
        return (1);
    }

    if ((ret != strlen(base64_test_dec)) || (memcmp(base64_test_dec, buffer, strlen(base64_test_dec)) != 0))
    {
        printf("Base64 decoding test failed!\n");
        return (1);
    }

    printf("Base64 decoding test passed!\n");

    return (0);
}

int32_t main(int32_t argc, char **argv)
{
    return base64_self_test();
}
#endif
