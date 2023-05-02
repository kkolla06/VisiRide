/**
 * @file app_demo.c
 * @author Emery Nagy
 * @brief Demo app for Module 3 demo
 * @version 0.1
 * @date 2023-02-13
 *
 */

/* HAL includes */
#include "system.h"
#include "sys/alt_stdio.h"

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"

/* app includes */
#include "app_demo.h"
#include "app_camera.h"

/* lib includes */
#include "lib_base64.h"
#include "lib_lte.h"
#include "lib_gps.h"

/* stdlib includes */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

/* private defines */
#define APP_DEMO_HTTP_OK 200
#define APP_DEMO_MAX_HTTP_PAYLOAD 2048
#define APP_DEMO_DEFAULT_ALLOWABLE_RETRIES 5
#define APP_DEMO_GPS_PERIOD_S 10
#define APP_DEMO_WAIT_TIME_AFTER_IMAGE_SEND_S 4
#define APP_DEMO_RUN_PERIOD_MS 50
#define APP_DEMO_SERVER_CONTACT_SKIPS (APP_DEMO_GPS_PERIOD_S *1000)/APP_DEMO_RUN_PERIOD_MS
#define APP_DEMO_SERVER_WAIT_TIME_AFTER_IMAGE_SKIPS APP_DEMO_SERVER_CONTACT_SKIPS - (APP_DEMO_WAIT_TIME_AFTER_IMAGE_SEND_S *1000)/APP_DEMO_RUN_PERIOD_MS
#define APP_DEMO_DEVICE_UUID 0
#define APP_DEMO_DEBUG_MSG 0
#define APP_DEMO_UNLOCKED_ERROR_RETRIES 10

/* private types */
typedef enum {
    APP_DEMO_SERVER_REPSONSE_LOCK = 0, /* server requests device lock */
    APP_DEMO_SERVER_RESPONSE_UNLOCK, /* server requests device unlock*/
    APP_DEMO_SERVER_RESPONSE_TAKE_PHOTO, /* server requests to take a photo */
    APP_DEMO_SERVER_RESPONSE_ERROR /* error with server response */
} app_demo_server_resp_E;

/* private data - needed to contact server */
static app_demo_state_E current_state = APP_DEMO_STATE_IDLE_LOCKED;
static alt_u8* data_transfer_ptr = NULL; // save time uploading photo by pre-allocating static databuffer
static alt_u8* data_transfer_outptr = NULL; // save time uploading photo by pre-allocating static databuffer
const char APP_DEMO_IMAGE_SIZE[] = {"\"size\",%d"};
const char APP_DEMO_UUID[] = {"\"scooterId\",%d"};
const char APP_DEMO_IMAGE_DATA[] = {"\"data_%d\",%s"};
const char APP_DEMO_LAT_DATA[] = {"\"lat\",%s"};
const char APP_DEMO_LONG_DATA[] = {"\"lon\",%s"};
const char APP_DEMO_GPS_VALID[] = {"\"valid\",\"true\""};
const char APP_DEMO_GPS_NOT_VALID[] = {"\"valid\",\"false\""};

const char APP_DEMO_SERVER_RESP_LOCK[] = {"lock"};
const char APP_DEMO_SERVER_RESP_UNLOCK[] = {"unlock"};
const char APP_DEMO_SERVER_RESP_TAKE_PHOTO[] = {"photo"};

const char APP_DEMO_SERVER_URL[] = {"http://20.238.80.71:8081"};
const char APP_DEMO_IMAGE_ENDPOINT[] = {"/checkFace"};
const char APP_DEMO_GPS_ENDPOINT[] = {"/gps"};


/**
 * @brief attach module to network
 *
 * @param num_tries number of attempts to attach in event of failure
 * @return lib_lte_result_E
 */
static lib_lte_result_E app_demo_attach_to_network(alt_u16 num_tries) {
    lib_lte_result_E ret = LTE_ERROR;
    alt_u16 tries = 0;

    while ((ret != LTE_SUCCESS) && (tries <= num_tries)) {
        if (tries > num_tries) {
            break;
        }
        do {
            /* Check signal strength of network before connecting */
            alt_u8 rssi = LIB_LTE_RSSI_INVALID;
            if (lib_lte_get_signal_strength(&rssi) != LTE_SUCCESS) {
                tries += 1;
                break;
            }
            if (rssi == LIB_LTE_RSSI_INVALID) {
                tries += 1;
                break;
            }

            /* set apn */
            if (lib_lte_set_apn() != LTE_SUCCESS) {
                tries += 1;
                break;
            }

            vTaskDelay(pdMS_TO_TICKS(250));

            /* attach to network */
            if (lib_lte_attach_to_network() != LTE_SUCCESS) {
                tries += 1;
                break;
            }

            ret = LTE_SUCCESS;
        } while (0);
    }
    return ret;
}

/**
 * @brief connect to server and send photo metadata
 *
 * @param num_tries number of command retries to tolerate
 * @param image_size size of image being subsequently sent
 * @param uuid device uuid
 * @return lib_lte_result_E
 */
static lib_lte_result_E app_demo_connect_to_server(alt_u16 num_tries, alt_u32 image_size, alt_u32 uuid) {
    lib_lte_result_E ret = LTE_ERROR;
    alt_u16 tries = 0;

    do {
        /* setup module side connection metadata */
        while (lib_lte_setup_http_connection(APP_DEMO_SERVER_URL, 350, 4096) != LTE_SUCCESS) {
            tries++;
            if (tries >= num_tries) {
                break;
            }
        }

        /* start http connection on module side */
        while (lib_lte_start_http_connection() != LTE_SUCCESS) {
            tries++;
            if (tries >= num_tries) {
                break;
            }
        }

        /* clear both http head and body */
        while ((lib_lte_clear_http_body() != LTE_SUCCESS) || (lib_lte_clear_http_header() != LTE_SUCCESS)) {
            tries++;
            if (tries >= num_tries) {
                break;
            }
        }

        /* write image size */
        char size_str[sizeof(APP_DEMO_IMAGE_SIZE)+10];
        sprintf(size_str, APP_DEMO_IMAGE_SIZE, image_size);
        while (lib_lte_write_to_http_body(size_str) != LTE_SUCCESS) {
            tries++;
            if (tries >= num_tries) {
                break;
            }
        }

        /* write uuid */
        char uuid_str[sizeof(APP_DEMO_UUID)+10];
        sprintf(uuid_str, APP_DEMO_UUID, uuid);
        while (lib_lte_write_to_http_body(uuid_str) != LTE_SUCCESS) {
            tries++;
            if (tries >= num_tries) {
                break;
            }
        }

        /* post command */
        alt_u16 status = 0;
        alt_u16 resp_size = 0;
        while ((lib_lte_post_http_request(APP_DEMO_IMAGE_ENDPOINT, &status, &resp_size) != LTE_SUCCESS) || (status != APP_DEMO_HTTP_OK)) {
            tries++;
            if (tries >= num_tries) {
                break;
            }
        }

        /* only successful if we send request using less than allocated retries */
        if (tries <= num_tries) {
            ret = LTE_SUCCESS;
        }
    } while (0);

    return ret;
}

/**
 * @brief attempt to send camera chunk to server
 *
 * @param num_tries number of retries in case where module command fails
 * @param encoded_data encoded data to send
 * @param encoded_data_size size of encoded data to send
 * @param total_sent total sent data of original image size including this chunk
 * @param current_payload size of current payload (will be modified by method)
 * @param image_size total size of image (non-encoded)
 * @param uuid device uuid to send in each chunk
 * @return lib_lte_result_E
 */
static lib_lte_result_E app_demo_send_camera_chunk(alt_u16 num_tries, alt_u8* encoded_data, alt_u32 encoded_data_size, alt_u32 total_sent, alt_u32 *current_payload, alt_u32 image_size, alt_u32 uuid) {
    lib_lte_result_E ret = LTE_ERROR;
    alt_u16 tries = 0;

    do {
        sprintf(data_transfer_outptr, APP_DEMO_IMAGE_DATA, total_sent, encoded_data);

        /* try and write data to module field, if we get an error, incrememnt the current payload to be safe*/
        while (lib_lte_write_to_http_body(data_transfer_outptr) != LTE_SUCCESS) {
            tries++;
            *current_payload += encoded_data_size;
            if (tries >= num_tries) {
                break;
            }
        }

        *current_payload += encoded_data_size;
        if ((*current_payload >= APP_DEMO_MAX_HTTP_PAYLOAD) || (total_sent == image_size)) {
            /* write uuid */
            char uuid_str[sizeof(APP_DEMO_UUID)+10];
            sprintf(uuid_str, APP_DEMO_UUID, uuid);
            while (lib_lte_write_to_http_body(uuid_str) != LTE_SUCCESS) {
                tries++;
                if (tries >= num_tries) {
                    break;
                }
            }
            alt_u16 status = 400;
            alt_u16 resp_size = 0;
            while ((lib_lte_post_http_request(APP_DEMO_IMAGE_ENDPOINT, &status, &resp_size) != LTE_SUCCESS) || (status != APP_DEMO_HTTP_OK)) {
                tries++;
                if (tries >= num_tries) {
                    break;
                }
            }
            *current_payload = 0;
            while (lib_lte_clear_http_body() != LTE_SUCCESS) {
                tries++;
                if (tries >= num_tries) {
                    break;
                }
            }
        }

        /* only successful if we send request using less than allocated retries */
        if (tries <= num_tries) {
            ret = LTE_SUCCESS;
        }
    } while (0);
    return ret;
}

/**
 * @brief Connect to sever and send lat/long data
 *
 * @param num_tries number of AT command retries to accept before failure
 * @param lat null appended latitude string
 * @param longi null appended longitude string
 * @param uuid device uuid
 * @param gps_valid indicate weather or not the GPS data is valid data
 * @return lib_lte_result_E
 */
static app_demo_server_resp_E app_demo_check_in_to_server(alt_u16 num_tries, alt_u8* lat, alt_u8* longi, alt_u32 uuid, bool gps_valid) {
    app_demo_server_resp_E ret = APP_DEMO_SERVER_RESPONSE_ERROR;
    alt_u16 tries = 0;

    do {
        /* setup module side connection metadata */
        while (lib_lte_setup_http_connection(APP_DEMO_SERVER_URL, 350, 4096) != LTE_SUCCESS) {
            tries++;
            if (tries >= num_tries) {
                break;
            }
        }

        /* start http connection on module side */
        while (lib_lte_start_http_connection() != LTE_SUCCESS) {
            tries++;
            if (tries >= num_tries) {
                break;
            }
        }

        /* clear both http head and body */
        while ((lib_lte_clear_http_header() != LTE_SUCCESS) && (lib_lte_clear_http_body() != LTE_SUCCESS)) {
            tries++;
            if (tries >= num_tries) {
                break;
            }
        }

        /* write lat */
        char lat_str[sizeof(APP_DEMO_LAT_DATA)+strlen(lat)];
        sprintf(lat_str, APP_DEMO_LAT_DATA, lat);
        while (lib_lte_write_to_http_body(lat_str) != LTE_SUCCESS) {
            tries++;
            if (tries >= num_tries) {
                break;
            }
        }

        /* write long */
        char long_str[sizeof(APP_DEMO_LONG_DATA)+strlen(longi)];
        sprintf(long_str, APP_DEMO_LONG_DATA, longi);
        while (lib_lte_write_to_http_body(long_str) != LTE_SUCCESS) {
            tries++;
            if (tries >= num_tries) {
                break;
            }
        }

        /* write uuid */
        char uuid_str[sizeof(APP_DEMO_UUID)+10];
        sprintf(uuid_str, APP_DEMO_UUID, uuid);
        while (lib_lte_write_to_http_body(uuid_str) != LTE_SUCCESS) {
            tries++;
            if (tries >= num_tries) {
                break;
            }
        }

        /* write data valid */
        if (gps_valid == true) {
            char valid_str[sizeof(APP_DEMO_GPS_VALID)+10];
            sprintf(valid_str, APP_DEMO_GPS_VALID);
            while (lib_lte_write_to_http_body(valid_str) != LTE_SUCCESS) {
                tries++;
                if (tries >= num_tries) {
                    break;
                }
            }
        } else {
            char valid_str[sizeof(APP_DEMO_GPS_NOT_VALID)+10];
            sprintf(valid_str, APP_DEMO_GPS_NOT_VALID);
            while (lib_lte_write_to_http_body(valid_str) != LTE_SUCCESS) {
                tries++;
                if (tries >= num_tries) {
                    break;
                }
            }
        }

        /* post command */
        alt_u16 status = 0;
        alt_u16 resp_size = 0;
        while ((lib_lte_post_http_request(APP_DEMO_GPS_ENDPOINT, &status, &resp_size) != LTE_SUCCESS) && (status != APP_DEMO_HTTP_OK)) {
            tries++;
            if (tries >= num_tries) {
                break;
            }
        }

        /* read response */
        alt_u8* resp_data = (alt_u8*)pvPortMalloc(resp_size + 50);
        resp_data[resp_size+49] = '\0';
        while (lib_lte_get_http_response_data(resp_size, 0, resp_data, resp_size+49) != LTE_SUCCESS) {
            tries++;
            if (tries >= num_tries) {
                vPortFree(resp_data);
                break;
            }
        }

        /* only successful if we send request using less than allocated retries */
        if (tries <= num_tries) {
            if (strstr(resp_data, APP_DEMO_SERVER_RESP_UNLOCK) != NULL) {
                ret = APP_DEMO_SERVER_RESPONSE_UNLOCK;
            } else if (strstr(resp_data, APP_DEMO_SERVER_RESP_TAKE_PHOTO) != NULL) {
                ret = APP_DEMO_SERVER_RESPONSE_TAKE_PHOTO;
            } else if (strstr(resp_data, APP_DEMO_SERVER_RESP_LOCK) != NULL) {
                ret = APP_DEMO_SERVER_REPSONSE_LOCK;
            }
        }

        vPortFree(resp_data);
    } while (0);

    return ret;
}

/**
 * @brief take and upload a photo
 *
 */
static bool app_demo_take_picture(alt_u32 uuid) {
    /* schedule picture */
    app_camera_datastram_id id;
    app_camera_schedule_picture(&id);
    vTaskDelay(pdMS_TO_TICKS(200)); // wait to allow scheduling
    alt_u32 image_size = app_camera_get_image_size(id);
    /**
     * Here we do the following:
     * 1) Turn off GPS
     * 2) Connect to the server
     * 3) Send image size header
     * 4) Make sure the image size header was received
     */
    vTaskDelay(pdMS_TO_TICKS(500));
    alt_u32 total_sent = 0;
    alt_u32 current_payload = 0;
    if (app_demo_connect_to_server(APP_DEMO_DEFAULT_ALLOWABLE_RETRIES, image_size, uuid) != LTE_SUCCESS) {
        printf("FAILURE\n");
        return false;
    }
    while (app_camera_get_stream_status(id) != CAMERA_STREAM_UNKNOWN) {
        alt_u8** data;
        alt_u32 size;
        if (app_camera_get_next_stream_chunk(data, &size, id) == CAMERA_SUCCESS) {
            alt_u8* dataptr = *data;
            alt_u32 outsize;
            lib_base64_encode_static(dataptr, size, &outsize, data_transfer_ptr, LIB_BASE64_DEFAULT_CAMERA_ENCODE_SIZE + 1);
            vPortFree(*data);
            /**
             * Here we would do the following:
             * 3) Send image data
             * 4) Make sure the image data was received
             */
            total_sent += size;
            if (app_demo_send_camera_chunk(APP_DEMO_DEFAULT_ALLOWABLE_RETRIES, data_transfer_ptr, outsize, total_sent, &current_payload, image_size, uuid) != LTE_SUCCESS) {
                printf("FAILURE\n");
                return false;
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(APP_DEMO_RUN_PERIOD_MS));
    }
    /* kill http connection on finish */
    lib_lte_end_http_connection();
    return true;
}

/**
 * @brief check if a photo should be uploaded manually
 *
 * @return true
 * @return false
 */
static bool app_demo_check_for_picture(void) {
    if ((*((volatile unsigned int*) SWITCH_BASE) != 15)) {
        return true;
    }
    return false;
}

/**
 * @brief handle error stat
 *
 * @return true
 * @return false
 */
static bool app_demo_handle_error(void) {
    bool ret = false;
    printf("DEVICE ERROR - ATTEMPTING TO HANDLE\n");
    do {
        if (lib_lte_reset_module() != LTE_SUCCESS) {
            break;
        }

        if (lib_gps_reset_module() != LTE_SUCCESS) {
            break;
        }

        if (app_demo_attach_to_network(APP_DEMO_DEFAULT_ALLOWABLE_RETRIES) != LTE_SUCCESS) {
            break;
        }

        if (lib_gps_turn_on_gps() != LTE_SUCCESS) {
            break;
        }
        ret = true;
    } while(0);

    return ret;
}

/**
 * @brief send gps data to server and get server action
 *
 * @param uuid device id to identify hardware with server
 *
 * @return true
 * @return false
 */
static app_demo_server_resp_E app_demo_contact_server(alt_u32 uuid) {

    app_demo_server_resp_E ret = APP_DEMO_SERVER_RESPONSE_ERROR;

    alt_u8 lat[10] = {0};
    alt_u8 longi[12] = {0};
    do {
        /* read gps data, try and send result to server and get server action */
        if (lib_gps_read_gps(lat, longi) != LTE_SUCCESS) {
            ret = app_demo_check_in_to_server(APP_DEMO_DEFAULT_ALLOWABLE_RETRIES, "0" , "0", uuid, false);
        } else {
            ret = app_demo_check_in_to_server(APP_DEMO_DEFAULT_ALLOWABLE_RETRIES, lat , longi, uuid, true);
        }

        if (lib_lte_end_http_connection() != LTE_SUCCESS) {
            ret = APP_DEMO_SERVER_RESPONSE_ERROR;
            break;
        }

    } while(0);

    return ret;
}

/**
 * @brief init device and connect to network
 *
 * @param uuid
 */
static bool app_demo_init(alt_u32 uuid) {
    bool ret = false;
    /* Initialize module */
    lib_lte_init(CELL_MODULE_BASE, CELL_MODULE_IRQ);
    lib_gps_init(GPS_BASE, GPS_IRQ);
	if (lib_gps_power_state_on((volatile unsigned int *)GPIO_OUT_BASE) == GPS_TIMEOUT) {
        lib_gps_power_state_on((volatile unsigned int *)GPIO_OUT_BASE);
    }
	lib_lte_reset_module();
	lib_gps_reset_module();

    /* try and connect to network - if unable, retry every minute */
    while (app_demo_attach_to_network(APP_DEMO_DEFAULT_ALLOWABLE_RETRIES)) {
        printf("Bad reception or network issue\n");
        return ret;
    }

    /* turn on GPS hardware */
    lib_gps_turn_on_gps();
    printf("module ready!\n");
    vTaskDelay(pdMS_TO_TICKS(2000));
    ret = true;
    return ret;
}

/**
 * @brief run function for app_demo
 *
 * @param p generic task datapointer
 */
void app_demo_run(void*p) {

    /* init device and check if we need to go */
    if (app_demo_init(APP_DEMO_DEVICE_UUID) == false) {
        current_state = APP_DEMO_STATE_ERROR;
    }

    alt_u32 server_contact_count = 0;
    alt_u32 unlocked_error_count = 0; /* in the case a server check-in fails when unlocked, do not immediately re-lock*/
    data_transfer_ptr = (alt_u8*)pvPortMalloc(LIB_BASE64_DEFAULT_CAMERA_ENCODE_SIZE + 1);
    memset(data_transfer_ptr, 0, LIB_BASE64_DEFAULT_CAMERA_ENCODE_SIZE + 1);

    data_transfer_outptr = (alt_u8*)pvPortMalloc(sizeof(APP_DEMO_IMAGE_DATA) + 1 + LIB_BASE64_DEFAULT_CAMERA_ENCODE_SIZE);
    memset(data_transfer_outptr, 0, sizeof(APP_DEMO_IMAGE_DATA) + 1 + LIB_BASE64_DEFAULT_CAMERA_ENCODE_SIZE);

    while (1) {
        switch(current_state) {
            case APP_DEMO_STATE_IDLE_LOCKED:
            {
#if (APP_DEMO_DEBUG_MSG == 1)
                printf("\nSTATE: IDLE_LOCKED\n");
#endif
                if (app_demo_check_for_picture() == true) {
                    current_state = APP_DEMO_STATE_UPLOAD_PICTURE;
                }
                server_contact_count += 1;
                if (server_contact_count == APP_DEMO_SERVER_CONTACT_SKIPS) {
                    app_demo_server_resp_E resp = app_demo_contact_server(APP_DEMO_DEVICE_UUID);
                    current_state = (app_demo_state_E) resp; /* change state based on response */
                    server_contact_count = 0;
                }
                break;
            }
            case APP_DEMO_STATE_IDLE_UNLOCKED:
            {
#if (APP_DEMO_DEBUG_MSG == 1)
                printf("\nSTATE: IDLE_UNLOCKED\n");
#endif
                server_contact_count += 1;
                if (server_contact_count == APP_DEMO_SERVER_CONTACT_SKIPS) {
                    app_demo_server_resp_E resp = app_demo_contact_server(APP_DEMO_DEVICE_UUID);
                    /* if there is an error when checking into server when unlocked, we retry before re-locking */
                    if (resp == APP_DEMO_SERVER_RESPONSE_ERROR) {
                        unlocked_error_count++;
                        if (unlocked_error_count == APP_DEMO_UNLOCKED_ERROR_RETRIES) {
                            current_state = APP_DEMO_STATE_ERROR;
                            unlocked_error_count = 0;
                        }
                    } else {
                        current_state = (app_demo_state_E) resp; /* change state based on response */
                    }
                    server_contact_count = 0;
                }
                break;
            }
            case APP_DEMO_STATE_UPLOAD_PICTURE:
            {
#if (APP_DEMO_DEBUG_MSG == 1)
                printf("\nSTATE: PICTURE\n");
#endif
                printf("START PICTURE %d\n", xTaskGetTickCount());
                if (app_demo_take_picture(APP_DEMO_DEVICE_UUID) == true) {
                    current_state = APP_DEMO_STATE_IDLE_LOCKED;
                    printf("END PICTURE %d\n", xTaskGetTickCount());
                    server_contact_count = APP_DEMO_SERVER_WAIT_TIME_AFTER_IMAGE_SKIPS; // wait for approx 3s to request again
                } else {
                    current_state = APP_DEMO_STATE_ERROR;
                }
                break;
            }
            case APP_DEMO_STATE_ERROR:
            {
#if (APP_DEMO_DEBUG_MSG == 1)
                printf("\nSTATE: ERROR\n");
#endif
                if (app_demo_handle_error() == true) {
                    current_state = APP_DEMO_STATE_IDLE_LOCKED;
                }
                break;
            }
            default:
            {
                current_state = APP_DEMO_STATE_ERROR;
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(APP_DEMO_RUN_PERIOD_MS));
    }
}

/**
 * @brief get device state
 *
 * @return app_demo_state_E
 */
app_demo_state_E app_demo_get_state(void) {
    return current_state;
}
