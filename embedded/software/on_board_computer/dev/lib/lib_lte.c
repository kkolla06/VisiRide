/**
 * @file lib_lte.c
 * @author Emery Nagy
 * @brief New LTE driver for SIM7080G
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
#include "lib_lte.h"
#include "lib_lte_cmd.h"

/* defines */
#define LIB_LTE_RX_BUF_SIZE 513 // 512 byte buffer + 1 for
#define LIB_LTE_RX_BUF_SIZE_SMALL 64
#define LIB_LTE_RX_BUF_SIZE_GPS_PAYLOAD LIB_LTE_RX_BUF_SIZE_SMALL + 95 // guaranteed 94 byte async data payload
#define LIB_LTE_AT_PRINT_OUTPUT 0
#define LIB_LTE_AT_PORT_ECHO_RESPONSE 0
#define LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS 1000
#define LIB_LTE_AT_MODULE_RESET_WAKEUP_RETRIES 20

/* private types */

typedef enum {
    RX_RECEIVING,
    RX_ERROR,
    RX_DONE
} lib_lte_rxstate_E;

/* lte module rx buffer used to collect data from uart callback */
typedef struct {
    /* rx buffer */
    volatile alt_u8* rx_buf;

    /* rx buffer current index */
    alt_u32 idx;

    /* rx buffer size */
    alt_u32 buffersize;

    /* Current RX state */
    lib_lte_rxstate_E rx_state;

} lib_lte_rx_buffer_S;

/* state struct */
typedef struct {

    /* UART settings */
    lib_uart_config_S* config;

    /* RX buffer */
    volatile lib_lte_rx_buffer_S* rxbuf;

    /* mutex for handling multiple requests */
    SemaphoreHandle_t mutex;

} lib_lte_state_S;

/* UART driver */
static void lib_lte_rx_error_callback(void);
static void lib_lte_rx_callback(alt_u8 rxdata);


/* Private data */

/* uart configuration */
static lib_uart_config_S lte_config = {
    .uart_rx_irq = (lib_uart_generic_rx_irq*)lib_lte_rx_callback,
    .uart_rx_error = (lib_uart_rx_error*)lib_lte_rx_error_callback
};

/* state configuration, initially no rxbuffer set up */
volatile static lib_lte_state_S lte_state = {
    .config = &lte_config,
    .rxbuf = NULL,
};

/**
 * @brief UART rx error callback, sets rxbuffer error to notify lib that incoming data chunk is garbage
 *
 */
static void lib_lte_rx_error_callback(void) {
    volatile lib_lte_rx_buffer_S* buffer = lte_state.rxbuf;
    if (buffer == NULL) {
        return;
    }
    /* set rxbuffer error for currently incomming data only if we are expecting a response */
    if (buffer->rx_state == RX_RECEIVING) {
        buffer->rx_state = RX_ERROR;
    }
}

/**
 * @brief UART rxdata callback, writes new data into rxbuffer if rxbuffer is in a good state,
 *
 * @param rxdata incoming UART data
 */
static void lib_lte_rx_callback(alt_u8 rxdata) {
    volatile lib_lte_rx_buffer_S* buffer = lte_state.rxbuf;
    if (buffer == NULL) {
        return;
    }

    /* set rxbuffer error for currently incomming data only if we are expecting a response */
    if (buffer->rx_state == RX_RECEIVING) {
        buffer->rx_buf[buffer->idx] = rxdata;
        (buffer->idx)++;
        if (buffer->idx >= buffer->buffersize) {
            buffer->rx_state = RX_DONE;
        }
    }
}

/**
 * @brief Send a command sequence to the lte module
 *
 * @param cmd command sequence to sent
 * @param cmd_len size of command sequence
 * @param cmd_tx_timeout timeout of transmission window in ms
 * @param rxbuf receivebuffer for response data
 * @return lib_lte_result_E
 */
static lib_lte_result_E lib_lte_send_cmd(alt_u8* cmd, alt_u32 cmd_len, alt_u32 cmd_tx_timeout) {

    lib_lte_result_E result = LTE_ERROR;
    /* set up rx buffer before sending command */
    if (lib_uart_tx(lte_state.config, cmd, cmd_len, cmd_tx_timeout) == LIB_UART_SUCCESS) {
        result = LTE_SUCCESS;
    }

    return result;
}

/**
 * @brief construct command string from command datatypes
 *
 * @param cmd command datastructure
 * @param cmd_data command data string
 * @param cmd_len command size
 * @return lib_lte_result_E
 */
static lib_lte_result_E lib_lte_construct_cmd_strings(lib_lte_cmd_type_E cmd, alt_u8** cmd_data, alt_u32* cmd_len, alt_u8* preformatted_args) {

    /* first calculate the length of command and response */
    *cmd_len = (cmd.cmd_len + LIB_LTE_CMD_SIZE_OVERHEAD + cmd.args_len);
    if (cmd.formatted_args == true) {
        *cmd_len = *cmd_len + strlen(preformatted_args);
    }
    /* generate AT command string */
    alt_u8* cmd_string = (alt_u8*)pvPortMalloc(*cmd_len);
    memset(cmd_string, 0, *cmd_len);
    strcat(cmd_string, LIB_LTE_CMD_HEADER);
    strcat(cmd_string, cmd.cmd);
    if (cmd.is_query) {
        strcat(cmd_string, LIB_LTE_CMD_QUERY);
    } else if (cmd.formatted_args == true) {
        strcat(cmd_string, LIB_LTE_CMD_ARGS_ASSIGNMENT);
        strcat(cmd_string, preformatted_args);
    }
    else if (cmd.args_len != 0) {
        strcat(cmd_string, LIB_LTE_CMD_ARGS_ASSIGNMENT);
        strcat(cmd_string, cmd.cmd_args);
    }
    strcat(cmd_string, LIB_LTE_CMD_FOOTER);

    *cmd_data = cmd_string;
    return LTE_SUCCESS;
}


/**
 * @brief execute AT command and parse reponse
 *
 * @param cmd command to execute
 * @param preformatted_args already formatted args to send with AT command
 * @param rxbuf optional -> copy rxsize bytes captured from the AT port to existing buffer rxbuf
 * @param rxsize optional -> number of bytes to copy out
 * @param timeout_ms command timeout in ms
 * @return lib_lte_result_E
 */
static lib_lte_result_E lib_lte_execute_cmd(lib_lte_cmd_type_E cmd, alt_u8* preformatted_args, alt_u8* rxbuf, alt_u32 rxsize, alt_u32 timeout_ms) {
    lib_lte_result_E res = LTE_TIMEOUT;

    /* make rx buf */
    alt_u8 rx[LIB_LTE_RX_BUF_SIZE] = {0};
    volatile lib_lte_rx_buffer_S new_rx_buf = {
        .buffersize = LIB_LTE_RX_BUF_SIZE,
        .idx = 0,
        .rx_buf = rx,
        .rx_state = RX_RECEIVING
    };

    lte_state.rxbuf = &new_rx_buf;

    /* generate command data */
    alt_u8* cmd_data = NULL;
    alt_u32 cmd_data_len = 0;
    lib_lte_construct_cmd_strings(cmd, &cmd_data, &cmd_data_len, preformatted_args);

    do {

        /* first send the command */
        res = lib_lte_send_cmd(cmd_data, cmd_data_len, timeout_ms);
        if (res != LTE_SUCCESS) {
            break;
        }

        alt_u32 start_time = xTaskGetTickCount();
        res = LTE_TIMEOUT;
        /* wait for command response */
        while (((xTaskGetTickCount() - start_time)) <= pdMS_TO_TICKS(timeout_ms)) {
            //vTaskDelay(pdMS_TO_TICKS(5)); //10ms delay, let other tasks do their thing
            /* conditions for AT command success are 1) OK response string, 2) Command response string if applicable,
             * and any response must end in \r\n */
            if (strstr(new_rx_buf.rx_buf, LIB_LTE_CMD_RESP_OK)) {
                /* if the response type is a regular string then we look for the regex pattern <cmd>: */
                if (cmd.resp_type == LIB_LTE_RESP_TYPE_STRING) {
                    alt_u8* search_str[LIB_LTE_RX_BUF_SIZE_SMALL];
                    sprintf(search_str, "%s: ", cmd.cmd);
                    if ((strstr(new_rx_buf.rx_buf, search_str) != NULL) &&
                        (new_rx_buf.rx_buf[new_rx_buf.idx-1] == '\n') &&
                        (new_rx_buf.rx_buf[new_rx_buf.idx-2] == '\r')) {
                            res = LTE_SUCCESS;
                            break;
                    }
                } else if (cmd.resp_type == LIB_LTE_RESP_TYPE_ASYNC) {
                    if (strstr(new_rx_buf.rx_buf, cmd.response_str) != NULL) {
                        res = LTE_SUCCESS;
                        break;
                    }
                } else {
                    res = LTE_SUCCESS;
                    break;
                }
            } else if (strstr(new_rx_buf.rx_buf, LIB_LTE_CMD_RESP_ERROR) != NULL) {
                res = LTE_ERROR;
                break;
            } else if (new_rx_buf.rx_state == RX_DONE) {
                res = LTE_ERROR;
                break;
            } else if (new_rx_buf.rx_state == RX_ERROR) {
                res = LTE_ERROR;
                break;
            }
        }

    } while (0);

    lte_state.rxbuf = NULL;

#if (LIB_LTE_AT_PRINT_OUTPUT == 1)
    printf("\nReceived CELL %d:\n", new_rx_buf.idx);
    printf("CMD: %s, length: %d\n", cmd_data, cmd_data_len);
    for (int i = 0; i < new_rx_buf.idx; i++) {
        printf("%c", new_rx_buf.rx_buf[i]);
    }
    printf("\n");
#endif

    /* copy to output */
    if (rxbuf != NULL) {
        memcpy(rxbuf, rx, rxsize);
    }

    vPortFree(cmd_data);

    return res;
}

/* public API*/

/**
 * @brief initializes the lte driver, should only be called once by any task
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_init(alt_u32 UART_BASE, alt_u32 UART_IRQ) {
    lib_lte_result_E res = LTE_ERROR;

    lte_state.config->uart_base = (volatile unsigned int*)UART_BASE;
    lte_state.config->uart_irq = UART_IRQ;

    if (lib_uart_init(lte_state.config) == LIB_UART_SUCCESS) {
        res = LTE_SUCCESS;
    }

    lte_state.mutex = xSemaphoreCreateMutex();

    return res;
}

/**
 * @brief verify that module is responsive and is powered on, if not attempt to power on and contact module
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_power_state_on(volatile unsigned int* GPIO_BASE) {
    lib_lte_result_E res = lib_lte_get_sim_status();
    if (res != LTE_SUCCESS) {
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
        while (lib_lte_get_sim_status() != LTE_SUCCESS) {
            vTaskDelay(pdMS_TO_TICKS(LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS));
            wakeup_count++;
            if (wakeup_count > LIB_LTE_AT_MODULE_RESET_WAKEUP_RETRIES) {
                res = LTE_TIMEOUT;
                break;
            }
        }
    }
    return res;
}

/**
 * @brief Get the sim tray status
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_get_sim_status(void) {
    return lib_lte_execute_cmd(LIB_LTE_SIM_STATUS_CMD, NULL, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/**
 * @brief reset LTE module -> AT command port can take upward of 30 seconds to re-initialize
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_reset_module(void) {
    lib_lte_result_E ret = lib_lte_execute_cmd(LIB_LTE_RESET_CMD, NULL, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
    /* delay for 500 ms to allow reset to execute on module */
    vTaskDelay(pdMS_TO_TICKS(LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS*4));
    lib_lte_get_sim_status();
    vTaskDelay(pdMS_TO_TICKS(LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS));
    alt_u32 wakeup_count = 0;
    while (lib_lte_get_sim_status() != LTE_SUCCESS) {
        vTaskDelay(pdMS_TO_TICKS(LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS));
        wakeup_count++;
        if (wakeup_count > LIB_LTE_AT_MODULE_RESET_WAKEUP_RETRIES) {
            ret = LTE_TIMEOUT;
            break;
        }
    }

    /* supress basic command response checking to speed up large data transfers */
#if (LIB_LTE_AT_PORT_ECHO_RESPONSE == 0)
    lib_lte_execute_cmd(LIB_LTE_ECHO_OFF_CMD, NULL, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
#endif
    return ret;
}

/**
 * @brief get the module rssi
 *
 * @param rssi output rssi
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_get_signal_strength(alt_u8* rssi) {
    /* prepare buffers */
    alt_u8* rxbuf = pvPortMalloc(LIB_LTE_RX_BUF_SIZE_SMALL);
    memset(rxbuf, 0, LIB_LTE_RX_BUF_SIZE_SMALL);
    alt_u8 ber;

    /* execute command, check that response can be read from reported RSSI */
    lib_lte_result_E ret = lib_lte_execute_cmd(LIB_LTE_RSSI_CMD, NULL, rxbuf, LIB_LTE_RX_BUF_SIZE_SMALL - 1, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
    if (ret != LTE_SUCCESS) {
        vPortFree(rxbuf);
        return ret;
    }

    /* now parse the actual AT command response directly */
    ret = LTE_ERROR;
    alt_u8* token = strtok(rxbuf, LIB_LTE_RSSI_CMD.cmd);
    while (token != NULL) {
        if (sscanf(token, LIB_LTE_CMD_RSSI_RESPONSE_STRING, rssi, &ber) == 2) {
            ret = LTE_SUCCESS;
            break;
        }
        token = strtok(NULL, LIB_LTE_RSSI_CMD.cmd);
    }

    vPortFree(rxbuf);
    return ret;
}

/**
 * @brief turn off module radio
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_turn_off_radio(void) {
    return lib_lte_execute_cmd(LIB_LTE_RADIO_OFF_CMD, NULL, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/**
 * @brief turn on module radio
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_turn_on_radio(void) {
    return lib_lte_execute_cmd(LIB_LTE_RADIO_ON_CMD, NULL, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/**
 * @brief Set module APN to m2m.telus.iot
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_set_apn(void) {
    return lib_lte_execute_cmd(LIB_LTE_SET_APN_CMD, NULL, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/**
 * @brief attach module to network
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_attach_to_network(void) {
    return lib_lte_execute_cmd(LIB_LTE_NETWORK_ATTACH_CMD, NULL, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS*5);
}

/**
 * @brief detach module form network
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_detach_from_network(void) {
    return lib_lte_execute_cmd(LIB_LTE_NETWORK_DETACH_CMD, NULL, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/**
 * @brief setup http connection with server
 *
 * @param addr URL of server to connect to
 * @param hdr_size size of header
 * @param body_size size of body
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_setup_http_connection(alt_u8* addr, alt_u16 hdr_size, alt_u16 body_size) {
    lib_lte_result_E res = LTE_ERROR;

    do {
        /* Assemble web URL in command string */
        alt_u8* url_cmd_string = (alt_u8*)pvPortMalloc(strlen(addr)+10); // "URL","<URL>"
        url_cmd_string[0] = '\0';
        sprintf(url_cmd_string, "\"URL\",\"%s\"", addr);

        if (lib_lte_execute_cmd(LIB_LTE_HTTP_CONFIGURE_CMD, url_cmd_string, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS) != LTE_SUCCESS) {
            vPortFree(url_cmd_string);
            break;
        }
        vPortFree(url_cmd_string);

        /* Assemble body and header command strings */
        alt_u8* body_cmd_string = (alt_u8*)pvPortMalloc(24);
        body_cmd_string[0] = '\0';
        sprintf(body_cmd_string, "\"BODYLEN\",%u", body_size);
        if (lib_lte_execute_cmd(LIB_LTE_HTTP_CONFIGURE_CMD, body_cmd_string, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS) != LTE_SUCCESS) {
            vPortFree(body_cmd_string);
            break;
        }
        vPortFree(body_cmd_string);

        alt_u8* hdr_cmd_string = (alt_u8*)pvPortMalloc(24);
        hdr_cmd_string[0] = '\0';
        sprintf(hdr_cmd_string, "\"HEADERLEN\",%u", hdr_size);
        if (lib_lte_execute_cmd(LIB_LTE_HTTP_CONFIGURE_CMD, hdr_cmd_string, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS) != LTE_SUCCESS) {
            vPortFree(hdr_cmd_string);
            break;
        }
        vPortFree(hdr_cmd_string);

        res = LTE_SUCCESS;
    } while (0);

    return res;
}

/**
 * @brief start http connection with existing http connection parameters
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_start_http_connection(void) {
    return lib_lte_execute_cmd(LIB_LTE_HTTP_CONN_CMD, NULL, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS*10);
}

/**
 * @brief end existing http connection
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_end_http_connection(void) {
    return lib_lte_execute_cmd(LIB_LTE_HTTP_DISCONNECT_CMD, NULL, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/**
 * @brief check if there is an active http connection already, return LTE_SUCCESS if connected
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_check_http_connection(void) {
    return lib_lte_execute_cmd(LIB_LTE_HTTP_CHECK_CONN_CMD, NULL, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/**
 * @brief clear the existing http header
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_clear_http_header(void) {
    return lib_lte_execute_cmd(LIB_LTE_HTTP_CLEAR_HEADER_CMD, NULL, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/**
 * @brief write data to the existing http header
 *
 * @param databuffer string to write (should be formatted as a regular C string)
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_write_to_http_header(alt_u8* databuffer) {
    return lib_lte_execute_cmd(LIB_LTE_HTTP_WRITE_TO_HEADER_CMD, databuffer, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/**
 * @brief clear the existing http body
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_clear_http_body(void) {
    return lib_lte_execute_cmd(LIB_LTE_HTTP_CLEAR_BODY_CMD, NULL, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/**
 * @brief write data to the existing http body
 *
 * @param databuffer string to write (should be formatted as a regular C string)
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_write_to_http_body(alt_u8* databuffer) {
    return lib_lte_execute_cmd(LIB_LTE_HTTP_WRITE_TO_BODY_CMD, databuffer, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/**
 * @brief post data to server via http (uses already set up connection)
 *
 * @param response_code standard http response code
 * @param resp_len data available to read as server response
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_post_http_request(alt_u8* endpoint, alt_u16* response_code, alt_u16* resp_len) {
    /* prepare buffers */
    alt_u8* rxbuf = pvPortMalloc(LIB_LTE_RX_BUF_SIZE_SMALL);
    memset(rxbuf, 0, LIB_LTE_RX_BUF_SIZE_SMALL);

    /* prepare command args */
    alt_u8* txbuf = pvPortMalloc(strlen(endpoint) + 20);
    memset(txbuf, 0, strlen(endpoint) + 10);
    snprintf(txbuf, "\"%s\"", endpoint);
    txbuf = strcat(txbuf, ",3");

    /* execute command */
    lib_lte_result_E ret = lib_lte_execute_cmd(LIB_LTE_HTTP_POST_CMD, txbuf, rxbuf, LIB_LTE_RX_BUF_SIZE_SMALL - 1, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS*2);
    if (ret != LTE_SUCCESS) {
        vPortFree(rxbuf);
        vPortFree(txbuf);
        return ret;
    }

    /* now parse the actual AT command response directly */
    ret = LTE_ERROR;

    alt_u8* token = strstr(rxbuf, LIB_LTE_HTTP_POST_CMD.cmd);
    if (token != NULL) {
        token = token + strlen(LIB_LTE_HTTP_POST_CMD.cmd);
    }
    while (token != NULL) {
        /* still not sure why scanning this directly doesn't work... */
        char rc[4];
        char rl[4];
        if (sscanf(token, LIB_LTE_CMD_HTTP_POST_RESPONSE_STRING, rc, rl) == 2) {
            *response_code = atoi(rc);
            *resp_len = atoi(rl);
            ret = LTE_SUCCESS;
            break;
        }
        token = strstr(token, LIB_LTE_HTTP_POST_CMD.cmd);
        if (token != NULL) {
            token = token + strlen(LIB_LTE_HTTP_POST_CMD.cmd);
        }
    }

    vPortFree(rxbuf);
    vPortFree(txbuf);
    return ret;
}

/**
 * @brief Get HTTP response data
 *
 * @param length number of response bytes to read
 * @param start_addr starting byte to read
 * @param databuffer databuffer to read into
 * @param data_size number of bytes to read into databuffer
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_get_http_response_data(alt_u16 length, alt_u16 start_addr, alt_u8* databuffer, alt_u16 data_size) {
    /* Assemble web URL in command string */
    alt_u8* cmd_string = (alt_u8*)pvPortMalloc(40); // 0,length
    cmd_string[0] = '\0';
    sprintf(cmd_string, "%u,%u", start_addr, length);
    lib_lte_result_E ret = lib_lte_execute_cmd(LIB_LTE_HTTP_READ_RESP_CMD, cmd_string, databuffer, data_size, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
    vPortFree(cmd_string);
    return ret;
}

/**
 * @brief turn ON GPS power to the module - Note that GPS and LTE do not run properly at the same time
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_turn_on_gps(void) {
    return lib_lte_execute_cmd(LIT_LTE_GPS_POWER_ON_CMD, NULL, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/**
 * @brief turn OFF GPS power to the module
 *
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_turn_off_gps(void) {
    return lib_lte_execute_cmd(LIT_LTE_GPS_POWER_OFF_CMD, NULL, NULL, 0, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS);
}

/**
 * @brief Get latitude and longitude from GPS
 *
 * @param lat 10 digit string containing latitude information
 * @param long 11 digit strng containing longitude information
 * @return lib_lte_result_E
 */
lib_lte_result_E lib_lte_read_gps(alt_u8* lat, alt_u8* longi) {
    /* prepare buffers */
    alt_u8* rxbuf = pvPortMalloc(LIB_LTE_RX_BUF_SIZE_GPS_PAYLOAD);
    memset(rxbuf, 0, LIB_LTE_RX_BUF_SIZE_GPS_PAYLOAD);

    /* execute command - 10s timeout because GPS can take a long time to respond */
    lib_lte_result_E ret = lib_lte_execute_cmd(LIT_LTE_GPS_GET_LOCATION_CMD, NULL, rxbuf, LIB_LTE_RX_BUF_SIZE_GPS_PAYLOAD - 1, LIB_LTE_AT_DEFAULT_CMD_TIMEOUT_MS*10);
    if (ret != LTE_SUCCESS) {
        vPortFree(rxbuf);
        return ret;
    }

    /* now parse the actual AT command response directly */
    ret = LTE_ERROR;

    alt_u8* token = strstr(rxbuf, LIT_LTE_GPS_GET_LOCATION_CMD.cmd);
    if (token != NULL) {
        token = token + strlen(LIT_LTE_GPS_GET_LOCATION_CMD.cmd);
    }
    while (token != NULL) {
        alt_u8* utc_time[19];
        if (sscanf(token, LIB_LTE_GPS_DATA_RESPONSE_STRING, utc_time, lat, longi) == 3) {
            ret = LTE_SUCCESS;
            break;
        }
        token = strstr(token, LIT_LTE_GPS_GET_LOCATION_CMD.cmd);
        if (token != NULL) {
            token = token + strlen(LIT_LTE_GPS_GET_LOCATION_CMD.cmd);
        }
    }

    vPortFree(rxbuf);
    return ret;
}
