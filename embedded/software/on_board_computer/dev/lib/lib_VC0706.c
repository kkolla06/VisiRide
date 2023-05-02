/**
 * @file lib_VC0706.c
 * @author Emery Nagy
 * @brief VC0706 camera command execution task, allows app_camera to execute specific commands from camera with set
*          timeout
 * @version 0.1
 * @date 2023-02-12
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

/* Lib includes */
#include "lib_uart.h"
#include "lib_VC0706.h"

/* defines */
#define lib_VC0706_MUTEX_WAIT_TIME_TICKS pdMS_TO_TICKS(25)

#define lib_VC0706_REQUEST_HEADER 0x56
#define lib_VC0706_RESPONSE_HEADER 0x76
#define lib_VC0706_RESPONSE_HEADER_B3 0x00
#define lib_VC0706_SERIAL_NUM 0x00

#define lib_VC0706_RESET_CMD 0x26

#define lib_VC0706_CAMERA_FBUF_CTRL_CMD 0x36
#define lib_VC0706_CAMERA_FBUF_CTRL_CMD_2 0x01
#define lib_VC0706_CAMERA_FBUF_CTR_STOP 0x00
#define lib_VC0706_CAMERA_FBUF_CTR_RESUME 0x03

#define lib_VC0706_CAMERA_FBUF_LEN_CMD 0x34
#define lib_VC0706_CAMERA_FBUF_LEN_CMD_2 0x01
#define lib_VC0706_CAMERA_FBUF_LEN_FBUF_TYPE 0x00

#define lib_VC0706_CAMERA_READ_FBUF_CMD 0x32
#define lib_VC0706_CAMERA_READ_FBUF_CMD_2 0x0C
#define lib_VC0706_CAMERA_READ_FBUF_CMD_3 0x00
#define lib_VC0706_CAMERA_READ_FBUF_CMD_4 0x0A
#define lib_VC0706_CAMERA_DELAY_B1 0x00
#define lib_VC0706_CAMERA_DELAY_B2 0x0A

/* Private data definitions */

typedef enum {
    RX_RECEIVING,
    RX_ERROR,
    RX_DONE
} lib_VC0706_rxstate_E;

/* camera rx buffer used to collect data from uart callback */
typedef struct {
    /* rx buffer */
    volatile alt_u8* rx_buf;

    /* rx buffer current index */
    alt_u32 idx;

    /* rx buffer target size */
    alt_u32 targ_size;

    /* Current RX state */
    lib_VC0706_rxstate_E rx_state;

} lib_VC0706_rx_buffer_S;

/* state struct */
typedef struct {

    /* UART settings */
    lib_uart_config_S* config;

    /* RX buffer */
    volatile lib_VC0706_rx_buffer_S* rxbuf;

    /* mutex for handling multiple requests */
    SemaphoreHandle_t mutex;

    /* flag for if we are expecting a response from UART */
    bool bus_busy;

} lib_VC0706_state_S;

/* private function definitions */

/* UART driver */
static void lib_VC0706_rx_error_callback(void);
static void lib_VC0706_rx_callback(alt_u8 rxdata);

/* Private data */

/* uart configuration */
static lib_uart_config_S VC0706_config = {
    .uart_rx_irq = (lib_uart_generic_rx_irq*)lib_VC0706_rx_callback,
    .uart_rx_error = (lib_uart_rx_error*)lib_VC0706_rx_error_callback
};

/* state configuration, initially no rxbuffer set up */
static lib_VC0706_state_S VC0706_state = {
    .config = &VC0706_config,
    .rxbuf = NULL,
    .bus_busy = false
};


/* Private functions */

/**
 * @brief UART rxdata callback, writes new data into rxbuffer if rxbuffer is in a good state,
 *
 * @param rxdata incomming UART data
 */
static void lib_VC0706_rx_callback(alt_u8 rxdata) {
    volatile lib_VC0706_rx_buffer_S* buffer = VC0706_state.rxbuf;
    if (VC0706_state.bus_busy == false) {
        return;
    }

    /* set rxbuffer error for currently incomming data only if we are expecting a response */
    if (buffer->rx_state == RX_RECEIVING) {
        buffer->rx_buf[buffer->idx] = rxdata;
        (buffer->idx)++;
        if (buffer->idx >= buffer->targ_size) {
            buffer->rx_state = RX_DONE;
        }
    }
}

/**
 * @brief UART rx error callback, sets rxbuffer error to notify lib that incomming data chunk is garbage
 *
 */
static void lib_VC0706_rx_error_callback(void) {
    volatile lib_VC0706_rx_buffer_S* buffer = VC0706_state.rxbuf;
    if (VC0706_state.bus_busy == false) {
        return;
    }
    /* set rxbuffer error for currently incomming data only if we are expecting a response */
    if (buffer->rx_state == RX_RECEIVING) {
        //printf("RX ERROR for expected data");
        buffer->rx_state = RX_ERROR;
    } else {
        //printf("RX ERROR for unexpected data");
    }
}

/**
 * @brief Send a command sequence to the camera
 *
 * @param cmd command sequence to sent
 * @param cmd_len size of command sequence
 * @param cmd_tx_timeout timeout of transmission window in ms
 * @param rxbuf receivebuffer for response data
 * @return lib_VC0706_result_E
 */
static lib_VC0706_result_E lib_VC0706_send_cmd(alt_u8* cmd, alt_u32 cmd_len, alt_u32 cmd_tx_timeout,
                                                                                        volatile lib_VC0706_rx_buffer_S* rxbuf) {

    lib_VC0706_result_E result = VC0706_ERROR;
    VC0706_state.rxbuf = rxbuf;
    VC0706_state.bus_busy = true;
    /* set up rx buffer before sending command */
    if (lib_uart_tx(VC0706_state.config, cmd, cmd_len, cmd_tx_timeout) == LIB_UART_SUCCESS) {
        result = VC0706_SUCCESS;
    }

    return result;
}

/**
 * @brief Verify command response data has proper header
 *
 * @param rxbuf received response data
 * @param cmd command executed (hex)
 * @return lib_VC0706_result_E
 */
static lib_VC0706_result_E lib_VC0706_verify_response(lib_VC0706_rx_buffer_S* rxbuf, alt_u8 cmd) {
    lib_VC0706_result_E result = VC0706_ERROR;
    alt_u8* rxdata = rxbuf->rx_buf;

    /* first check that we received all the data */
    if (rxbuf->idx == rxbuf->targ_size) {
        if ((rxdata[0] == lib_VC0706_RESPONSE_HEADER) &&
            (rxdata[1] == lib_VC0706_SERIAL_NUM) &&
            (rxdata[2] == cmd) &&
            (rxdata[3] == lib_VC0706_RESPONSE_HEADER_B3)) {
                result = VC0706_SUCCESS;
            }
    }

    return result;
}

/**
 * @brief send data to camera and expect response
 *
 * @param txbuf transmission data
 * @param tx_size size of transmission data buffer in bytes
 * @param rxbuf received data buffer
 * @param rxsize size of received data buffer in bytes
 * @param timeout_ms command timeout
 * @return lib_VC0706_result_E
 */
static lib_VC0706_result_E lib_VC0706_execute_cmd(alt_u8* txbuf, alt_u32 tx_size, volatile alt_u8* rxbuf, alt_u32 rxsize,
                                                                                                alt_u32 timeout_ms) {
    lib_VC0706_result_E res = VC0706_ERROR;

    /* construct rx packet */
    volatile lib_VC0706_rx_buffer_S new_rx_buf = {
        .rx_buf = rxbuf,
        .idx = 0,
        .targ_size = rxsize,
        .rx_state = RX_RECEIVING
    };
    do {
        /* first send the command */
        res = lib_VC0706_send_cmd(txbuf, tx_size, timeout_ms/3, &new_rx_buf);
        alt_u32 start_time = xTaskGetTickCount();
        if (res != VC0706_SUCCESS) {
            break;
        }
        /* wait for command response */
        while (((xTaskGetTickCount() - start_time)) <= pdMS_TO_TICKS(timeout_ms)) {
            if (new_rx_buf.rx_state == RX_DONE) {
                break;
            }
        }
        VC0706_state.bus_busy = false;

        /* verify we did not hit command response timeout */
        if ((((xTaskGetTickCount() - start_time)) > pdMS_TO_TICKS(timeout_ms)) || (new_rx_buf.rx_state != RX_DONE)) {
            res = VC0706_TIMEOUT;
            break;
        }

        res = lib_VC0706_verify_response(&new_rx_buf, txbuf[2]);
    } while (0);

    return res;
}

/* Public API */

/**
 * @brief initializes the VC0706 driver, should only be called once by any task
 *
 * @return lib_VC0706_result_E
 */
lib_VC0706_result_E lib_VC0706_init(alt_u32 UART_BASE, alt_u32 UART_IRQ) {
    lib_VC0706_result_E res = VC0706_ERROR;

    VC0706_state.config->uart_base = (volatile unsigned int*)UART_BASE;
    VC0706_state.config->uart_irq = UART_IRQ;

    if (lib_uart_init(VC0706_state.config) == LIB_UART_SUCCESS) {
        res = VC0706_SUCCESS;
    }
    VC0706_state.mutex = xSemaphoreCreateMutex();
    vTaskDelay(pdMS_TO_TICKS(100));


    return res;
}

/**
 * @brief reset the camera. IMPORTANT! User should delay for at least 1 second before calling any other camera commands
 *        as garbage data is usually returned.
 *
 * @param timeout_ms number of ms before command timeout
 * @return lib_VC0706_result_E
 */
lib_VC0706_result_E lib_VC0706_cmd_reset_camera(alt_u32 timeout_ms) {

    lib_VC0706_result_E res = VC0706_ERROR;

    /* grab mutex before running command */
    if (xSemaphoreTake(VC0706_state.mutex, lib_VC0706_MUTEX_WAIT_TIME_TICKS) == pdTRUE) {
        alt_u8 tx[] = {lib_VC0706_REQUEST_HEADER, lib_VC0706_SERIAL_NUM, lib_VC0706_RESET_CMD, 0x00};
        volatile alt_u8 rx[5] = {0}; // 0x76+Serial number+0x26+0x00+0x00
        res = lib_VC0706_execute_cmd(tx, sizeof(tx), rx, sizeof(rx), timeout_ms);
        xSemaphoreGive(VC0706_state.mutex);
    }
    return res;
}

/**
 * @brief Set camera baud rate to 115200 baud (max baud)
 *
 * @param timeout_ms number of ms before command timeout
 * @return lib_VC0706_result_E
 */
lib_VC0706_result_E lib_VC0706_increase_baud(alt_u32 timeout_ms) {
    lib_VC0706_result_E res = VC0706_ERROR;

    /* grab mutex before running command */
    if (xSemaphoreTake(VC0706_state.mutex, lib_VC0706_MUTEX_WAIT_TIME_TICKS) == pdTRUE) {
        alt_u8 tx[] = {lib_VC0706_REQUEST_HEADER, lib_VC0706_SERIAL_NUM, 0x24, 0x03, 0x01, 0x0d, 0xa6};
        volatile alt_u8 rx[5] = {0}; // 0x76+Serial number+0x24+0x00+0x00
        res = lib_VC0706_execute_cmd(tx, sizeof(tx), rx, sizeof(rx), timeout_ms);
        /* set system uart to 115200 */
        lib_uart_set_baud(VC0706_state.config->uart_base, 115200);
        xSemaphoreGive(VC0706_state.mutex);
    }
    return res;
}

/**
 * @brief stop camera frame buffer on current frame
 *
 * @param timeout_ms number of ms before command timeout
 * @return lib_VC0706_result_E
 */
lib_VC0706_result_E lib_VC0706_cmd_stop_frame(alt_u32 timeout_ms) {
    lib_VC0706_result_E res = VC0706_ERROR;

    /* grab mutex before running command */
    if (xSemaphoreTake(VC0706_state.mutex, lib_VC0706_MUTEX_WAIT_TIME_TICKS) == pdTRUE) {
        alt_u8 tx[] = {lib_VC0706_REQUEST_HEADER, lib_VC0706_SERIAL_NUM, lib_VC0706_CAMERA_FBUF_CTRL_CMD,
                        lib_VC0706_CAMERA_FBUF_CTRL_CMD_2, lib_VC0706_CAMERA_FBUF_CTR_STOP};
        volatile alt_u8 rx[5] = {0}; // 0x76+serial number+0x36+0x00+0x00
        res = lib_VC0706_execute_cmd(tx, sizeof(tx), rx, sizeof(rx), timeout_ms);
        xSemaphoreGive(VC0706_state.mutex);
    }
    return res;
}

/**
 * @brief allow camera to resume updating its frame buffer
 *
 * @param timeout_ms number of ms before command timeout
 * @return lib_VC0706_result_E
 */
lib_VC0706_result_E lib_VC0706_cmd_start_frame(alt_u32 timeout_ms) {
    lib_VC0706_result_E res = VC0706_ERROR;

    /* grab mutex before running command */
    if (xSemaphoreTake(VC0706_state.mutex, lib_VC0706_MUTEX_WAIT_TIME_TICKS) == pdTRUE) {
        alt_u8 tx[] = {lib_VC0706_REQUEST_HEADER, lib_VC0706_SERIAL_NUM, lib_VC0706_CAMERA_FBUF_CTRL_CMD,
                        lib_VC0706_CAMERA_FBUF_CTRL_CMD_2, lib_VC0706_CAMERA_FBUF_CTR_RESUME};
        volatile alt_u8 rx[5] = {0}; // 0x76+serial number+0x36+0x00+0x00
        res = lib_VC0706_execute_cmd(tx, sizeof(tx), rx, sizeof(rx), timeout_ms);
        xSemaphoreGive(VC0706_state.mutex);
    }
    return res;
}


lib_VC0706_result_E lib_VC0706_cmd_get_fbuf_len(alt_u32* data, alt_u32 timeout_ms) {
    lib_VC0706_result_E res = VC0706_ERROR;

    /* grab mutex before running command */
    if (xSemaphoreTake(VC0706_state.mutex, lib_VC0706_MUTEX_WAIT_TIME_TICKS) == pdTRUE) {
        alt_u8 tx[] = {lib_VC0706_REQUEST_HEADER, lib_VC0706_SERIAL_NUM, lib_VC0706_CAMERA_FBUF_LEN_CMD,
                                                lib_VC0706_CAMERA_FBUF_LEN_CMD_2, lib_VC0706_CAMERA_FBUF_LEN_FBUF_TYPE};
        volatile alt_u8 rx[9] = {0}; // 0x76+serial number+0x34+0x00+0x04+FBUF data-lengths(4 bytes)
        res = lib_VC0706_execute_cmd(tx, sizeof(tx), rx, sizeof(rx), timeout_ms);
        /* if operation success, reconstruct fbuf length */
        if (res == VC0706_SUCCESS) {
            alt_u32 len = (rx[5] << 24) | (rx[6] << 16) | (rx[7] << 8) | rx[8];
            *data = len;
        }
        xSemaphoreGive(VC0706_state.mutex);
    }
    return res;
}

/**
 * @brief Retrieve a single chunk of camera image data
 *
 * @param dataptr output camera data
 * @param timeout_ms operation timeout in ms
 * @param image_start_addr starting address of data to retrieve from camera
 * @param data_len number of bytes to retrieve from camera
 * @return lib_VC0706_result_E
 */
lib_VC0706_result_E lib_VC0706_cmd_get_fbuf_data(alt_u8* dataptr, alt_u32 timeout_ms, alt_u32 image_start_addr,
                                                                                                    alt_u32 data_len) {

    lib_VC0706_result_E res = VC0706_ERROR;

    /* grab mutex before running command */
    if (xSemaphoreTake(VC0706_state.mutex, lib_VC0706_MUTEX_WAIT_TIME_TICKS) == pdTRUE) {
        /* partition starting address */
        alt_u8 addr_1 = (alt_u8)(image_start_addr >> 24);
        alt_u8 addr_2 = (alt_u8)(image_start_addr >> 16);
        alt_u8 addr_3 = (alt_u8)(image_start_addr >> 8);
        alt_u8 addr_4 = (alt_u8)image_start_addr;
        /* partition length */
        alt_u8 len_1 = (alt_u8)(data_len >> 24);
        alt_u8 len_2 = (alt_u8)(data_len >> 16);
        alt_u8 len_3 = (alt_u8)(data_len >> 8);
        alt_u8 len_4 = (alt_u8)data_len;
        alt_u8 tx[] = {lib_VC0706_REQUEST_HEADER, lib_VC0706_SERIAL_NUM, lib_VC0706_CAMERA_READ_FBUF_CMD,
        lib_VC0706_CAMERA_READ_FBUF_CMD_2, lib_VC0706_CAMERA_READ_FBUF_CMD_3, lib_VC0706_CAMERA_READ_FBUF_CMD_4,
        addr_1, addr_2, addr_3, addr_4, len_1, len_2, len_3, len_4, lib_VC0706_CAMERA_DELAY_B1,
        lib_VC0706_CAMERA_DELAY_B2};
        volatile alt_u8* rx = (alt_u8*)pvPortMalloc(data_len+10); //  0x76+serial+0x32+0x00+0x00 + data + 0x76+serial+0x32+0x00+0x00
        res = lib_VC0706_execute_cmd(tx, sizeof(tx), rx, data_len+10, timeout_ms);
        memcpy(dataptr, rx+5, data_len); // copy to output
        vPortFree(rx);
        xSemaphoreGive(VC0706_state.mutex);
    }
    return res;
}
