/**
 * @file lib_gps.h
 * @author Emery Nagy
 * @brief New GPS driver for SIM7080G
 * @version 0.1
 * @date 2023-02-23
 *
 */

#ifndef LIB_GPS_H_
#define LIB_GPS_H_

/* includes */
#include "system.h"
#include "alt_types.h"
#include "lib_gps_cmd.h"

/* public types */

/* generic result */
typedef enum {
    GPS_SUCCESS,
    GPS_TIMEOUT,
    GPS_ERROR
} lib_gps_result_E;


/* public API */
lib_gps_result_E lib_gps_init(alt_u32 UART_BASE, alt_u32 UART_IRQ);
lib_gps_result_E lib_gps_power_state_on(volatile unsigned int* GPIO_BASE);
lib_gps_result_E lib_gps_reset_module(void);
lib_gps_result_E lib_gps_turn_on_gps(void);
lib_gps_result_E lib_gps_turn_off_gps(void);
lib_gps_result_E lib_gps_read_gps(alt_u8* lat, alt_u8* longi);

#endif /* LIB_GPS_H_ */
