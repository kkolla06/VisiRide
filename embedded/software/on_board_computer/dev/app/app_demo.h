/**
 * @file app_demo.h
 * @author Emery Nagy
 * @brief Demo app
 * @version 0.1
 * @date 2023-02-13
 * 
 */

#ifndef APP_DEMO_H_
#define APP_DEMO_H_

/* public types */
typedef enum {
    APP_DEMO_STATE_IDLE_LOCKED = 0, /* run gps check-in, scooter is locked */
    APP_DEMO_STATE_IDLE_UNLOCKED, /* run gps check-in, scooter is unlocked */
    APP_DEMO_STATE_UPLOAD_PICTURE, /* currently uploading picture */
    APP_DEMO_STATE_ERROR /* error with module or connectivity */
} app_demo_state_E;

/* public API */
void app_demo_run(void* p);
app_demo_state_E app_demo_get_state(void);

#endif /* APP_DEMO_H_ */