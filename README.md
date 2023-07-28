# Base64 & Base16 encode and decode tool

## Base64

### Usage

```
$ base64
Base64 encode and decode tools.
Usage: base64 [options] [INPUT]...
Options:
    -h,--help                        Show this help message.
    -d,--decode                      Decode input. Default use encode.
    -f <PATH>,--file=<PATH>          Iutput file path.
    -o <PATH>,--output=<PATH>        Output file path.
```

### Example

```bash
$ ./base64 aabbccddeeffg
(main:488) Get string [aabbccddeeffg] size [13]!
YWFiYmNjZGRlZWZmZw==

$ ./base64 -d YWFiYmNjZGRlZWZmZw==
(main:488) Get string [YWFiYmNjZGRlZWZmZw==] size [20]!
aabbccddeeffg

$ ./base64 -f testfile
(main:141) Input file [testfile] size [17760]!
(main:156) Get file buff size [17760]!
(main:189) base64 output buff [23680] too large, write to file [base64.out]!
(main:193) output file name [base64.out]

$ ./base64 -d -f base64.out -o testfile.out
(main:141) Input file [base64.out] size [23680]!
(main:156) Get file buff size [23680]!
(main:193) output file name [testfile.out]

$ md5sum testfile
7917a2833ea4e129a46ca2cfe461171f  testfile

$ md5sum testfile.out
7917a2833ea4e129a46ca2cfe461171f  testfile.out
```



## Base16

### Usage

```
$ ./base16
Base16 encode and decode tools.
Usage: ./base16 [options] [INPUT]...
Options:
    -h,--help                        Show this help message.
    -d,--decode                      Decode input. Default use encode.
    -f <PATH>,--file=<PATH>          Iutput file path.
    -o <PATH>,--output=<PATH>        Output file path.
    -k <STRING>,--key=<STRING>       Encode/decode key.
```



### Example

```
$ ./base16 aabbccddeeffg
(main:330) Get string [aabbccddeeffg] size [13]!
(main:357) Base16 Encode:
61616262636364646565666667

$ ./base16 -d 61616262636364646565666667
(main:330) Get string [61616262636364646565666667] size [26]!
(main:346) Base16 Decode:
aabbccddeeffg

$ ./base16 -k qwertyuiopasdfgh aabbccddeeffg
(main:301) Input Key [qwertyuiopasdfgh]!
(main:305) Use Base16 Key [qwertyuiopasdfgh]!
(main:330) Get string [aabbccddeeffg] size [13]!
(main:357) Base16 Encode:
uwuwueueururututuyuyuuuuui

$ ./base16 -k qwertyuiopasdfgh -d uwuwueueururututuyuyuuuuui
(main:301) Input Key [qwertyuiopasdfgh]!
(main:305) Use Base16 Key [qwertyuiopasdfgh]!
(main:330) Get string [uwuwueueururututuyuyuuuuui] size [26]!
(main:346) Base16 Decode:
aabbccddeeffg

$ ./base16 -k base16qwrtyQWRTY789456 -f testfile -o output.encode
(main:291) Input file [testfile]!
(main:296) Output file [output.encode]!
(main:301) Input Key [base16qwrtyQWRTY789456]!
(main:305) Use Base16 Key [base16qwrtyQWRTY]!
(main:341) Get file buff size [21336]!
(main:357) Base16 Encode:

$ ./base16 -k base16qwrtyQWRTY789456 -f output.encode -d -o output.decode
(main:291) Input file [output.encode]!
(main:296) Output file [output.decode]!
(main:301) Input Key [base16qwrtyQWRTY789456]!
(main:305) Use Base16 Key [base16qwrtyQWRTY]!
(main:341) Get file buff size [42672]!
(main:346) Base16 Decode:

$ md5sum testfile output.decode
4bc6624631eed6c9db960300a76cf1a9  testfile
4bc6624631eed6c9db960300a76cf1a9  output.decode
```

