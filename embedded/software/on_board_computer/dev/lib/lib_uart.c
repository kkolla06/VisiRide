/**
 * @file lib_uart.c
 * @author Emery Nagy
 * @brief UART firmware driver
 * @version 0.1
 * @date 2023-02-11
 * 
 */


/* HAL includes */
#include "system.h"
#include "sys/alt_irq.h"
#include "priv/alt_legacy_irq.h"
#include "altera_avalon_uart_regs.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"

/* stdlib includes */
#include "stdio.h"

/* lib includes */
#include "lib_uart.h"


/* Private API */

/**
 * @brief Generic UART rx irq, processes interrupt and calls app layer irq as well as app rx error callback.
 *  App layer callbacks should be as lightweight as possible.
 *
 */
void lib_uart_rx_irq(void* isr_context, alt_u32 id) {

    lib_uart_config_S* config = (lib_uart_config_S*)isr_context;
    alt_u32 status = IORD_ALTERA_AVALON_UART_STATUS(config->uart_base);

    /* Clear any error flags set at the device */
    IOWR_ALTERA_AVALON_UART_STATUS(config->uart_base, 0);

    /* Dummy read to ensure IRQ is negated before ISR returns */
    IORD_ALTERA_AVALON_UART_STATUS(config->uart_base);

    /* Throw out any bad data */
    if (status & (ALTERA_AVALON_UART_STATUS_PE_MSK |
                    ALTERA_AVALON_UART_STATUS_FE_MSK|
                    ALTERA_AVALON_UART_STATUS_ROE_MSK))
    {

        /* call error callback */
        lib_uart_rx_error err_cb = config->uart_rx_error;
        err_cb();
        return;
    }

    /* process a read irq */
    if (status & ALTERA_AVALON_UART_STATUS_RRDY_MSK)
    {
        lib_uart_generic_rx_irq rx_cb = config->uart_rx_irq;
        rx_cb(IORD_ALTERA_AVALON_UART_RXDATA(config->uart_base));
    }

}


/* Public API */

/**
 * @brief initialize uart hardware
 *
 * @param config uart configuration struct
 * @return lib_uart_resp_E
 */
lib_uart_resp_E lib_uart_init(lib_uart_config_S* config) {

    /* Enable rx interrupts */
    alt_16 cntrl = ALTERA_AVALON_UART_CONTROL_RRDY_MSK |
                    ALTERA_AVALON_UART_CONTROL_PE_MSK |
                    ALTERA_AVALON_UART_CONTROL_FE_MSK |
                    ALTERA_AVALON_UART_CONTROL_ROE_MSK;

    IOWR_ALTERA_AVALON_UART_CONTROL(config->uart_base, cntrl);

    /* Register rx interrupt irq, calls uart generic rx interrupt irq */
    alt_irq_register(config->uart_irq, config, (alt_isr_func)lib_uart_rx_irq);

    return LIB_UART_SUCCESS;
}

/**
 * @brief Send tx buffer via UART
 *
 * @param config UART config struct
 * @param tx_buf UART transmit buffer
 * @param tx_len Size of transmit buffer in bytes
 * @return lib_uart_resp_E
 */
lib_uart_resp_E lib_uart_tx(lib_uart_config_S* config, void* tx_buf, alt_u32 tx_len, alt_32 timeout_ms) {

    lib_uart_resp_E result = LIB_UART_ERROR;

    /* Get uart status and only transmit if hardware ready */
    alt_u32 status = IORD_ALTERA_AVALON_UART_STATUS(config->uart_base);
    alt_u64 starting_timestamp = xTaskGetTickCount();
    alt_8* tx_ptr = tx_buf;
    alt_32 sent = 0;

    /* Only block app task for maximum amount of time defined by timeout_ms */
    while (((xTaskGetTickCount() - starting_timestamp)/portTICK_PERIOD_MS) < timeout_ms) {

        if (status & ALTERA_AVALON_UART_STATUS_TRDY_MSK) {
            IOWR_ALTERA_AVALON_UART_TXDATA(config->uart_base, *tx_ptr);
            sent++;

            if (sent == tx_len) {
                result = LIB_UART_SUCCESS;
                break;
            }
            tx_ptr++;
        }
        status = IORD_ALTERA_AVALON_UART_STATUS(config->uart_base);
    }

    return result;
}

/**
 * @brief change the UART system baud rate accoring to https://www.intel.com/content/www/us/en/docs/programmable/683130/22-3/divisor-register-optional.html
 *
 * @param config uart device to change
 * @param rate new baud rate to set
 */
void lib_uart_set_baud(lib_uart_config_S* config, alt_u32 rate) {
    printf("Setting %d\n", (CAMERA_FREQ / rate) - 1);
    IOWR_ALTERA_AVALON_UART_DIVISOR(config->uart_base, (CAMERA_FREQ/rate) - 1);
}