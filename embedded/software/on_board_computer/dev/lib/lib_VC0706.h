/**
 * @file lib_VC0706.h
 * @author Emery Nagy
 * @brief
 * @version 0.1
 * @date 2023-02-12
 *
 */

#ifndef LIB_VC0706_H_
#define LIB_VC0706_H_

/* HAL includes */
#include "system.h"

/* Public types */

/* result enum */
typedef enum {
    VC0706_SUCCESS,
    VC0706_TIMEOUT,
    VC0706_ERROR
} lib_VC0706_result_E;

/* Public API */

lib_VC0706_result_E lib_VC0706_init(alt_u32 UART_BASE, alt_u32 UART_IRQ);
lib_VC0706_result_E lib_VC0706_increase_baud(alt_u32 timeout_ms);
lib_VC0706_result_E lib_VC0706_cmd_reset_camera(alt_u32 timeout_ms);
lib_VC0706_result_E lib_VC0706_cmd_stop_frame(alt_u32 timeout_ms);
lib_VC0706_result_E lib_VC0706_cmd_start_frame(alt_u32 timeout_ms);
lib_VC0706_result_E lib_VC0706_cmd_get_fbuf_len(alt_u32* data, alt_u32 timeout_ms);
lib_VC0706_result_E lib_VC0706_cmd_get_fbuf_data(alt_u8* dataptr, alt_u32 timeout_ms,
                                                    alt_u32 image_start_addr, alt_u32 data_len);

#endif /* LIB_VC0706_H_ */
