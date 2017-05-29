
/*
 * Copyright (c) 2012 Karl N. Redgate
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/** \file uuid.c
 * \brief 
 *
 */

#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "xuid.h"

static char hexchar[] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
static char hexval[] = {
  /* 0 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 31 */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1, /* 63 */
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 95 */
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 127 */
};

/**
 */
int
parse_uuid( char *string, uint8_t *data ) {
    data[ 0] = (hexval[ string[ 0] & 0x7F ] << 4) + (hexval[ string[ 1] & 0x7F ]);
    data[ 1] = (hexval[ string[ 2] & 0x7F ] << 4) + (hexval[ string[ 3] & 0x7F ]);
    data[ 2] = (hexval[ string[ 4] & 0x7F ] << 4) + (hexval[ string[ 5] & 0x7F ]);
    data[ 3] = (hexval[ string[ 6] & 0x7F ] << 4) + (hexval[ string[ 7] & 0x7F ]);
    // string[8] == '-'
    data[ 4] = (hexval[ string[ 9] & 0x7F ] << 4) + (hexval[ string[10] & 0x7F ]);
    data[ 5] = (hexval[ string[11] & 0x7F ] << 4) + (hexval[ string[12] & 0x7F ]);
    // string[13] == '-'
    data[ 6] = (hexval[ string[14] & 0x7F ] << 4) + (hexval[ string[15] & 0x7F ]);
    data[ 7] = (hexval[ string[16] & 0x7F ] << 4) + (hexval[ string[17] & 0x7F ]);
    // string[18] == '-'
    data[ 8] = (hexval[ string[19] & 0x7F ] << 4) + (hexval[ string[20] & 0x7F ]);
    data[ 9] = (hexval[ string[21] & 0x7F ] << 4) + (hexval[ string[22] & 0x7F ]);
    // string[23] == '-'
    data[10] = (hexval[ string[24] & 0x7F ] << 4) + (hexval[ string[25] & 0x7F ]);
    data[11] = (hexval[ string[26] & 0x7F ] << 4) + (hexval[ string[27] & 0x7F ]);
    data[12] = (hexval[ string[28] & 0x7F ] << 4) + (hexval[ string[29] & 0x7F ]);
    data[13] = (hexval[ string[30] & 0x7F ] << 4) + (hexval[ string[31] & 0x7F ]);
    data[14] = (hexval[ string[32] & 0x7F ] << 4) + (hexval[ string[33] & 0x7F ]);
    data[15] = (hexval[ string[34] & 0x7F ] << 4) + (hexval[ string[35] & 0x7F ]);
    return 1;
}

/**
 */
int
parse_guid( char *string, guid_t *data ) {
    uint32_t data1 = 0;
    data1 |= (hexval[ string[ 0] & 0x7F ] << 28);
    data1 |= (hexval[ string[ 1] & 0x7F ] << 24);
    data1 |= (hexval[ string[ 2] & 0x7F ] << 20);
    data1 |= (hexval[ string[ 3] & 0x7F ] << 16);
    data1 |= (hexval[ string[ 4] & 0x7F ] << 12);
    data1 |= (hexval[ string[ 5] & 0x7F ] << 8);
    data1 |= (hexval[ string[ 6] & 0x7F ] << 4);
    data1 |= (hexval[ string[ 7] & 0x7F ]);
    // string[8] == '-'
    data->data1 = data1;

    uint16_t data2 = 0;
    data2 |= (hexval[ string[ 9] & 0x7F ] << 12);
    data2 |= (hexval[ string[10] & 0x7F ] << 8);
    data2 |= (hexval[ string[11] & 0x7F ] << 4);
    data2 |= (hexval[ string[12] & 0x7F ]);
    // string[13] == '-'
    data->data2 = data2;

    uint16_t data3 = 0;
    data3 |= (hexval[ string[14] & 0x7F ] << 12);
    data3 |= (hexval[ string[15] & 0x7F ] << 8);
    data3 |= (hexval[ string[16] & 0x7F ] << 4);
    data3 |= (hexval[ string[17] & 0x7F ]);
    // string[18] == '-'
    data->data3 = data3;

    data->data4[0] = (hexval[ string[19] & 0x7F ] << 4) + (hexval[ string[20] & 0x7F ]);
    data->data4[1] = (hexval[ string[21] & 0x7F ] << 4) + (hexval[ string[22] & 0x7F ]);
    // string[23] == '-'
    data->data4[2] = (hexval[ string[24] & 0x7F ] << 4) + (hexval[ string[25] & 0x7F ]);
    data->data4[3] = (hexval[ string[26] & 0x7F ] << 4) + (hexval[ string[27] & 0x7F ]);
    data->data4[4] = (hexval[ string[28] & 0x7F ] << 4) + (hexval[ string[29] & 0x7F ]);
    data->data4[5] = (hexval[ string[30] & 0x7F ] << 4) + (hexval[ string[31] & 0x7F ]);
    data->data4[6] = (hexval[ string[32] & 0x7F ] << 4) + (hexval[ string[33] & 0x7F ]);
    data->data4[7] = (hexval[ string[34] & 0x7F ] << 4) + (hexval[ string[35] & 0x7F ]);

    return 1;
}

/**
 */
static uint8_t
nybble( uint8_t *data, int n ) {
    uint8_t byte = data[ (n>>1)&0xF ];
    return (n&1) ? byte & 0xF : (byte>>4) & 0xF;
}

/**
 */
int
format_uuid( char *string, uint8_t *data ) {
    string[ 0] = hexchar[ nybble(data, 0) ];
    string[ 1] = hexchar[ nybble(data, 1) ];
    string[ 2] = hexchar[ nybble(data, 2) ];
    string[ 3] = hexchar[ nybble(data, 3) ];
    string[ 4] = hexchar[ nybble(data, 4) ];
    string[ 5] = hexchar[ nybble(data, 5) ];
    string[ 6] = hexchar[ nybble(data, 6) ];
    string[ 7] = hexchar[ nybble(data, 7) ];
    string[ 8] = '-';
    string[ 9] = hexchar[ nybble(data, 8) ];
    string[10] = hexchar[ nybble(data, 9) ];
    string[11] = hexchar[ nybble(data,10) ];
    string[12] = hexchar[ nybble(data,11) ];
    string[13] = '-';
    string[14] = hexchar[ nybble(data,12) ];
    string[15] = hexchar[ nybble(data,13) ];
    string[16] = hexchar[ nybble(data,14) ];
    string[17] = hexchar[ nybble(data,15) ];
    string[18] = '-';
    string[19] = hexchar[ nybble(data,16) ];
    string[20] = hexchar[ nybble(data,17) ];
    string[21] = hexchar[ nybble(data,18) ];
    string[22] = hexchar[ nybble(data,19) ];
    string[23] = '-';
    string[24] = hexchar[ nybble(data,20) ];
    string[25] = hexchar[ nybble(data,21) ];
    string[26] = hexchar[ nybble(data,22) ];
    string[27] = hexchar[ nybble(data,23) ];
    string[28] = hexchar[ nybble(data,24) ];
    string[29] = hexchar[ nybble(data,25) ];
    string[30] = hexchar[ nybble(data,26) ];
    string[31] = hexchar[ nybble(data,27) ];
    string[32] = hexchar[ nybble(data,28) ];
    string[33] = hexchar[ nybble(data,29) ];
    string[34] = hexchar[ nybble(data,30) ];
    string[35] = hexchar[ nybble(data,31) ];
    return 1;
}

/**
 */
int
format_guid( char *string, guid_t *data ) {
    uint32_t data1 = data->data1;
    string[ 0] = hexchar[ (data1 >> 28) & 0xF ];
    string[ 1] = hexchar[ (data1 >> 24) & 0xF ];
    string[ 2] = hexchar[ (data1 >> 20) & 0xF ];
    string[ 3] = hexchar[ (data1 >> 16) & 0xF ];
    string[ 4] = hexchar[ (data1 >> 12) & 0xF ];
    string[ 5] = hexchar[ (data1 >>  8) & 0xF ];
    string[ 6] = hexchar[ (data1 >>  4) & 0xF ];
    string[ 7] = hexchar[  data1        & 0xF ];
    string[ 8] = '-';

    uint16_t data2 = data->data2;
    string[ 9] = hexchar[ (data2 >> 12) & 0xF ];
    string[10] = hexchar[ (data2 >>  8) & 0xF ];
    string[11] = hexchar[ (data2 >>  4) & 0xF ];
    string[12] = hexchar[  data2        & 0xF ];
    string[13] = '-';

    uint16_t data3 = data->data3;
    string[14] = hexchar[ (data3 >> 12) & 0xF ];
    string[15] = hexchar[ (data3 >>  8) & 0xF ];
    string[16] = hexchar[ (data3 >>  4) & 0xF ];
    string[17] = hexchar[  data3        & 0xF ];
    string[18] = '-';

    string[19] = hexchar[ (data->data4[0] >> 4) & 0xF ];
    string[20] = hexchar[  data->data4[0]       & 0xF ];
    string[21] = hexchar[ (data->data4[1] >> 4) & 0xF ];
    string[22] = hexchar[  data->data4[1]       & 0xF ];
    string[23] = '-';
    string[24] = hexchar[ (data->data4[2] >> 4) & 0xF ];
    string[25] = hexchar[  data->data4[2]       & 0xF ];
    string[26] = hexchar[ (data->data4[3] >> 4) & 0xF ];
    string[27] = hexchar[  data->data4[3]       & 0xF ];
    string[28] = hexchar[ (data->data4[4] >> 4) & 0xF ];
    string[29] = hexchar[  data->data4[4]       & 0xF ];
    string[30] = hexchar[ (data->data4[5] >> 4) & 0xF ];
    string[31] = hexchar[  data->data4[5]       & 0xF ];
    string[32] = hexchar[ (data->data4[6] >> 4) & 0xF ];
    string[33] = hexchar[  data->data4[6]       & 0xF ];
    string[34] = hexchar[ (data->data4[7] >> 4) & 0xF ];
    string[35] = hexchar[  data->data4[7]       & 0xF ];
    return 1;
}

/**
 */
int
compare_guid( guid_t *left, guid_t *right ) {
    long d = left->data1 - right->data1;
    if ( d != 0 )  return d;
    d = left->data2 - right->data2;
    if ( d != 0 )  return d;
    d = left->data3 - right->data3;
    if ( d != 0 )  return d;
    for ( int i = 0 ; i < 8 ; i++ ) {
        d = left->data4[i] - right->data4[i];
        if ( d != 0 )  return d;
    }
    return 0;
}

/* vim: set autoindent expandtab sw=4 syntax=c : */
