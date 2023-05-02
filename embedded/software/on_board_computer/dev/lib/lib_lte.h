/**
 * @file lib_lte.h
 * @author Emery Nagy
 * @brief New LTE driver for SIM7080G
 * @version 0.1
 * @date 2023-02-23
 *
 */

#ifndef LIB_LTE_H_
#define LIB_LTE_H_

/* includes */
#include "system.h"
#include "alt_types.h"
#include "lib_lte_cmd.h"

/* public defines */
#define LIB_LTE_RSSI_INVALID 99

/* public types */

/* generic result */
typedef enum {
    LTE_SUCCESS,
    LTE_TIMEOUT,
    LTE_ERROR
} lib_lte_result_E;


/* public API */
lib_lte_result_E lib_lte_init(alt_u32 UART_BASE, alt_u32 UART_IRQ);
lib_lte_result_E lib_lte_power_state_on(volatile unsigned int* GPIO_BASE);
lib_lte_result_E lib_lte_reset_module(void);
lib_lte_result_E lib_lte_get_signal_strength(alt_u8* rssi);
lib_lte_result_E lib_lte_get_sim_status(void);
lib_lte_result_E lib_lte_turn_off_radio(void);
lib_lte_result_E lib_lte_turn_on_radio(void);
lib_lte_result_E lib_lte_set_apn(void);
lib_lte_result_E lib_lte_attach_to_network(void);
lib_lte_result_E lib_lte_detach_from_network(void);
lib_lte_result_E lib_lte_setup_http_connection(alt_u8* addr, alt_u16 hdr_size, alt_u16 body_size);
lib_lte_result_E lib_lte_start_http_connection(void);
lib_lte_result_E lib_lte_end_http_connection(void);
lib_lte_result_E lib_lte_check_http_connection(void);
lib_lte_result_E lib_lte_clear_http_header(void);
lib_lte_result_E lib_lte_write_to_http_header(alt_u8* databuffer);
lib_lte_result_E lib_lte_clear_http_body(void);
lib_lte_result_E lib_lte_write_to_http_body(alt_u8* databuffer);
lib_lte_result_E lib_lte_post_http_request(alt_u8* endpoint, alt_u16* response_code, alt_u16* resp_len);
lib_lte_result_E lib_lte_get_http_response_data(alt_u16 length, alt_u16 start_addr, alt_u8* databuffer, alt_u16 data_size);
lib_lte_result_E lib_lte_turn_on_gps(void);
lib_lte_result_E lib_lte_turn_off_gps(void);
lib_lte_result_E lib_lte_read_gps(alt_u8* lat, alt_u8* longi);

#endif /* LIB_LTE_H_ */
