/**
 * @file lib_lte.h
 * @author Emery Nagy
 * @brief LTE driver for SIM7080G
 * @version 0.1
 * @date 2023-02-23
 *
 */

/* includes */
#include "alt_types.h"
#include "stdbool.h"

#ifndef LIB_LTE_CMD_H_
#define LIB_LTE_CMD_H_

/* defines */
#define LIB_LTE_CMD_SIZE_OVERHEAD 5 // formatting -> AT<cmd>=<args>\r\0
#define LIB_LTE_BASIC_RESPONSE_SIZE_OVERHEAD 10 // formatting -> \r\n<cmd>\r\n\r\nOK\r\n\0
#define LIB_LTE_EXTENDED_RESPONSE_PADING 6 // formatting -> \r\n<response>\r\n

/* public types */

/* generic response type */
typedef enum {
    LIB_LTE_RESP_TYPE_BASIC, // basic response type with OK, ERROR
    LIB_LTE_RESP_TYPE_STRING, // response data is in format <cmd>: <data> (can be in async response), autocheck that response exists, caller must actually parse response data
    LIB_LTE_RESP_TYPE_ASYNC // response data has completely seperate async response regex
} lib_lte_resp_type_E;

/* generic AT command type */
typedef struct {
    const char* cmd; /* command string */
    alt_u32 cmd_len;
    const char* cmd_args; /* command arguments */
    alt_u32 args_len;
    const char* response_str; /* response regex */
    alt_u32 resp_len;
    bool is_query; /* is this a query AT command? Uses the '?' syntax */
    lib_lte_resp_type_E resp_type; /* basic or data response type -> Supports async responses */
    bool formatted_args; /* should we use formatted args instead of the static provided args */
} lib_lte_cmd_type_E;

/* Command components */
extern const char LIB_LTE_CMD_HEADER[];
extern const char LIB_LTE_CMD_FOOTER[];
extern const char LIB_LTE_CMD_PADDING[];
extern const char LIB_LTE_CMD_QUERY[];
extern const char LIB_LTE_CMD_ARGS_ASSIGNMENT[];
extern const char LIB_LTE_CMD_RESP_OK[];
extern const char LIB_LTE_CMD_RESP_ERROR[];

/* Command strings */
extern const char LIB_LTE_CMD_RESET_STRING[];
extern const char LIB_LTE_CMD_SIM_STATUS_STRING[];
extern const char LIB_LTE_CMD_RSSI_STRING[];
extern const char LIB_LTE_CMD_RADIO_STRING[];
extern const char LIB_LTE_CMD_APN_STRING[];
extern const char LIB_LTE_CMD_NETWORK_STRING[];
extern const char LIB_LTE_HTTP_CONFIG_STRING[];
extern const char LIB_LTE_HTTP_CONNECT_STRING[];
extern const char LIB_LTE_HTTP_STATE_STRING[];
extern const char LIB_LTE_HTTP_CLEAR_HDR_STRING[];
extern const char LIB_LTE_HTTP_CLEAR_BDY_STRING[];
extern const char LIB_LTE_HTTP_WRITE_HDR_STRING[];
extern const char LIB_LTE_HTTP_WRITE_BDY_STRING[];
extern const char LIB_LTE_HTTP_POST_STRING[];
extern const char LIB_LTE_HTTP_READ_RESPONSE_STRING[];
extern const char LIB_LTE_HTTP_DISCONNECT_STRING[];
extern const char LIB_LTE_GPS_PWR_STRING[];
extern const char LIB_LTE_GPS_DATA_STRING[];
extern const char LIB_LTE_CMD_ECHO_OFF[];

/* Command arg strings */
extern const char LIB_LTE_RESET_ARGS_STRING[];
extern const char LIB_LTE_RADIO_ON_ARGS_STRING[];
extern const char LIB_LTE_RADIO_OFF_ARGS_STRING[];
extern const char LIB_LTE_TELUS_APN_ARGS_STRING[];
extern const char LIB_LTE_NETWORK_ATTACH_ARGS_STRING[];
extern const char LIB_LTE_NETWORK_DETACH_ARGS_STRING[];
extern const char LIB_LTE_HTTP_POST_ARGS_STRING[];
extern const char LIB_LTE_GPS_ON_ARGS_STRING[];
extern const char LIB_LTE_GPS_OFF_ARGS_STRING[];

/* Command response strings */
extern const char LIB_LTE_SIM_STATUS_RESPONSE_STRING[];
extern const char LIB_LTE_CMD_RSSI_RESPONSE_STRING[];
extern const char LIB_LTE_CMD_NETWORK_ATTACH_RESPONSE_STRING[];
extern const char LIB_LTE_CMD_NETWORK_DETACH_RESPONSE_STRING[];
extern const char LIB_LTE_CMD_HTTP_CONNECTED_RESPONSE_STRING[];
extern const char LIB_LTE_CMD_HTTP_POST_RESPONSE_STRING[];
extern const char LIB_LTE_CMD_HTTP_READ_DATA_RESPONSE_STRING[];
extern const char LIB_LTE_GPS_DATA_RESPONSE_STRING[];

/* Commands */

/* status commands */
extern const lib_lte_cmd_type_E LIB_LTE_RESET_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_SIM_STATUS_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_RSSI_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_ECHO_OFF_CMD;

/* network commands */
extern const lib_lte_cmd_type_E LIB_LTE_RADIO_OFF_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_RADIO_ON_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_SET_APN_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_NETWORK_ATTACH_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_NETWORK_DETACH_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_NETWORK_LOCAL_IP_CMD;

/* HTTP commands */
extern const lib_lte_cmd_type_E LIB_LTE_HTTP_CONFIGURE_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_HTTP_CONN_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_HTTP_CHECK_CONN_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_HTTP_CLEAR_HEADER_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_HTTP_CLEAR_BODY_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_HTTP_WRITE_TO_HEADER_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_HTTP_WRITE_TO_BODY_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_HTTP_WRITE_TO_BODY_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_HTTP_POST_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_HTTP_READ_RESP_CMD;
extern const lib_lte_cmd_type_E LIB_LTE_HTTP_DISCONNECT_CMD;

/* GPS commands */
extern const lib_lte_cmd_type_E LIT_LTE_GPS_POWER_ON_CMD;
extern const lib_lte_cmd_type_E LIT_LTE_GPS_POWER_OFF_CMD;
extern const lib_lte_cmd_type_E LIT_LTE_GPS_GET_LOCATION_CMD;

#endif /* LIB_LTE_CMD_H_ */
