/**
 * @file lib_gps.h
 * @author Emery Nagy
 * @brief GPS driver for SIM7080G
 * @version 0.1
 * @date 2023-02-23
 *
 */

/* includes */
#include "alt_types.h"
#include "stdbool.h"

#ifndef LIB_GPS_CMD_H_
#define LIB_GPS_CMD_H_

/* defines */
#define LIB_GPS_CMD_SIZE_OVERHEAD 5 // formatting -> AT<cmd>=<args>\r\0
#define LIB_GPS_BASIC_RESPONSE_SIZE_OVERHEAD 10 // formatting -> \r\n<cmd>\r\n\r\nOK\r\n\0
#define LIB_GPS_EXTENDED_RESPONSE_PADING 6 // formatting -> \r\n<response>\r\n

/* public types */

/* generic response type */
typedef enum {
    LIB_GPS_RESP_TYPE_BASIC, // basic response type with OK, ERROR
    LIB_GPS_RESP_TYPE_STRING, // response data is in format <cmd>: <data> (can be in async response), autocheck that response exists, caller must actually parse response data
    LIB_GPS_RESP_TYPE_ASYNC // response data has completely seperate async response regex
} lib_gps_resp_type_E;

/* generic AT command type */
typedef struct {
    const char* cmd; /* command string */
    alt_u32 cmd_len;
    const char* cmd_args; /* command arguments */
    alt_u32 args_len;
    const char* response_str; /* response regex */
    alt_u32 resp_len;
    bool is_query; /* is this a query AT command? Uses the '?' syntax */
    lib_gps_resp_type_E resp_type; /* basic or data response type -> Supports async responses */
    bool formatted_args; /* should we use formatted args instead of the static provided args */
} lib_gps_cmd_type_E;

/* Command components */
extern const char LIB_GPS_CMD_HEADER[];
extern const char LIB_GPS_CMD_FOOTER[];
extern const char LIB_GPS_CMD_PADDING[];
extern const char LIB_GPS_CMD_QUERY[];
extern const char LIB_GPS_CMD_ARGS_ASSIGNMENT[];
extern const char LIB_GPS_CMD_RESP_OK[];
extern const char LIB_GPS_CMD_RESP_ERROR[];

/* Command strings */
extern const char LIB_GPS_CMD_RESET_STRING[];
extern const char LIB_GPS_CMD_SIM_STATUS_STRING[];
extern const char LIB_GPS_PWR_STRING[];
extern const char LIB_GPS_DATA_STRING[];
extern const char LIB_GPS_ON_ARGS_STRING[];
extern const char LIB_GPS_OFF_ARGS_STRING[];
extern const char LIB_GPS_DATA_RESPONSE_STRING[];

/* status commands */
extern const lib_gps_cmd_type_E LIB_GPS_RESET_CMD;
extern const lib_gps_cmd_type_E LIB_GPS_SIM_STATUS_CMD;

/* GPS commands */
extern const lib_gps_cmd_type_E LIB_GPS_POWER_ON_CMD;
extern const lib_gps_cmd_type_E LIB_GPS_POWER_OFF_CMD;
extern const lib_gps_cmd_type_E LIB_GPS_GET_LOCATION_CMD;

#endif /* LIB_GPS_CMD_H_ */
