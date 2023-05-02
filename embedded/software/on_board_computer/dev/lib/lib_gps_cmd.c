/**
 * @file lib_gps_cmd.c
 * @author Emery Nagy
 * @brief
 * @version 0.1
 * @date 2023-02-23
 *
 */

#include "FreeRTOS.h"
#include "system.h"
#include "lib_gps_cmd.h"

/* Command components */
const char LIB_GPS_CMD_HEADER[] = {"AT"};
const char LIB_GPS_CMD_FOOTER[] = {"\r"};
const char LIB_GPS_CMD_PADDING[] = {"\r\n"};
const char LIB_GPS_CMD_QUERY[] = {"?"};
const char LIB_GPS_CMD_ARGS_ASSIGNMENT[] = {"="};
const char LIB_GPS_CMD_RESP_OK[] = {"\r\nOK\r\n"};
const char LIB_GPS_CMD_RESP_ERROR[] = {"\r\nERROR\r\n"};

/* Command strings */
const char LIB_GPS_CMD_RESET_STRING[] = {"+CFUN"};
const char LIB_GPS_CMD_SIM_STATUS_STRING[] = {"+CPIN"};
const char LIB_GPS_PWR_STRING[] = {"+CGNSPWR"};
const char LIB_GPS_DATA_STRING[] = {"+CGNSINF"};

/* Command arg strings */
const char LIB_GPS_RESET_ARGS_STRING[] = {"1,1"};
const char LIB_GPS_ON_ARGS_STRING[] = {"1"};
const char LIB_GPS_OFF_ARGS_STRING[] = {"0"};

/* Command response strings */
const char LIB_GPS_SIM_STATUS_RESPONSE_STRING[] = {"READY"};
const char LIB_GPS_DATA_RESPONSE_STRING[] = {": 1,1,%f,%f,%f,%*[^0123456789]"};

/* Command definitions */

/* status commands */
const lib_gps_cmd_type_E LIB_GPS_RESET_CMD = {.cmd = LIB_GPS_CMD_RESET_STRING,
                                            .cmd_len = sizeof(LIB_GPS_CMD_RESET_STRING),
                                            .cmd_args = LIB_GPS_RESET_ARGS_STRING,
                                            .args_len = sizeof(LIB_GPS_RESET_ARGS_STRING),
                                            .response_str = NULL,
                                            .resp_len = 0,
                                            .is_query = false,
                                            .resp_type = LIB_GPS_RESP_TYPE_BASIC,
                                            .formatted_args = false};

const lib_gps_cmd_type_E LIB_GPS_SIM_STATUS_CMD = {.cmd = LIB_GPS_CMD_SIM_STATUS_STRING,
                                                .cmd_len = sizeof(LIB_GPS_CMD_SIM_STATUS_STRING),
                                                .cmd_args = NULL,
                                                .args_len = 0,
                                                .response_str = LIB_GPS_SIM_STATUS_RESPONSE_STRING,
                                                .resp_len = sizeof(LIB_GPS_SIM_STATUS_RESPONSE_STRING),
                                                .is_query = true,
                                                .resp_type = LIB_GPS_RESP_TYPE_STRING,
                                                .formatted_args = false};

const lib_gps_cmd_type_E LIB_GPS_POWER_ON_CMD = {.cmd = LIB_GPS_PWR_STRING,
                                                    .cmd_len = sizeof(LIB_GPS_PWR_STRING),
                                                    .cmd_args = LIB_GPS_ON_ARGS_STRING,
                                                    .args_len = sizeof(LIB_GPS_ON_ARGS_STRING),
                                                    .response_str = NULL,
                                                    .resp_len = 0,
                                                    .is_query = false,
                                                    .resp_type = LIB_GPS_RESP_TYPE_BASIC,
                                                    .formatted_args = false};

const lib_gps_cmd_type_E LIB_GPS_POWER_OFF_CMD = {.cmd = LIB_GPS_PWR_STRING,
                                                    .cmd_len = sizeof(LIB_GPS_PWR_STRING),
                                                    .cmd_args = LIB_GPS_OFF_ARGS_STRING,
                                                    .args_len = sizeof(LIB_GPS_OFF_ARGS_STRING),
                                                    .response_str = NULL,
                                                    .resp_len = 0,
                                                    .is_query = false,
                                                    .resp_type = LIB_GPS_RESP_TYPE_BASIC,
                                                    .formatted_args = false};

const lib_gps_cmd_type_E LIB_GPS_GET_LOCATION_CMD = {.cmd = LIB_GPS_DATA_STRING,
                                                    .cmd_len = sizeof(LIB_GPS_DATA_STRING),
                                                    .cmd_args = NULL,
                                                    .args_len = 0,
                                                    .response_str = NULL,
                                                    .resp_len = 0,
                                                    .is_query = false,
                                                    .resp_type = LIB_GPS_RESP_TYPE_STRING,
                                                    .formatted_args = false};
