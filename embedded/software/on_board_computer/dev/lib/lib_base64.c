/**
 * @file lib_base64.c
 * @author Emery Nagy
 * @brief Encode data as base64 strings
 * @version 0.1
 * @date 2023-02-13
 * 
 */

/* HAL includes */
#include "system.h"

/* lib includes */
#include "lib_base64.h"

/* stdlib includes */
#include "stdlib.h"
#include "stdio.h"

/* macros */
#define LIB_BASE64_FIRST_ITEM(buf) (alt_u8)((buf[0] & 0b11111100) >> 2)
#define LIB_BASE64_SECOND_ITEM(buf) (alt_u8)(((buf[0] & 0b00000011) << 4) | ((buf[1] & 0b11110000) >> 4))
#define LIB_BASE64_THIRD_ITEM(buf) (alt_u8)(((buf[1] & 0b00001111) << 2) | ((buf[2] & 0b11000000) >> 6))
#define LIB_BASE64_FOURTH_ITEM(buf) (alt_u8)(buf[2] & 0b00111111)

/* private data */
static alt_u8 encoding[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
                            'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
                            'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1',
                            '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

/* public API */

/**
 * @brief encodes data chunk as null appended base64 string
 *
 * @param data data chunk to encode
 * @param size size of chunk
 * @param outsize size of null appended string
 * @return unsigned* with pointer to encoded string
 */
unsigned char* lib_base64_encode(alt_u8* data, alt_u32 size, alt_u32* outsize) {

    alt_u8 pad = 0;

    if (size == APP_CAMERA_DEFAULT_STREAM_CHUNK_SIZE) {
        *outsize = LIB_BASE64_DEFAULT_CAMERA_ENCODE_SIZE;
    }
    else if (size % 3 == 0) {
        *outsize = (size/3)*4;
    } else if (size % 3 == 1) {
        *outsize = ((size - 1)/3)*4 + 4; // double the main part + trailing byte and 2 pad
        pad = 2;
    } else {
        /* size % 3 == 2 */
        *outsize = ((size - 2)/3)*4 + 4; // double the main part + 2 trailing byte and pad
        pad = 1;
    }

    unsigned char* outmem = (unsigned char*) pvPortMalloc(*outsize + 1);

    /* process all except for the last 3 bytes */
    int outindex = 0;
    for (int i = 0; i <= (size - 3); i+=3) {
        alt_u8* curr_chunk = data + i;
        outmem[outindex] = encoding[LIB_BASE64_FIRST_ITEM(curr_chunk)];
        outmem[outindex + 1] = encoding[LIB_BASE64_SECOND_ITEM(curr_chunk)];
        outmem[outindex + 2] = encoding[LIB_BASE64_THIRD_ITEM(curr_chunk)];
        outmem[outindex + 3] = encoding[LIB_BASE64_FOURTH_ITEM(curr_chunk)];
        outindex += 4;
    }

    /* apply pad to last 3 bytes */
    if (pad == 2) {
        //printf("\nhere 2, outsize %d, realsize %d\n", *outsize, size);
        alt_u8 curr_chunk[] = {data[size - 1], 0x00, 0x00};
        outmem[outindex] = encoding[LIB_BASE64_FIRST_ITEM(curr_chunk)];
        outmem[outindex + 1] = encoding[LIB_BASE64_SECOND_ITEM(curr_chunk)];
        outmem[outindex + 2] = '=';
        outmem[outindex + 3] = '=';
    } else if (pad == 1) {
        //printf("\nhere outsize %d, realsize %d\n", *outsize, size);
        alt_u8 curr_chunk[] = {data[size - 2], data[size - 1], 0x00};
        outmem[outindex] = encoding[LIB_BASE64_FIRST_ITEM(curr_chunk)];
        outmem[outindex + 1] = encoding[LIB_BASE64_SECOND_ITEM(curr_chunk)];
        outmem[outindex + 2] = encoding[LIB_BASE64_THIRD_ITEM(curr_chunk)];
        outmem[outindex + 3] = '=';
    } else {
        alt_u8 curr_chunk[] = {data[size - 3], data[size - 2], data[size - 1]};
        outmem[outindex] = encoding[LIB_BASE64_FIRST_ITEM(curr_chunk)];
        outmem[outindex + 1] = encoding[LIB_BASE64_SECOND_ITEM(curr_chunk)];
        outmem[outindex + 2] = encoding[LIB_BASE64_THIRD_ITEM(curr_chunk)];
        outmem[outindex + 3] = encoding[LIB_BASE64_FOURTH_ITEM(curr_chunk)];
    }

    outmem[*outsize] = '\0'; /* increase speed later in pipeline*/

    return outmem;
}

/**
 * @brief encodes data chunk as null appended base64 string using provided static buffer
 *
 * @param data_in data input to be encoded
 * @param size_in size of data input
 * @param size_out size of valid encoded data chunk (not including NULL append)
 * @param data_out data output buffer (preallocated by caller)
 * @param static_buf_maxsize max size of data that can be encoded with the provided static buffer
 * @return true
 * @return false
 */
bool lib_base64_encode_static(alt_u8* data_in, alt_u32 size_in, alt_u32* size_out, alt_u8* data_out, alt_u32 static_buf_maxsize) {
    alt_u8 pad = 0;

    if (size_in == APP_CAMERA_DEFAULT_STREAM_CHUNK_SIZE) {
        *size_out = LIB_BASE64_DEFAULT_CAMERA_ENCODE_SIZE;
    }
    else if (size_in % 3 == 0) {
        *size_out = (size_in/3)*4;
    } else if (size_in % 3 == 1) {
        *size_out = ((size_in - 1)/3)*4 + 4; // double the main part + trailing byte and 2 pad
        pad = 2;
    } else {
        /* size % 3 == 2 */
        *size_out = ((size_in - 2)/3)*4 + 4; // double the main part + 2 trailing byte and pad
        pad = 1;
    }

    /* check that databuffer provided will fit encoded data */
    if (*size_out + 1 > static_buf_maxsize) {
        return false;
    }

    /* process all except for the last 3 bytes */
    int outindex = 0;
    for (int i = 0; i <= (size_in - 3); i+=3) {
        alt_u8* curr_chunk = data_in + i;
        data_out[outindex] = encoding[LIB_BASE64_FIRST_ITEM(curr_chunk)];
        data_out[outindex + 1] = encoding[LIB_BASE64_SECOND_ITEM(curr_chunk)];
        data_out[outindex + 2] = encoding[LIB_BASE64_THIRD_ITEM(curr_chunk)];
        data_out[outindex + 3] = encoding[LIB_BASE64_FOURTH_ITEM(curr_chunk)];
        outindex += 4;
    }

    /* apply pad to last 3 bytes */
    if (pad == 2) {
        //printf("\nhere 2, outsize %d, realsize %d\n", *outsize, size);
        alt_u8 curr_chunk[] = {data_in[size_in - 1], 0x00, 0x00};
        data_out[outindex] = encoding[LIB_BASE64_FIRST_ITEM(curr_chunk)];
        data_out[outindex + 1] = encoding[LIB_BASE64_SECOND_ITEM(curr_chunk)];
        data_out[outindex + 2] = '=';
        data_out[outindex + 3] = '=';
    } else if (pad == 1) {
        //printf("\nhere outsize %d, realsize %d\n", *outsize, size);
        alt_u8 curr_chunk[] = {data_in[size_in - 2], data_in[size_in - 1], 0x00};
        data_out[outindex] = encoding[LIB_BASE64_FIRST_ITEM(curr_chunk)];
        data_out[outindex + 1] = encoding[LIB_BASE64_SECOND_ITEM(curr_chunk)];
        data_out[outindex + 2] = encoding[LIB_BASE64_THIRD_ITEM(curr_chunk)];
        data_out[outindex + 3] = '=';
    } else {
        alt_u8 curr_chunk[] = {data_in[size_in - 3], data_in[size_in - 2], data_in[size_in - 1]};
        data_out[outindex] = encoding[LIB_BASE64_FIRST_ITEM(curr_chunk)];
        data_out[outindex + 1] = encoding[LIB_BASE64_SECOND_ITEM(curr_chunk)];
        data_out[outindex + 2] = encoding[LIB_BASE64_THIRD_ITEM(curr_chunk)];
        data_out[outindex + 3] = encoding[LIB_BASE64_FOURTH_ITEM(curr_chunk)];
    }

    data_out[*size_out] = '\0'; /* increase speed later in pipeline*/

    return true;
}