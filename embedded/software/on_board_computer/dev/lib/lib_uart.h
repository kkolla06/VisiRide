/**
 * @file lib_uart.h
 * @author Emery Nagy
 * @brief UART firmware driver
 * @version 0.1
 * @date 2023-02-11
 * 
 */

#ifndef LIB_UART_H_
#define LIB_UART_H_

/* HAL includes */
#include "alt_types.h"

/* Public types */

/**
 * @brief Generic uart rx irq function pointer, to recieve data, app layer must register its own callback of this type
 *        to the associated uart_config struct to receive uart data
 */
typedef void (*lib_uart_generic_rx_irq)(alt_8 rxdata);

/**
 * @brief Generic uart rx error function pointer
 *
 */
typedef void (*lib_uart_rx_error)(void);

/**
 * @brief configuration struct for UART hardware
 *
 */
typedef struct {

    /* UART base address */
    volatile unsigned int* uart_base;
    /* UART irq number */
    unsigned int uart_irq;
    /* UART RX irq */
    lib_uart_generic_rx_irq uart_rx_irq;
    /* UART RX error callback */
    lib_uart_rx_error uart_rx_error;

} lib_uart_config_S;

/**
 * @brief UART driver response enum
 *
 */
typedef enum {

    LIB_UART_SUCCESS,
    LIB_UART_ERROR

} lib_uart_resp_E;

/* Public API */
lib_uart_resp_E lib_uart_init(lib_uart_config_S* config);
lib_uart_resp_E lib_uart_tx(lib_uart_config_S* config, void* tx_buf, alt_u32 tx_len, alt_32 timeout_ms);
void lib_uart_set_baud(lib_uart_config_S* config, alt_u32 rate);

#endif /* LIB_UART_H_ */
