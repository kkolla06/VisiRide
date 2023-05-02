/**
 * @file lib_lte_cmd.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-02-23
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "FreeRTOS.h"
#include "system.h"
#include "lib_lte_cmd.h"

/* Command components */
const char LIB_LTE_CMD_HEADER[] = {"AT"};
const char LIB_LTE_CMD_FOOTER[] = {"\r"};
const char LIB_LTE_CMD_PADDING[] = {"\r\n"};
const char LIB_LTE_CMD_QUERY[] = {"?"};
const char LIB_LTE_CMD_ARGS_ASSIGNMENT[] = {"="};
const char LIB_LTE_CMD_RESP_OK[] = {"\r\nOK\r\n"};
const char LIB_LTE_CMD_RESP_ERROR[] = {"\r\nERROR\r\n"};

/* Command strings */
const char LIB_LTE_CMD_RESET_STRING[] = {"+CFUN"};
const char LIB_LTE_CMD_SIM_STATUS_STRING[] = {"+CPIN"};
const char LIB_LTE_CMD_RSSI_STRING[] = {"+CSQ"};
const char LIB_LTE_CMD_RADIO_STRING[] = {"+CFUN"};
const char LIB_LTE_CMD_APN_STRING[] = {"+CNCFG"};
const char LIB_LTE_CMD_NETWORK_STRING[] = {"+CNACT"};
const char LIB_LTE_HTTP_CONFIG_STRING[] = {"+SHCONF"};
const char LIB_LTE_HTTP_CONNECT_STRING[] = {"+SHCONN"};
const char LIB_LTE_HTTP_STATE_STRING[] = {"+SHSTATE"};
const char LIB_LTE_HTTP_CLEAR_HDR_STRING[] = {"+SHCHEAD"};
const char LIB_LTE_HTTP_CLEAR_BDY_STRING[] = {"+SHCPARA"};
const char LIB_LTE_HTTP_WRITE_HDR_STRING[] = {"+SHAHEAD"};
const char LIB_LTE_HTTP_WRITE_BDY_STRING[] = {"+SHPARA"};
const char LIB_LTE_HTTP_POST_STRING[] = {"+SHREQ"};
const char LIB_LTE_HTTP_READ_RESPONSE_STRING[] = {"+SHREAD"};
const char LIB_LTE_HTTP_DISCONNECT_STRING[] = {"+SHDISC"};
const char LIB_LTE_GPS_PWR_STRING[] = {"+CGNSPWR"};
const char LIB_LTE_GPS_DATA_STRING[] = {"+CGNSINF"};
const char LIB_LTE_CMD_ECHO_OFF[] = {"E0"};

/* Command arg strings */
const char LIB_LTE_RESET_ARGS_STRING[] = {"1,1"};
const char LIB_LTE_RADIO_ON_ARGS_STRING[] = {"1"};
const char LIB_LTE_RADIO_OFF_ARGS_STRING[] = {"0"};
const char LIB_LTE_TELUS_APN_ARGS_STRING[] = {"0,1,\"m2m.telus.iot\""};
const char LIB_LTE_NETWORK_ATTACH_ARGS_STRING[] = {"0,1"};
const char LIB_LTE_NETWORK_DETACH_ARGS_STRING[] = {"0,0"};
const char LIB_LTE_HTTP_POST_ARGS_STRING[] = {"\"/checkFace\",3"};
const char LIB_LTE_GPS_ON_ARGS_STRING[] = {"1"};
const char LIB_LTE_GPS_OFF_ARGS_STRING[] = {"0"};

/* Command response strings */
const char LIB_LTE_SIM_STATUS_RESPONSE_STRING[] = {"READY"};
const char LIB_LTE_CMD_RSSI_RESPONSE_STRING[] = {": %d,%d\r\n%*[^0123456789]"};
const char LIB_LTE_CMD_NETWORK_ATTACH_RESPONSE_STRING[] = {"+APP PDP: 0,ACTIVE"};
const char LIB_LTE_CMD_NETWORK_DETACH_RESPONSE_STRING[] = {"+APP PDP: 0,DEACTIVE"};
const char LIB_LTE_CMD_HTTP_CONNECTED_RESPONSE_STRING[] = {"1"};
const char LIB_LTE_CMD_HTTP_POST_RESPONSE_STRING[] = {": \"POST\",%3s,%3s"};
const char LIB_LTE_CMD_HTTP_READ_DATA_RESPONSE_STRING[] = {"*\r\n"}; /* STOP character from server */
const char LIB_LTE_GPS_DATA_RESPONSE_STRING[] = {": 1,1,%18s,%9s,%11s,%*[^0123456789]"};

/* Command definitions */

/* status commands */
const lib_lte_cmd_type_E LIB_LTE_RESET_CMD = {.cmd = LIB_LTE_CMD_RESET_STRING,
                                            .cmd_len = sizeof(LIB_LTE_CMD_RESET_STRING),
                                            .cmd_args = LIB_LTE_RESET_ARGS_STRING,
                                            .args_len = sizeof(LIB_LTE_RESET_ARGS_STRING),
                                            .response_str = NULL,
                                            .resp_len = 0,
                                            .is_query = false,
                                            .resp_type = LIB_LTE_RESP_TYPE_BASIC,
                                            .formatted_args = false};

const lib_lte_cmd_type_E LIB_LTE_SIM_STATUS_CMD = {.cmd = LIB_LTE_CMD_SIM_STATUS_STRING,
                                                .cmd_len = sizeof(LIB_LTE_CMD_SIM_STATUS_STRING),
                                                .cmd_args = NULL,
                                                .args_len = 0,
                                                .response_str = LIB_LTE_SIM_STATUS_RESPONSE_STRING,
                                                .resp_len = sizeof(LIB_LTE_SIM_STATUS_RESPONSE_STRING),
                                                .is_query = true,
                                                .resp_type = LIB_LTE_RESP_TYPE_STRING,
                                                .formatted_args = false};

const lib_lte_cmd_type_E LIB_LTE_RSSI_CMD = {.cmd = LIB_LTE_CMD_RSSI_STRING,
                                            .cmd_len = sizeof(LIB_LTE_CMD_RSSI_STRING),
                                            .cmd_args = NULL,
                                            .args_len = 0,
                                            .response_str = NULL,
                                            .resp_len = 0,
                                            .is_query = false,
                                            .resp_type = LIB_LTE_RESP_TYPE_STRING,
                                            .formatted_args = false};


const lib_lte_cmd_type_E LIB_LTE_ECHO_OFF_CMD = {.cmd = LIB_LTE_CMD_ECHO_OFF,
                                            .cmd_len = sizeof(LIB_LTE_CMD_ECHO_OFF),
                                            .cmd_args = NULL,
                                            .args_len = 0,
                                            .response_str = NULL,
                                            .resp_len = 0,
                                            .is_query = false,
                                            .resp_type = LIB_LTE_RESP_TYPE_BASIC,
                                            .formatted_args = false};


/* network commands */
const lib_lte_cmd_type_E LIB_LTE_RADIO_OFF_CMD = {.cmd = LIB_LTE_CMD_RADIO_STRING,
                                                .cmd_len = sizeof(LIB_LTE_CMD_RADIO_STRING),
                                                .cmd_args = LIB_LTE_RADIO_OFF_ARGS_STRING,
                                                .args_len = sizeof(LIB_LTE_RADIO_OFF_ARGS_STRING),
                                                .response_str = NULL,
                                                .resp_len = 0,
                                                .is_query = false,
                                                .resp_type = LIB_LTE_RESP_TYPE_BASIC,
                                                .formatted_args = false};

const lib_lte_cmd_type_E LIB_LTE_RADIO_ON_CMD = {.cmd = LIB_LTE_CMD_RADIO_STRING,
                                                .cmd_len = sizeof(LIB_LTE_CMD_RADIO_STRING),
                                                .cmd_args = LIB_LTE_RADIO_ON_ARGS_STRING,
                                                .args_len = sizeof(LIB_LTE_RADIO_ON_ARGS_STRING),
                                                .response_str = NULL,
                                                .resp_len = 0,
                                                .is_query = false,
                                                .resp_type = LIB_LTE_RESP_TYPE_BASIC,
                                                .formatted_args = false};

const lib_lte_cmd_type_E LIB_LTE_SET_APN_CMD = {.cmd = LIB_LTE_CMD_APN_STRING,
                                                .cmd_len = sizeof(LIB_LTE_CMD_APN_STRING),
                                                .cmd_args = LIB_LTE_TELUS_APN_ARGS_STRING,
                                                .args_len = sizeof(LIB_LTE_TELUS_APN_ARGS_STRING),
                                                .response_str = NULL,
                                                .resp_len = 0,
                                                .is_query = false,
                                                .resp_type = LIB_LTE_RESP_TYPE_BASIC,
                                                .formatted_args = false};

const lib_lte_cmd_type_E LIB_LTE_NETWORK_ATTACH_CMD = {.cmd = LIB_LTE_CMD_NETWORK_STRING,
                                                        .cmd_len = sizeof(LIB_LTE_CMD_NETWORK_STRING),
                                                        .cmd_args = LIB_LTE_NETWORK_ATTACH_ARGS_STRING,
                                                        .args_len = sizeof(LIB_LTE_NETWORK_ATTACH_ARGS_STRING),
                                                        .response_str = LIB_LTE_CMD_NETWORK_ATTACH_RESPONSE_STRING,
                                                        .resp_len = sizeof(LIB_LTE_CMD_NETWORK_ATTACH_RESPONSE_STRING),
                                                        .is_query = false,
                                                        .resp_type = LIB_LTE_RESP_TYPE_ASYNC,
                                                        .formatted_args = false};

const lib_lte_cmd_type_E LIB_LTE_NETWORK_DETACH_CMD = {.cmd = LIB_LTE_CMD_NETWORK_STRING,
                                                        .cmd_len = sizeof(LIB_LTE_CMD_NETWORK_STRING),
                                                        .cmd_args = LIB_LTE_NETWORK_DETACH_ARGS_STRING,
                                                        .args_len = sizeof(LIB_LTE_NETWORK_DETACH_ARGS_STRING),
                                                        .response_str = LIB_LTE_CMD_NETWORK_DETACH_RESPONSE_STRING,
                                                        .resp_len = sizeof(LIB_LTE_CMD_NETWORK_DETACH_RESPONSE_STRING),
                                                        .is_query = false,
                                                        .resp_type = LIB_LTE_RESP_TYPE_ASYNC,
                                                        .formatted_args = false};

const lib_lte_cmd_type_E LIB_LTE_NETWORK_LOCAL_IP_CMD = {.cmd = LIB_LTE_CMD_NETWORK_STRING,
                                                        .cmd_len = sizeof(LIB_LTE_CMD_NETWORK_STRING),
                                                        .cmd_args = NULL,
                                                        .args_len = 0,
                                                        .response_str = NULL,
                                                        .resp_len = 0,
                                                        .is_query = true,
                                                        .resp_type = LIB_LTE_RESP_TYPE_STRING,
                                                        .formatted_args = false};

/* HTTP commands */
const lib_lte_cmd_type_E LIB_LTE_HTTP_CONFIGURE_CMD = {.cmd = LIB_LTE_HTTP_CONFIG_STRING,
                                                    .cmd_len = sizeof(LIB_LTE_HTTP_CONFIG_STRING),
                                                    .cmd_args = NULL,
                                                    .args_len = 0,
                                                    .response_str = NULL,
                                                    .resp_len = 0,
                                                    .is_query = false,
                                                    .resp_type = LIB_LTE_RESP_TYPE_BASIC,
                                                    .formatted_args = true};

const lib_lte_cmd_type_E LIB_LTE_HTTP_CONN_CMD = {.cmd = LIB_LTE_HTTP_CONNECT_STRING,
                                                    .cmd_len = sizeof(LIB_LTE_HTTP_CONNECT_STRING),
                                                    .cmd_args = NULL,
                                                    .args_len = 0,
                                                    .response_str = NULL,
                                                    .resp_len = 0,
                                                    .is_query = false,
                                                    .resp_type = LIB_LTE_RESP_TYPE_BASIC,
                                                    .formatted_args = false};

const lib_lte_cmd_type_E LIB_LTE_HTTP_CHECK_CONN_CMD = {.cmd = LIB_LTE_HTTP_STATE_STRING,
                                                        .cmd_len = sizeof(LIB_LTE_HTTP_STATE_STRING),
                                                        .cmd_args = NULL,
                                                        .args_len = 0,
                                                        .response_str = LIB_LTE_CMD_HTTP_CONNECTED_RESPONSE_STRING,
                                                        .resp_len = sizeof(LIB_LTE_CMD_HTTP_CONNECTED_RESPONSE_STRING),
                                                        .is_query = true,
                                                        .resp_type = LIB_LTE_RESP_TYPE_STRING,
                                                        .formatted_args = false};

const lib_lte_cmd_type_E LIB_LTE_HTTP_CLEAR_HEADER_CMD = {.cmd = LIB_LTE_HTTP_CLEAR_HDR_STRING,
                                                    .cmd_len = sizeof(LIB_LTE_HTTP_CLEAR_HDR_STRING),
                                                    .cmd_args = NULL,
                                                    .args_len = 0,
                                                    .response_str = NULL,
                                                    .resp_len = 0,
                                                    .is_query = false,
                                                    .resp_type = LIB_LTE_RESP_TYPE_BASIC,
                                                    .formatted_args = false};

const lib_lte_cmd_type_E LIB_LTE_HTTP_CLEAR_BODY_CMD = {.cmd = LIB_LTE_HTTP_CLEAR_BDY_STRING,
                                                    .cmd_len = sizeof(LIB_LTE_HTTP_CLEAR_BDY_STRING),
                                                    .cmd_args = NULL,
                                                    .args_len = 0,
                                                    .response_str = NULL,
                                                    .resp_len = 0,
                                                    .is_query = false,
                                                    .resp_type = LIB_LTE_RESP_TYPE_BASIC,
                                                    .formatted_args = false};

const lib_lte_cmd_type_E LIB_LTE_HTTP_WRITE_TO_HEADER_CMD = {.cmd = LIB_LTE_HTTP_WRITE_HDR_STRING,
                                                    .cmd_len = sizeof(LIB_LTE_HTTP_WRITE_HDR_STRING),
                                                    .cmd_args = NULL,
                                                    .args_len = 0,
                                                    .response_str = NULL,
                                                    .resp_len = 0,
                                                    .is_query = false,
                                                    .resp_type = LIB_LTE_RESP_TYPE_BASIC,
                                                    .formatted_args = true};

const lib_lte_cmd_type_E LIB_LTE_HTTP_WRITE_TO_BODY_CMD = {.cmd = LIB_LTE_HTTP_WRITE_BDY_STRING,
                                                        .cmd_len = sizeof(LIB_LTE_HTTP_WRITE_BDY_STRING),
                                                        .cmd_args = NULL,
                                                        .args_len = 0,
                                                        .response_str = NULL,
                                                        .resp_len = 0,
                                                        .is_query = false,
                                                        .resp_type = LIB_LTE_RESP_TYPE_BASIC,
                                                        .formatted_args = true};

const lib_lte_cmd_type_E LIB_LTE_HTTP_POST_CMD = {.cmd = LIB_LTE_HTTP_POST_STRING,
                                                    .cmd_len = sizeof(LIB_LTE_HTTP_POST_STRING),
                                                    .cmd_args = NULL,
                                                    .args_len = 0,
                                                    .response_str = LIB_LTE_CMD_HTTP_POST_RESPONSE_STRING,
                                                    .resp_len = sizeof(LIB_LTE_CMD_HTTP_POST_RESPONSE_STRING),
                                                    .is_query = false,
                                                    .resp_type = LIB_LTE_RESP_TYPE_STRING,
                                                    .formatted_args = true};

const lib_lte_cmd_type_E LIB_LTE_HTTP_READ_RESP_CMD = {.cmd = LIB_LTE_HTTP_READ_RESPONSE_STRING,
                                                    .cmd_len = sizeof(LIB_LTE_HTTP_READ_RESPONSE_STRING),
                                                    .cmd_args = NULL,
                                                    .args_len = 0,
                                                    .response_str = LIB_LTE_CMD_HTTP_READ_DATA_RESPONSE_STRING,
                                                    .resp_len = sizeof(LIB_LTE_CMD_HTTP_READ_DATA_RESPONSE_STRING),
                                                    .is_query = false,
                                                    .resp_type = LIB_LTE_RESP_TYPE_ASYNC,
                                                    .formatted_args = true};

const lib_lte_cmd_type_E LIB_LTE_HTTP_DISCONNECT_CMD = {.cmd = LIB_LTE_HTTP_DISCONNECT_STRING,
                                                    .cmd_len = sizeof(LIB_LTE_HTTP_DISCONNECT_STRING),
                                                    .cmd_args = NULL,
                                                    .args_len = 0,
                                                    .response_str = NULL,
                                                    .resp_len = 0,
                                                    .is_query = false,
                                                    .resp_type = LIB_LTE_RESP_TYPE_BASIC,
                                                    .formatted_args = false};

const lib_lte_cmd_type_E LIT_LTE_GPS_POWER_ON_CMD = {.cmd = LIB_LTE_GPS_PWR_STRING,
                                                    .cmd_len = sizeof(LIB_LTE_GPS_PWR_STRING),
                                                    .cmd_args = LIB_LTE_GPS_ON_ARGS_STRING,
                                                    .args_len = sizeof(LIB_LTE_GPS_ON_ARGS_STRING),
                                                    .response_str = NULL,
                                                    .resp_len = 0,
                                                    .is_query = false,
                                                    .resp_type = LIB_LTE_RESP_TYPE_BASIC,
                                                    .formatted_args = false};

const lib_lte_cmd_type_E LIT_LTE_GPS_POWER_OFF_CMD = {.cmd = LIB_LTE_GPS_PWR_STRING,
                                                    .cmd_len = sizeof(LIB_LTE_GPS_PWR_STRING),
                                                    .cmd_args = LIB_LTE_GPS_OFF_ARGS_STRING,
                                                    .args_len = sizeof(LIB_LTE_GPS_OFF_ARGS_STRING),
                                                    .response_str = NULL,
                                                    .resp_len = 0,
                                                    .is_query = false,
                                                    .resp_type = LIB_LTE_RESP_TYPE_BASIC,
                                                    .formatted_args = false};

const lib_lte_cmd_type_E LIT_LTE_GPS_GET_LOCATION_CMD = {.cmd = LIB_LTE_GPS_DATA_STRING,
                                                    .cmd_len = sizeof(LIB_LTE_GPS_DATA_STRING),
                                                    .cmd_args = NULL,
                                                    .args_len = 0,
                                                    .response_str = NULL,
                                                    .resp_len = 0,
                                                    .is_query = false,
                                                    .resp_type = LIB_LTE_RESP_TYPE_STRING,
                                                    .formatted_args = false};
