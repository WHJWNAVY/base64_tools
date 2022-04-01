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

#include "base64.h"

#define LOG(LEVEL, FMT, ...)                                                     \
    do {                                                                         \
        fprintf(stderr, "(%s:%d) " FMT "\n", __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#define PRINT_DEBUG(FMT, ...) LOG(LOG_DEBUG, FMT, ##__VA_ARGS__)
#define PRINT_ERROR(FMT, ...) LOG(LOG_ERR, FMT, ##__VA_ARGS__)

static void print_usage(const char *exe_name) {
    printf("Base64 encode and decode tools.\r\n");
    printf("Usage: %s [options] [INPUT]...\r\n", exe_name);
    printf("Options:\r\n");
    printf("    -h,--help                        Show this help message.\r\n");
    printf("    -d,--decode                      Decode input. Default use encode.\r\n");
    printf("    -f <PATH>,--file=<PATH>          Iutput file path.\r\n");
    printf("    -o <PATH>,--output=<PATH>        Output file path.\r\n");
}

#define BASE64_OUT_BUFLEN (1024)
#define BASE64_OUT_FILE ("base64.out")

int main(int argc, char **argv) {
    int32_t ret = 0;
    char *ascii = NULL;
    char *file = NULL;
    char *input = NULL;
    char *output = NULL;
    char *b64buf = NULL;

    uint32_t buflen = 0;
    uint32_t b64len = 0;

    FILE *fp = NULL;
    FILE *fo = NULL;

    bool is_decode = false;

    int opt = 0, opt_index = 0;

    static struct option long_options[] = {{"help", no_argument, 0, 'h'},
                                           {"decode", no_argument, 0, 'd'},
                                           {"file", required_argument, 0, 'f'},
                                           {"output", required_argument, 0, 'o'},
                                           {0, 0, 0, 0}};

    while ((opt = getopt_long(argc, argv, "f:o:dh", long_options, &opt_index)) != -1) {
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
            case 'h':
                ret = 1;
                goto err;
            default:
                PRINT_ERROR("Unknown option -- %c\n\n", opt);
                goto err;
        }
    }

    // if (file != NULL)
    // {
    //  PRINT_DEBUG("Input file [%s]!", file);
    // }

    // if (output != NULL)
    // {
    //  PRINT_DEBUG("Output file [%s]!", output);
    // }

    // PRINT_DEBUG("Args [%d] [%d]!", optind, argc);

    if (file == NULL) {
        if (optind >= argc) {
            // PRINT_ERROR("Invalid args [%d] [%d]!", optind, argc);
            ret = 1;
            goto err;
        }
        ascii = argv[optind];
        buflen = strlen(ascii);
        input = malloc(buflen + 1);
        if (input == NULL) {
            PRINT_ERROR("Failed to malloc!");
            ret = -1;
            goto err;
        }
        memset(input, 0, buflen + 1);
        strncpy(input, ascii, buflen);
        PRINT_DEBUG("Get string [%s] size [%u]!", input, buflen);
    } else {
        fp = fopen(file, "r");
        if (fp == NULL) {
            PRINT_ERROR("Failed to open file [%s]!", file);
            ret = -1;
            goto err;
        }
        fseek(fp, 0, SEEK_END);
        buflen = ftell(fp);

        if (buflen <= 0) {
            PRINT_ERROR("Failed to get file [%s] size!", file);
            ret = -1;
            goto err;
        }
        fseek(fp, 0, SEEK_SET);

        PRINT_DEBUG("Input file [%s] size [%u]!", file, buflen);

        input = malloc(buflen + 1);
        if (input == NULL) {
            PRINT_ERROR("Failed to malloc!");
            ret = -1;
            goto err;
        }
        memset(input, 0, buflen + 1);

        if (buflen != fread(input, 1, buflen, fp)) {
            PRINT_ERROR("Failed to read file [%s] to buff [%u]!\n", file, buflen);
            ret = -1;
            goto err;
        }
        PRINT_DEBUG("Get file buff size [%u]!", buflen);
    }

    b64len = buflen * 2;
    b64buf = malloc(b64len);
    if (b64buf == NULL) {
        PRINT_ERROR("Failed to malloc!");
        ret = -1;
        goto err;
    }
    memset(b64buf, 0, b64len);

    if (is_decode) {
        ret = base64_decode(input, b64buf, b64len);
        if (ret <= 0) {
            PRINT_ERROR("Base64 decode failed!");
            ret = -1;
            goto err;
        }

        b64len = ret;
    } else {
        ret = base64_encode(input, buflen, b64buf, b64len);
        if ((ret <= 0) || (ret != strlen(b64buf))) {
            PRINT_ERROR("Base64 encode failed!");
            ret = -1;
            goto err;
        }

        b64len = ret;
    }
    if ((output == NULL) && (b64len > BASE64_OUT_BUFLEN)) {
        output = BASE64_OUT_FILE;
        PRINT_DEBUG("base64 output buff [%u] too large, write to file [%s]!", b64len, output);
    }

    if (output != NULL) {
        PRINT_DEBUG("output file name [%s]", output);
        fo = fopen(output, "w+");
        if (fo == NULL) {
            PRINT_ERROR("Failed to open file [%s]!", output);
            ret = -1;
            goto err;
        }

        if (b64len != fwrite(b64buf, 1, b64len, fo)) {
            PRINT_ERROR("Failed to write buff [%u] to file [%s]!\n", b64len, output);
            ret = -1;
            goto err;
        }
    } else {
        printf("%s\n", b64buf);
    }

    ret = 0;

err:
    if (input != NULL) {
        free(input);
    }
    if (b64buf != NULL) {
        free(b64buf);
    }
    if (fp != NULL) {
        fclose(fp);
    }
    if (fo != NULL) {
        fclose(fo);
    }
    if (ret) {
        print_usage(argv[0]);
    }
    return ret;
}