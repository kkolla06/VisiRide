/**
 * @file main.c
 * @author Emery N
 * @brief Sys entrypoint and heartbeat monitor
 * @version 0.1
 * @date 2023-04-10
 *
 */

/* includes */
#include "FreeRTOS.h"
#include "task.h"
#include "sys/alt_irq.h"
#include "system.h"
#include "stdio.h"
#include "app_camera.h"
#include "app_demo.h"
#include "lib_lte.h"
#include "lib_lte_cmd.h"
#include "string.h"


/* defines */
#define SYS_HEARTBEAT_PRINT_THREAD_STATUS 1

/* private functions for bit manipulations */

/**
 * @brief Set the kth bit of n
 *
 * @param n
 * @param k
 * @return int
 */
static inline int set_bit(int n, int k)
{
    return (n | (1 << (k - 1)));
}

/**
 * @brief Clear the kth bit of n
 *
 * @param n
 * @param k
 * @return int
 */
static inline int clear_bit(int n, int k)
{
    return (n & (~(1 << (k - 1))));
}

/**
 * @brief toggle the kth bit of n
 *
 * @param n
 * @param k
 * @return int
 */
static inline int toggle_bit(int n, int k)
{
    return (n ^ (1 << (k - 1)));
}

/**
 * @brief system heartbeat task, responsible for lighting indications and also
 *
 * @param p
 */
void sys_heartbeat(void *p) {
	while (1) {
		volatile unsigned int c_led = *((volatile unsigned int *)LEDS_BASE);
		c_led = toggle_bit(c_led, 1);
		app_demo_state_E state = app_demo_get_state();

		if (state == APP_DEMO_STATE_IDLE_LOCKED) {
			c_led = clear_bit(c_led, 2);
			c_led = clear_bit(c_led, 3);
		} else if (state == APP_DEMO_STATE_UPLOAD_PICTURE) {
			c_led = set_bit(c_led, 2);
			c_led = clear_bit(c_led, 3);
		} else if (state == APP_DEMO_STATE_IDLE_UNLOCKED){
			c_led = clear_bit(c_led, 2);
			c_led = set_bit(c_led, 3);
		} else {
			/* error case */
			c_led = set_bit(c_led, 1);
			c_led = set_bit(c_led, 2);
			c_led = set_bit(c_led, 3);
		}

	*((volatile unsigned int *)LEDS_BASE) = c_led;

	/* report thread status via FreeRTOS, this is not a computationally cheap operation so only turn on when necessary */
#if (SYS_HEARTBEAT_PRINT_THREAD_STATUS == 1)
	TaskHandle_t camera_handle =  xTaskGetHandle( "camera" );
	TaskHandle_t app_handle =  xTaskGetHandle( "demo" );

	TaskStatus_t camera_details;
	TaskStatus_t app_details;

	vTaskGetInfo(camera_handle, &camera_details, pdTRUE, eInvalid);
	vTaskGetInfo(app_handle, &app_details, pdTRUE, eInvalid);

	printf("CAMERA -> STACK:%d, STATE:%d\n", camera_details.usStackHighWaterMark, camera_details.eCurrentState);
	printf("APP -> STACK:%d, STATE:%d\n", app_details.usStackHighWaterMark, app_details.eCurrentState);
	printf("DEVICE MASTER STATE %d\n\n", state);
#endif

	vTaskDelay(pdMS_TO_TICKS(1000));
	}

}

int main()
{
	printf("Starting scheduler\n");
	*((volatile unsigned int *)LEDS_BASE) = 0;
	xTaskCreate(sys_heartbeat, "sys_heartbeat", 1024, NULL, 2, NULL);
	xTaskCreate(app_camera_run, "camera", 2048, NULL, 3, NULL);
	xTaskCreate(app_demo_run, "demo", 2048, NULL, 5, NULL);
	vTaskStartScheduler();
	return 0;
}
