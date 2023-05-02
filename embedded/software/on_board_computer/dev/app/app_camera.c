/**
 * @file app_camera.c
 * @author Emery Nagy
 * @brief Higher level control over VC0706 camera, specifically for picture taking
 * @version 0.1
 * @date 2023-02-13
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

/* app includes */
#include "app_camera.h"

/* defines */

#define APP_CAMERA_DEBUG_MSG 0

#define APP_CAMERA_DEFAULT_STREAM_CHUNK_SIZE 48
#define APP_CAMERA_DEFAULT_STREAM_OUT_QUEUE_SIZE 64
#define APP_CAMERA_DEFAULT_REQUEST_QUEUE_SIZE 10
#define APP_CAMERA_RESET_DELAY_MS 1000
#define APP_CAMERA_CHUNK_REQUEST_TIMEOUT_MS 100
#define APP_CAMERA_MUTEX_BLOCKTIME_MS pdMS_TO_TICKS(12)
#define APP_CAMERA_STREAM_TIMEOUT pdMS_TO_TICKS(120000) // 2 minute timeout per stream
#define APP_CAMERA_RUN_DELAY_MS 15

/* private data types */

/* camera image datastream*/
typedef struct {
    /* queue containing camera stream datachunks */
    QueueHandle_t out_q;
    /* datastream id for external apps to claim data */
    alt_u32 id;
    /* stream status */
    app_camera_stream_status_E status;
    /* number of datachunks that have been received from camera */
    alt_u32 completed_chunks;
    /* total chunks in photo */
    alt_u32 total_size;

} app_camera_datastream_S;

/* application state structure */
typedef struct {
    /* request queue */
    QueueHandle_t in_q;
    /* currently running datastream */
    app_camera_datastream_S* running;
    /* next assigned stream id */
    alt_u32 next_gen;
    /* last stream that was finished */
    alt_u32 last_finished;
    /* keep track if there is a current stream running */
    bool stream_running;
    /* starting timestamp of the current stream */
    alt_u32 start_timestamp;
    /* mutex to enable other threads to queue commands */
    SemaphoreHandle_t mutex;

} app_camera_S;


/* private data */

static app_camera_S app_camera_state = {
    .running = 0,
    .next_gen = 0,
    .last_finished = 0
};

/* private functions */

/**
 * @brief stop the camera frame buffer and get the picture size
 *
 * @param datasize retrieved picture size
 * @return app_camera_result_E
 */
app_camera_result_E app_camera_take_picture_get_data_size(alt_u32* datasize) {
    app_camera_result_E res = CAMERA_ERROR;

    do {
        if (lib_VC0706_cmd_stop_frame(APP_CAMERA_CHUNK_REQUEST_TIMEOUT_MS) != VC0706_SUCCESS) {
            break;
        }
        if (lib_VC0706_cmd_get_fbuf_len(datasize, APP_CAMERA_CHUNK_REQUEST_TIMEOUT_MS) != VC0706_SUCCESS) {
            break;
        }
        if (*datasize == 0) {
            printf("FATAL CAMERA ERROR\n");
            break;
        }

        res = CAMERA_SUCCESS;
    } while (0);

    return res;
}

/**
 * @brief reset camera, wait 1s for garbage on UART line to be worked out
 *
 * @return app_camera_result_E
 */
app_camera_result_E app_camera_reset_due_to_error(void) {
    app_camera_result_E res = CAMERA_ERROR;

    if (lib_VC0706_cmd_reset_camera(APP_CAMERA_CHUNK_REQUEST_TIMEOUT_MS) == VC0706_SUCCESS) {
        res = CAMERA_SUCCESS;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));

    return res;
}

/**
 * @brief deallocated all heap memory associated with camera data stream
 *
 * @param stream camera stream to vPortFree
 */
void app_camera_output_stream_free(app_camera_datastream_S* stream) {

    if (pdTRUE == xSemaphoreTake(app_camera_state.mutex, APP_CAMERA_MUTEX_BLOCKTIME_MS)) {
        /* must vPortFree induvidual dataptrs in queue object since FreeRTOS will only vPortFree direct data */
        while (uxQueueMessagesWaiting(stream->out_q) > 0) {
            app_camera_photo_chunk_T* chunk = (app_camera_photo_chunk_T*)pvPortMalloc(sizeof(app_camera_photo_chunk_T));
            xQueueReceive(stream->out_q, &chunk, APP_CAMERA_CHUNK_REQUEST_TIMEOUT_MS);
            alt_u8* dataptr = chunk->dataptr;
            vPortFree(dataptr);
        }

        /* vPortFree the queue itself */
        vQueueDelete(stream->out_q);
        /* vPortFree the output stream data */
        vPortFree(stream);
        xSemaphoreGive(app_camera_state.mutex);
    }
}



/* public API */

/**
 * @brief schedule a photo
 *
 * @param camera_stream_id streamid for client to access datastream
 * @return app_camera_result_E
 */
app_camera_result_E app_camera_schedule_picture(alt_u32* camera_stream_id) {
    app_camera_result_E res = CAMERA_ERROR;

    if ((pdTRUE == xSemaphoreTake(app_camera_state.mutex, APP_CAMERA_MUTEX_BLOCKTIME_MS)) &&
                                                                    uxQueueSpacesAvailable(app_camera_state.in_q) > 0) {
        app_camera_datastream_S* new_stream = (app_camera_datastream_S*)pvPortMalloc(sizeof(app_camera_datastream_S));
        new_stream->id = app_camera_state.next_gen;
        *camera_stream_id = app_camera_state.next_gen;
        app_camera_state.next_gen++;
        xQueueSend(app_camera_state.in_q, new_stream, APP_CAMERA_MUTEX_BLOCKTIME_MS);
        vPortFree(new_stream);

        xSemaphoreGive(app_camera_state.mutex);
    }

    return res;
}

/**
 * @brief get camera stream data
 *
 * @param outchunk new camera data
 * @param id stream id, must match currently streaming photo
 * @return app_camera_result_E
 */
app_camera_result_E app_camera_get_next_stream_chunk(alt_u8** data, alt_u32* size, alt_u32 id) {
    app_camera_result_E res = CAMERA_ERROR;

    /* only send data out if the client is the correct recipient */
    if (app_camera_state.stream_running && (pdTRUE == xSemaphoreTake(app_camera_state.mutex, APP_CAMERA_MUTEX_BLOCKTIME_MS))) {
        if ((id == app_camera_state.running->id) && (uxQueueMessagesWaiting(app_camera_state.running->out_q) > 0)) {
            app_camera_photo_chunk_T outchunk;
            xQueueReceive(app_camera_state.running->out_q, &outchunk, APP_CAMERA_MUTEX_BLOCKTIME_MS);
            *size = outchunk.size;
            *data = outchunk.dataptr;
            res = CAMERA_SUCCESS;
#if (APP_CAMERA_DEBUG_MSG == 1)
            printf("reading chunk id %d, size %d\n", outchunk.id, *size);
#endif
        }
        xSemaphoreGive(app_camera_state.mutex);
    }

    return res;
}

/**
 * @brief get the status of a specific camera stream
 *
 * @param id stream id to check
 * @return app_camera_stream_status_E
 */
app_camera_stream_status_E app_camera_get_stream_status(alt_u32 id) {
    app_camera_stream_status_E stream = CAMERA_STREAM_UNKNOWN;

    /* if there is a stream currently running we check there first */
    if ((app_camera_state.stream_running) && (id == app_camera_state.running->id)) {
        stream = app_camera_state.running->status;
    } else if ((id > app_camera_state.last_finished) && (id < app_camera_state.next_gen)) {
        stream = CAMERA_STREAM_IN_QUEUE;
    }

    return stream;
}

/**
 * @brief get picture length in bytes for given stream (if known). returns 0 if unknown
 *
 * @param id stream id to check
 * @return alt_u32
 */
alt_u32 app_camera_get_image_size(alt_u32 id) {
    if ((app_camera_state.stream_running) && (id == app_camera_state.running->id)) {
        return app_camera_state.running->total_size;
    } else {
        return 0;
    }
}


void app_camera_run(void *p) {

    /* init camera and app data */
    lib_VC0706_init(CAMERA_BASE, CAMERA_IRQ);
    app_camera_state.in_q = xQueueCreate(APP_CAMERA_DEFAULT_REQUEST_QUEUE_SIZE, sizeof(app_camera_datastream_S));
    app_camera_state.mutex = xSemaphoreCreateMutex();

    /* reset camera for use */
    lib_VC0706_cmd_reset_camera(APP_CAMERA_CHUNK_REQUEST_TIMEOUT_MS);
    vTaskDelay(pdMS_TO_TICKS(APP_CAMERA_RESET_DELAY_MS));
    //lib_VC0706_increase_baud(pdMS_TO_TICKS(APP_CAMERA_RESET_DELAY_MS)); // set baud rate to 115200 baud

    /* task idle */
    while (1) {

        /* start running new stream */
        if ((!app_camera_state.stream_running) && (uxQueueMessagesWaiting(app_camera_state.in_q) > 0)) {
            app_camera_datastream_S* new_stream = (app_camera_datastream_S*) pvPortMalloc(sizeof(app_camera_datastream_S));
            xQueueReceive(app_camera_state.in_q, new_stream, portMAX_DELAY);
            alt_u32 picturesize = 0;
            /* "take picture", verify we can start stream properly, if not set error status and restart camera */
            if (app_camera_take_picture_get_data_size(&picturesize) != CAMERA_ERROR) {
#if (APP_CAMERA_DEBUG_MSG == 1)
                printf("picturesize: %d", picturesize);
#endif
                app_camera_state.running = new_stream;
                new_stream->status = CAMERA_STREAM_RUNNING;
                app_camera_state.stream_running = true;
                app_camera_state.start_timestamp = xTaskGetTickCount();
                new_stream->completed_chunks = 0;
                new_stream->total_size = picturesize;
                new_stream->out_q = xQueueCreate(APP_CAMERA_DEFAULT_STREAM_OUT_QUEUE_SIZE, sizeof(app_camera_photo_chunk_T));
                /* TODO: add stream start callback notification */
            } else {
                /* TODO: add error callback notification */
                vPortFree(new_stream);
                app_camera_reset_due_to_error();
            }
        } else if (app_camera_state.stream_running) {
            /* see if we need to timeout stream */
            if ((xTaskGetTickCount() - app_camera_state.start_timestamp) > APP_CAMERA_STREAM_TIMEOUT) {
                app_camera_state.stream_running = false;
                app_camera_output_stream_free(app_camera_state.running);
                /* TODO: add error callback notification */
                lib_VC0706_cmd_start_frame(APP_CAMERA_CHUNK_REQUEST_TIMEOUT_MS);
            } else if ((uxQueueMessagesWaiting(app_camera_state.running->out_q) < APP_CAMERA_DEFAULT_STREAM_OUT_QUEUE_SIZE) && (app_camera_state.running->status == CAMERA_STREAM_RUNNING)) {
                /* get next chunk */
                alt_u32 start_addr = app_camera_state.running->completed_chunks*APP_CAMERA_DEFAULT_STREAM_CHUNK_SIZE;
                alt_u32 number_bytes_left = (app_camera_state.running->total_size - start_addr);
                /* although most chunks are default byte size, the last one in any photo might be smaller */
                alt_u32 number_bytes = (number_bytes_left > APP_CAMERA_DEFAULT_STREAM_CHUNK_SIZE) ?
                                                            APP_CAMERA_DEFAULT_STREAM_CHUNK_SIZE : number_bytes_left;
                alt_u8* new_data = (alt_u8*) pvPortMalloc(number_bytes);
                if (lib_VC0706_cmd_get_fbuf_data(new_data, APP_CAMERA_CHUNK_REQUEST_TIMEOUT_MS, start_addr, number_bytes)
                                                                                                    == VC0706_SUCCESS) {
                    app_camera_photo_chunk_T new_chunk = {
                        .dataptr = new_data,
                        .size = number_bytes,
                        .id = app_camera_state.running->completed_chunks
                    };
                    xQueueSend(app_camera_state.running->out_q, &new_chunk, APP_CAMERA_MUTEX_BLOCKTIME_MS);
                    app_camera_state.running->completed_chunks++;
#if (APP_CAMERA_DEBUG_MSG == 1)
                    printf("got chunk %d, size %d\n", (app_camera_state.running->completed_chunks), new_chunk.size);
#endif
                    /* we have completed all the valid chunks */
                    alt_u32 data_processed = ((app_camera_state.running->completed_chunks)*APP_CAMERA_DEFAULT_STREAM_CHUNK_SIZE);
                    if (data_processed >= (app_camera_state.running->total_size)) {
                        app_camera_state.running->status = CAMERA_STREAM_DONE;
                        /* enable camera frame buffer to update, if operation fails then reset camera */
                        if (lib_VC0706_cmd_start_frame(APP_CAMERA_CHUNK_REQUEST_TIMEOUT_MS) != VC0706_SUCCESS) {
                            app_camera_reset_due_to_error();
                        }
                        /* although we have finished streaming the data out of the camera, the stream is not finished
                         * until client pulls all data from queue
                         */
                    } else {
                        app_camera_state.running->status = CAMERA_STREAM_RUNNING;
                    }
                } else {
#if (APP_CAMERA_DEBUG_MSG == 1)
                    printf("Chunk error, chunk: %d\n", (app_camera_state.running->completed_chunks) + 1);
#endif
                    vPortFree(new_data);
                }
            }
            else if ((app_camera_state.running->status == CAMERA_STREAM_DONE) && (uxQueueMessagesWaiting(app_camera_state.running->out_q) == 0)) {
                /* check if stream should be deallocated */
                app_camera_state.last_finished = app_camera_state.running->id;
                app_camera_state.stream_running = false;
#if (APP_CAMERA_DEBUG_MSG == 1)
                printf("Streaming done!!!");
#endif
                app_camera_output_stream_free(app_camera_state.running);
            } else if (uxQueueMessagesWaiting(app_camera_state.running->out_q) == APP_CAMERA_DEFAULT_STREAM_OUT_QUEUE_SIZE) {
                /* no more vPortFree space in output queue */
#if (APP_CAMERA_DEBUG_MSG == 1)
                printf("Queue full, chunk: %d\n", (app_camera_state.running->completed_chunks));
#endif
            }
        }
        vTaskDelay(pdMS_TO_TICKS(APP_CAMERA_RUN_DELAY_MS));
    }

}