/**
 * @file lib_base64.h
 * @author Emery Nagy
 * @brief Encode data as base64 strings
 * @version 0.2
 * @date 2023-02-13
 * 
 */

#ifndef LIB_BASE64_H_
#define LIB_BASE64_H_

#include "system.h"
#include "alt_types.h"
#include "app_camera.h" // note this is for optimization - not good practice
#include "stdbool.h"


/* public defines */
#define LIB_BASE64_DEFAULT_CAMERA_ENCODE_SIZE (APP_CAMERA_DEFAULT_STREAM_CHUNK_SIZE/3)*(4)

/* public API */
unsigned char* lib_base64_encode(alt_u8* data, alt_u32 size, alt_u32* outsize);
bool lib_base64_encode_static(alt_u8* data_in, alt_u32 size_in, alt_u32* size_out, alt_u8* data_out, alt_u32 static_buf_maxsize);


#endif /* LIB_BASE64_H_ */