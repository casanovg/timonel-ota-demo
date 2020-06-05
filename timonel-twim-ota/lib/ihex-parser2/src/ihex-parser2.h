/*
 *******************************
 * Intel Hex Parser Library    *
 * Version: 0.1 "Benja"        *
 *******************************
 */

#ifndef _IHEX_PARSER2_H_
#define _IHEX_PARSER2_H_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_TYPE_INTEL_HEX 1
#define FILE_TYPE_RAW 2
#define DEBUGLVL 1
#define BYTESPERLINE 8

//#define TML_HEXPARSER_VERSION " Timonel Hex Parser version: 0.3"

// Prototypes
int parseRaw(char *hexfile, unsigned char *buffer, int *startAddr, int *endAddr);
int parseIntelHex(char *hexfile, unsigned char *buffer, int *startAddr, int *endAddr); /* taken from bootloadHID */
int parseUntilColon(FILE *fp);                                                         /* taken from bootloadHID */
int parseHex(FILE *p_file, uint8_t num_digits);                                                 /* taken from bootloadHID */
int use_ansi = 0;

// Globals
unsigned char dataBuffer[65536 + 256]; /* file data buffer */
int res;
char *file = NULL;


#endif  // _IHEX_PARSER2_H_