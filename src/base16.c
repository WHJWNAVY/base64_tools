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

#define LOG(LEVEL, FMT, ...)                                                     \
    do {                                                                         \
        fprintf(stderr, "(%s:%d) " FMT "\n", __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define PRINT_DEBUG(FMT, ...) LOG(LOG_DEBUG, FMT, ##__VA_ARGS__)
#define PRINT_ERROR(FMT, ...) LOG(LOG_ERR, FMT, ##__VA_ARGS__)

#define BASE16_LEN 16
char BASE16_CHARS[BASE16_LEN + 1] = {"0123456789ABCDEF"};

#define BASE16_CHR2INT(c)                              \
    ({                                                 \
        uint8_t __i__ = 0, __v__ = 0;                  \
        uint8_t __c__ = (c);                           \
        for (__i__ = 0; __i__ < BASE16_LEN; __i__++) { \
            if (__c__ == BASE16_CHARS[__i__]) {        \
                __v__ = __i__;                         \
                break;                                 \
            }                                          \
        }                                              \
        __v__;                                         \
    })

int base16_encode(uint8_t *input, uint32_t slen, uint8_t **output, uint32_t *olen) {
    uint32_t i = 0, blen = 0;
    uint8_t *buffer = NULL;
    if (!input || !output || !slen) {
        return -1;
    }

    blen = slen * 2;
    buffer = malloc(blen + 1);
    if (!buffer) {
        return -1;
    }
    memset(buffer, 0, blen);
    for (i = 0; i < slen; i++) {
        buffer[i * 2] = BASE16_CHARS[input[i] >> 4];
        buffer[i * 2 + 1] = BASE16_CHARS[input[i] & 0x0F];
    }
    if (olen) {
        *olen = blen;
    }
    *output = buffer;
    return 0;
}

int base16_decode(uint8_t *input, uint32_t slen, uint8_t **output, uint32_t *olen) {
    uint32_t i = 0, j = 0;
    uint32_t blen = 0;
    uint8_t *buffer = NULL;
    uint8_t hv = 0, lv = 0;
    if (!input || !output || !olen) {
        return -1;
    }

    if (!slen) {
        slen = strlen(input);
    }

    blen = slen / 2;
    buffer = malloc(blen);
    if (!buffer) {
        return -1;
    }
    memset(buffer, 0, blen);

    for (i = 0; i < blen; i++) {
        hv = BASE16_CHR2INT(input[i * 2]);
        lv = BASE16_CHR2INT(input[i * 2 + 1]);
        buffer[i] = (hv << 4) | lv;
    }
    *olen = blen;
    *output = buffer;
    return 0;
}

int read_file(const char *file, uint8_t **fbuff, uint32_t *pflen) {
    int ret = 0;
    FILE *fp = NULL;
    uint8_t *pbuff = NULL;
    uint32_t fsize = 0;
    uint32_t rsize = 0;

    if ((file == NULL) || (fbuff == NULL) || (pflen == NULL)) {
        ret = -1;
        goto err;
    }

    fp = fopen(file, "rb");
    if (fp == NULL) {
        ret = -1;
        goto err;
    }

    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    if (fsize <= 0) {
        ret = fsize;
        goto err;
    }
    fseek(fp, 0, SEEK_SET);

    pbuff = malloc(fsize);
    if (pbuff == NULL) {
        ret = fsize;
        goto err;
    }
    memset(pbuff, 0, fsize);

    while ((rsize = fread(pbuff, 1, fsize, fp)) <= 0) {
        if (errno == EINTR || errno == EAGAIN) {
            errno = 0;
            continue;
        }
        break;
    }

    if (rsize != fsize) {
        ret = rsize;
        goto err;
    }

    *fbuff = pbuff;
    *pflen = fsize;
    ret = 0;
err:
    if (fp != NULL) {
        fclose(fp);
    }
    if ((ret != 0) && (pbuff != NULL)) {
        free(pbuff);
    }
    return ret;
}

int write_file(const char *file, uint8_t *fbuff, uint32_t flen) {
    int ret = 0;
    FILE *fp = NULL;
    uint32_t len = 0;
    if ((file == NULL) || (fbuff == NULL) || (flen == 0)) {
        ret = -1;
        goto err;
    }
    fp = fopen(file, "wb");
    if (fp == NULL) {
        ret = -1;
        goto err;
    }
    len = fwrite(fbuff, 1, flen, fp);
    if (len != flen) {
        ret = -1;
        goto err;
    }
    ret = 0;
err:
    if (fp != NULL) {
        fclose(fp);
    }
    return ret;
}

static void print_usage(const char *exe_name) {
    printf("Base16 encode and decode tools.\r\n");
    printf("Usage: %s [options] [INPUT]...\r\n", exe_name);
    printf("Options:\r\n");
    printf("    -h,--help                        Show this help message.\r\n");
    printf("    -d,--decode                      Decode input. Default use encode.\r\n");
    printf("    -f <PATH>,--file=<PATH>          Iutput file path.\r\n");
    printf("    -o <PATH>,--output=<PATH>        Output file path.\r\n");
    printf("    -k <STRING>,--key=<STRING>       Encode/decode key.\r\n");
}

#define BASE16_OUT_BUFLEN (1024)
#define BASE16_OUT_FILE ("/tmp/base16.out")

int main(int argc, char **argv) {
    int32_t ret = 0;
    char *ascii = NULL;
    char *file = NULL;
    char *key = NULL;
    uint8_t *input = NULL;
    uint8_t *output = NULL;
    uint8_t *b16buf = NULL;

    uint32_t inlen = 0;
    uint32_t b16len = 0;

    bool is_decode = false;

    int opt = 0, opt_index = 0;

    static struct option long_options[] = {{"help", no_argument, 0, 'h'},         {"decode", no_argument, 0, 'd'},
                                           {"key", required_argument, 0, 'k'},    {"file", required_argument, 0, 'f'},
                                           {"output", required_argument, 0, 'o'}, {0, 0, 0, 0}};

    while ((opt = getopt_long(argc, argv, "f:o:dk:h", long_options, &opt_index)) != -1) {
        switch (opt) {
            case 0:
                if (strcmp("file", long_options[opt_index].name) == 0) {
                    file = optarg;
                }
                if (strcmp("output", long_options[opt_index].name) == 0) {
                    output = optarg;
                }
                if (strcmp("decode", long_options[opt_index].name) == 0) {
                    is_decode = true;
                }
                if (strcmp("key", long_options[opt_index].name) == 0) {
                    key = optarg;
                }
                break;
            case 'f':
                file = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case 'd':
                is_decode = true;
                break;
            case 'k':
                key = optarg;
                break;
            case 'h':
                ret = 1;
                goto err;
            default:
                PRINT_ERROR("Unknown option -- %c\n\n", opt);
                goto err;
        }
    }

    if (file != NULL) {
        PRINT_DEBUG("Input file [%s]!", file);
    }

    if (output != NULL) {
        PRINT_DEBUG("Output file [%s]!", output);
    }

    if (key != NULL) {
        PRINT_DEBUG("Input Key [%s]!", key);
        if (strlen(key) >= BASE16_LEN) {
            strncpy(BASE16_CHARS, key, BASE16_LEN);
            PRINT_DEBUG("Use Base16 Key [%s]!", BASE16_CHARS);
        }
    }

    // PRINT_DEBUG("Args [%d] [%d]!", optind, argc);

    if (file == NULL) {
        if (optind >= argc) {
            // PRINT_ERROR("Invalid args [%d] [%d]!", optind, argc);
            ret = 1;
            goto err;
        }
        ascii = argv[optind];
        inlen = strlen(ascii);
        input = malloc(inlen + 1);
        if (input == NULL) {
            PRINT_ERROR("Failed to malloc!");
            ret = -1;
            goto err;
        }
        memset(input, 0, inlen + 1);
        strncpy(input, ascii, inlen);
        PRINT_DEBUG("Get string [%s] size [%u]!", input, inlen);
    } else {
        ret = read_file(file, &input, &inlen);
        if ((ret != 0) || (input == NULL) || (inlen <= 0)) {
            PRINT_ERROR("Failed to open file [%s]!", file);
            ret = -1;
            goto err;
        }
        PRINT_DEBUG("Get file buff size [%u]!", inlen);
    }

    if (is_decode) {
        PRINT_DEBUG("Base16 Decode:");
        ret = base16_decode(input, inlen, &b16buf, &b16len);
        if ((ret != 0) || (b16buf == NULL) || (b16len <= 0)) {
            PRINT_ERROR("Base16 decode failed!");
            ret = -1;
            goto err;
        }
    } else {
        PRINT_DEBUG("Base16 Encode:");
        ret = base16_encode(input, inlen, &b16buf, &b16len);
        if ((ret != 0) || (b16buf == NULL) || (b16len <= 0)) {
            PRINT_ERROR("Base16 encode failed!");
            ret = -1;
            goto err;
        }
    }
    if ((output == NULL) && (b16len > BASE16_OUT_BUFLEN)) {
        output = BASE16_OUT_FILE;
        PRINT_DEBUG("base64 output buff [%u] too large, write to file [%s]!", b16len, output);
    }

    if (output != NULL) {
        ret = write_file(output, b16buf, b16len);
        if (ret != 0) {
            PRINT_ERROR("Failed to write buff [%u] to file [%s]!\n", b16len, output);
            ret = -1;
            goto err;
        }
    } else {
        printf("%s\n", b16buf);
    }
    ret = 0;
err:
    if (input != NULL) {
        free(input);
    }
    if (b16buf != NULL) {
        free(b16buf);
    }
    if (ret) {
        print_usage(argv[0]);
    }
    return ret;
}