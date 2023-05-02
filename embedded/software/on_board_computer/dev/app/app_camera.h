/**
 * @file app_camera.h
 * @author Emery Nagy
 * @brief Higher level control over VC0706 camera
 * @version 0.1
 * @date 2023-02-13
 * 
 */

#ifndef APP_CAMERA_H_
#define APP_CAMERA_H_

/* public defines */
#define APP_CAMERA_DEFAULT_STREAM_CHUNK_SIZE 48

/* public types */

/* camera error type */
typedef enum {
    CAMERA_SUCCESS,
    CAMERA_ERROR
} app_camera_result_E;

/* indicates the status of the camera stream */
typedef enum {
    CAMERA_STREAM_IN_QUEUE,
    CAMERA_STREAM_RUNNING,
    CAMERA_STREAM_FULL,
    CAMERA_STREAM_DONE,
    CAMERA_STREAM_CRITICAL_ERROR,
    CAMERA_STREAM_UNKNOWN
} app_camera_stream_status_E;

/* identify specific camera datastream */
typedef alt_u32 app_camera_datastram_id;

/* used by other apps to retrieve single stream objects */
typedef struct {
    /* datachunk */
    alt_u8* dataptr;
    /* chunk size in bytes */
    alt_u32 size;
    /* chunk unique id */
    alt_u32 id;

} app_camera_photo_chunk_T;

/* public API */
void app_camera_run(void *p);
app_camera_result_E app_camera_schedule_picture(alt_u32* camera_stream_id);
app_camera_stream_status_E app_camera_get_stream_status(alt_u32 id);
app_camera_result_E app_camera_get_next_stream_chunk(alt_u8** data, alt_u32* size, alt_u32 id);
alt_u32 app_camera_get_image_size(alt_u32 id);

#endif /* APP_CAMERA_H_ */
