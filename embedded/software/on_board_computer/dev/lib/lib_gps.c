/**
 * @file lib_gps.c
 * @author Emery Nagy
 * @brief New GPS driver for SIM7070G
 * @version 0.1
 * @date 2023-02-14
 *
 */

/* HAL includes */
#include "system.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* stdlib includes */
#include "stdio.h"
#include "string.h"
#include "stdbool.h"
#include "stdlib.h"

/* lib includes */
#include "lib_uart.h"
#include "lib_gps.h"

/* defines */
#define LIB_gps_RX_BUF_SIZE 513 // 512 byte buffer + 1 for
#define LIB_gps_RX_BUF_SIZE_SMALL 64
#define LIB_GPS_RX_BUF_SIZE_GPS_PAYLOAD LIB_gps_RX_BUF_SIZE_SMALL + 95 // guaranteed 94 byte async data payload
#define LIB_GPS_AT_PRINT_OUTPUT 1
#define LIB_GPS_AT_DEFAULT_CMD_TIMEOUT_MS 1000
#define LIB_gps_AT_MODULE_RESET_WAKEUP_RETRIES 20

/* private types */

typedef enum {
    RX_RECEIVING,
    RX_ERROR,
    RX_DONE
} lib_gps_rxstate_E;

/* gps module rx buffer used to collect data from uart callback */
typedef struct {
    /* rx buffer */
    volatile alt_u8* rx_buf;

    /* rx buffer current index */
    alt_u32 idx;

    /* rx buffer size */
    alt_u32 buffersize;

    /* Current RX state */
    lib_gps_rxstate_E rx_state;

} lib_gps_rx_buffer_S;

/* state struct */
typedef struct {

    /* UART settings */
    lib_uart_config_S* config;

    /* RX buffer */
    volatile lib_gps_rx_buffer_S* rxbuf;

    /* mutex for handling multiple requests */
    SemaphoreHandle_t mutex;

} lib_gps_state_S;

/* UART driver */
static void lib_gps_rx_error_callback(void);
static void lib_gps_rx_callback(alt_u8 rxdata);


/* Private data */

/* uart configuration */
static lib_uart_config_S gps_config = {
    .uart_rx_irq = (lib_uart_generic_rx_irq*)lib_gps_rx_callback,
    .uart_rx_error = (lib_uart_rx_error*)lib_gps_rx_error_callback
};

/* state configuration, initially no rxbuffer set up */
volatile static lib_gps_state_S gps_state = {
    .config = &gps_config,
    .rxbuf = NULL,
};

/**
 * @brief UART rx error callback, sets rxbuffer error to notify lib that incoming data chunk is garbage
 *
 */
static void lib_gps_rx_error_callback(void) {
    volatile lib_gps_rx_buffer_S* buffer = gps_state.rxbuf;
    if (buffer == NULL) {
        return;
    }
    /* set rxbuffer error for currently incoming data only if we are expecting a response */
    if (buffer->rx_state == RX_RECEIVING) {
        //buffer->rx_state = RX_ERROR;
    }
}

/**
 * @brief UART rxdata callback, writes new data into rxbuffer if rxbuffer is in a good state,
 *
 * @param rxdata incoming UART data
 */
static void lib_gps_rx_callback(alt_u8 rxdata) {
    volatile lib_gps_rx_buffer_S* buffer = gps_state.rxbuf;
    if (buffer == NULL) {
        return;
    }

    /* set rxbuffer error for currently incoming data only if we are expecting a response */
    if (buffer->rx_state == RX_RECEIVING) {
        buffer->rx_buf[buffer->idx] = rxdata;
        (buffer->idx)++;
        if (buffer->idx >= buffer->buffersize) {
            buffer->rx_state = RX_DONE;
        }
    }
}

/**
 * @brief Send a command sequence to the gps module
 *
 * @param cmd command sequence to sent
 * @param cmd_len size of command sequence
 * @param cmd_tx_timeout timeout of transmission window in ms
 * @param rxbuf receivebuffer for response data
 * @return lib_gps_result_E
 */
static lib_gps_result_E lib_gps_send_cmd(alt_u8* cmd, alt_u32 cmd_len, alt_u32 cmd_tx_timeout) {

    lib_gps_result_E result = GPS_ERROR;
    /* set up rx buffer before sending command */
    if (lib_uart_tx(gps_state.config, cmd, cmd_len, cmd_tx_timeout) == LIB_UART_SUCCESS) {
        result = GPS_SUCCESS;
    }

    return result;
}

/**
 * @brief construct command string from command datatypes
 *
 * @param cmd command datastructure
 * @param cmd_data command data string
 * @param cmd_len command size
 * @return lib_gps_result_E
 */
static lib_gps_result_E lib_gps_construct_cmd_strings(lib_gps_cmd_type_E cmd, alt_u8** cmd_data, alt_u32* cmd_len, alt_u8* preformatted_args) {

    /* first calculate the length of command and response */
    *cmd_len = (cmd.cmd_len + LIB_GPS_CMD_SIZE_OVERHEAD + cmd.args_len);
    if (cmd.formatted_args == true) {
        *cmd_len = *cmd_len + strlen(preformatted_args);
    }
    /* generate AT command string */
    alt_u8* cmd_string = (alt_u8*)pvPortMalloc(*cmd_len);
    memset(cmd_string, 0, *cmd_len);
    strcat(cmd_string, LIB_GPS_CMD_HEADER);
    strcat(cmd_string, cmd.cmd);
    if (cmd.is_query) {
        strcat(cmd_string, LIB_GPS_CMD_QUERY);
    } else if (cmd.formatted_args == true) {
        strcat(cmd_string, LIB_GPS_CMD_ARGS_ASSIGNMENT);
        strcat(cmd_string, preformatted_args);
    }
    else if (cmd.args_len != 0) {
        strcat(cmd_string, LIB_GPS_CMD_ARGS_ASSIGNMENT);
        strcat(cmd_string, cmd.cmd_args);
    }
    strcat(cmd_string, LIB_GPS_CMD_FOOTER);

    *cmd_data = cmd_string;
    return GPS_SUCCESS;
}


/**
 * @brief execute AT command and parse reponse
 *
 * @param cmd command to execute
 * @param preformatted_args already formatted args to send with AT command
 * @param rxbuf optional -> copy rxsize bytes captured from the AT port to existing buffer rxbuf
 * @param rxsize optional -> number of bytes to copy out
 * @param timeout_ms command timeout in ms
 * @return lib_gps_result_E
 */
static lib_gps_result_E lib_gps_execute_cmd(lib_gps_cmd_type_E cmd, alt_u8* preformatted_args, alt_u8* rxbuf, alt_u32 rxsize, alt_u32 timeout_ms) {
    lib_gps_result_E res = GPS_TIMEOUT;

    /* make rx buf */
    alt_u8* rx = (alt_u8*)pvPortMalloc(LIB_gps_RX_BUF_SIZE);
    memset(rx, 0, LIB_gps_RX_BUF_SIZE);
    volatile lib_gps_rx_buffer_S new_rx_buf = {
        .buffersize = LIB_gps_RX_BUF_SIZE,
        .idx = 0,
        .rx_buf = rx,
        .rx_state = RX_RECEIVING
    };

    gps_state.rxbuf = &new_rx_buf;

    /* generate command data */
    alt_u8* cmd_data = NULL;
    alt_u32 cmd_data_len = 0;
    lib_gps_construct_cmd_strings(cmd, &cmd_data, &cmd_data_len, preformatted_args);

    do {

        /* first send the command */
        res = lib_gps_send_cmd(cmd_data, strlen(cmd_data), timeout_ms);
        if (res != GPS_SUCCESS) {
            break;
        }

        alt_u32 start_time = xTaskGetTickCount();
        res = GPS_TIMEOUT;
        alt_u8* search_str = (alt_u8*)pvPortMalloc(cmd.cmd_len + 3);
        memset(search_str, 0, cmd.cmd_len + 3);
        /* wait for command response */
        while (((xTaskGetTickCount() - start_time)) <= pdMS_TO_TICKS(timeout_ms)) {
            /* conditions for AT command success are 1) OK response string, 2) Command response string if applicable,
             * and any response must end in \r\n */
            if (strstr(new_rx_buf.rx_buf, LIB_GPS_CMD_RESP_OK)) {
                /* if the response type is a regular string then we look for the regex pattern <cmd>: */
                if (cmd.resp_type == LIB_GPS_RESP_TYPE_STRING) {
                    sprintf(search_str, "%s: ", cmd.cmd);
                    if ((strstr(new_rx_buf.rx_buf, search_str) != NULL) &&
                        (new_rx_buf.rx_buf[new_rx_buf.idx-1] == '\n') &&
                        (new_rx_buf.rx_buf[new_rx_buf.idx-2] == '\r')) {
                            res = GPS_SUCCESS;
                            break;
                    }
                } else if (cmd.resp_type == LIB_GPS_RESP_TYPE_ASYNC) {
                    if (strstr(new_rx_buf.rx_buf, cmd.response_str) != NULL) {
                        res = GPS_SUCCESS;
                        break;
                    }
                } else {
                    res = GPS_SUCCESS;
                    break;
                }
            } else if (strstr(new_rx_buf.rx_buf, LIB_GPS_CMD_RESP_ERROR) != NULL) {
                res = GPS_ERROR;
                break;
            } else if (new_rx_buf.rx_state == RX_DONE) {
                res = GPS_ERROR;
                break;
            } else if (new_rx_buf.rx_state == RX_ERROR) {
                res = GPS_ERROR;
                break;
            }
        }

        vPortFree(search_str);
    } while (0);

    gps_state.rxbuf = NULL;

#if (LIB_GPS_AT_PRINT_OUTPUT == 1)
    printf("\nReceived GPS %d:\n", new_rx_buf.idx);
    for (int i = 0; i < new_rx_buf.idx; i++) {
        printf("%c", new_rx_buf.rx_buf[i]);
    }
    printf("\n");
#endif

    /* copy to output */
    if (rxbuf != NULL) {
        memcpy(rxbuf, rx, rxsize);
    }

    vPortFree(rx);
    vPortFree(cmd_data);

    return res;
}

/**
 * @brief Get the sim tray status
 *
 * @return lib_gps_result_E
 */
static lib_gps_result_E lib_gps_get_sim_status(void) {
    return lib_gps_execute_cmd(LIB_GPS_SIM_STATUS_CMD, NULL, NULL, 0, LIB_GPS_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/* public API*/

/**
 * @brief initializes the gps driver, should only be called once by any task
 *
 * @return lib_gps_result_E
 */
lib_gps_result_E lib_gps_init(alt_u32 UART_BASE, alt_u32 UART_IRQ) {
    lib_gps_result_E res = GPS_ERROR;

    gps_state.config->uart_base = (volatile unsigned int*)UART_BASE;
    gps_state.config->uart_irq = UART_IRQ;

    if (lib_uart_init(gps_state.config) == LIB_UART_SUCCESS) {
        res = GPS_SUCCESS;
    }

    gps_state.mutex = xSemaphoreCreateMutex();

    return res;
}

/**
 * @brief verify that module is responsive and is powered on, if not attempt to power on and contact module
 *
 * @return lib_gps_result_E
 */
lib_gps_result_E lib_gps_power_state_on(volatile unsigned int* GPIO_BASE) {
    lib_gps_result_E res = lib_gps_get_sim_status();
    if (res != GPS_SUCCESS) {
        /* Taken from section 3.2 of http://www.shenzhen2u.com/doc/Module/SIM7000C/SIM7000%20Hardware%20Design_V1.01.pdf
        * along with https://www.waveshare.com/w/upload/5/52/SIM7070X_Cat-M_NB-IoT_GPRS_HAT_Schematic.pdf ->
        * must toggle power key for at least 2 seconds to power on device */
        volatile unsigned int c_gpio = *((volatile unsigned int *)GPIO_BASE);
        vTaskDelay(pdMS_TO_TICKS(2000));
        *((volatile unsigned int *)GPIO_BASE) = ~c_gpio;
        vTaskDelay(pdMS_TO_TICKS(2000));
        *((volatile unsigned int *)GPIO_BASE) = c_gpio;
        vTaskDelay(pdMS_TO_TICKS(10000)); // 3.5 seconds for AT port available

        /* now attempt to contact module, usually takes a few attempts before it is responsive */
        alt_u32 wakeup_count = 0;
        while (lib_gps_get_sim_status() != GPS_SUCCESS) {
            vTaskDelay(pdMS_TO_TICKS(LIB_GPS_AT_DEFAULT_CMD_TIMEOUT_MS));
            wakeup_count++;
            if (wakeup_count > LIB_gps_AT_MODULE_RESET_WAKEUP_RETRIES) {
                res = GPS_TIMEOUT;
                break;
            }
        }
    }
    return res;
}

/**
 * @brief reset gps module -> AT command port can take upward of 30 seconds to re-initialize
 *
 * @return lib_gps_result_E
 */
lib_gps_result_E lib_gps_reset_module(void) {
    lib_gps_result_E ret = lib_gps_execute_cmd(LIB_GPS_RESET_CMD, NULL, NULL, 0, LIB_GPS_AT_DEFAULT_CMD_TIMEOUT_MS);
    /* delay for 500 ms to allow reset to execute on module */
    vTaskDelay(pdMS_TO_TICKS(LIB_GPS_AT_DEFAULT_CMD_TIMEOUT_MS*4));
    lib_gps_get_sim_status();
    vTaskDelay(pdMS_TO_TICKS(LIB_GPS_AT_DEFAULT_CMD_TIMEOUT_MS));
    alt_u32 wakeup_count = 0;
    while (lib_gps_get_sim_status() != GPS_SUCCESS) {
        vTaskDelay(pdMS_TO_TICKS(LIB_GPS_AT_DEFAULT_CMD_TIMEOUT_MS));
        wakeup_count++;
        if (wakeup_count > LIB_gps_AT_MODULE_RESET_WAKEUP_RETRIES) {
            ret = GPS_TIMEOUT;
            break;
        }
    }

    return ret;
}

/**
 * @brief turn ON GPS power to the module - Note that GPS and gps do not run properly at the same time
 *
 * @return lib_gps_result_E
 */
lib_gps_result_E lib_gps_turn_on_gps(void) {
    return lib_gps_execute_cmd(LIB_GPS_POWER_ON_CMD, NULL, NULL, 0, LIB_GPS_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/**
 * @brief turn OFF GPS power to the module
 *
 * @return lib_gps_result_E
 */
lib_gps_result_E lib_gps_turn_off_gps(void) {
    return lib_gps_execute_cmd(LIB_GPS_POWER_OFF_CMD, NULL, NULL, 0, LIB_GPS_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/**
 * @brief Get latitude and longitude from GPS
 *
 * @param lat 10 digit string containing latitude information
 * @param long 11 digit strng containing longitude information
 * @return lib_gps_result_E
 */
lib_gps_result_E lib_gps_read_gps(alt_u8* lat, alt_u8* longi) {
    /* prepare buffers */
    alt_u8* rxbuf = pvPortMalloc(LIB_GPS_RX_BUF_SIZE_GPS_PAYLOAD);
    memset(rxbuf, 0, LIB_GPS_RX_BUF_SIZE_GPS_PAYLOAD);

    /* execute command - 10s timeout because GPS can take a long time to respond */
    lib_gps_result_E ret = lib_gps_execute_cmd(LIB_GPS_GET_LOCATION_CMD, NULL, rxbuf, LIB_GPS_RX_BUF_SIZE_GPS_PAYLOAD - 1, LIB_GPS_AT_DEFAULT_CMD_TIMEOUT_MS*10);
    if (ret != GPS_SUCCESS) {
        vPortFree(rxbuf);
        return ret;
    }

    /* now parse the actual AT command response directly */
    ret = GPS_ERROR;

    alt_u8* token = strstr(rxbuf, LIB_GPS_GET_LOCATION_CMD.cmd);
    if (token != NULL) {
        token = token + strlen(LIB_GPS_GET_LOCATION_CMD.cmd);
    }
    while (token != NULL) {
        float utc_time;
        float lat_f;
        float long_f;
        if (sscanf(token, LIB_GPS_DATA_RESPONSE_STRING, &utc_time, &lat_f, &long_f) == 3) {
            printf("Lat %f long %f\n", lat_f, long_f);
            sprintf(lat, "%f", lat_f);
            sprintf(longi, "%f", long_f);
            ret = GPS_SUCCESS;
            break;
        }
        token = strstr(token, LIB_GPS_GET_LOCATION_CMD.cmd);
        if (token != NULL) {
            token = token + strlen(LIB_GPS_GET_LOCATION_CMD.cmd);
        }
    }

    vPortFree(rxbuf);
    return ret;
}
