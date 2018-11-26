/*
 * Copyright (C) 2018 Gunar Schorcht
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */
 /**
 * @ingroup     tests
 * @brief       Test application for the Sensirion SHT30/SHT31/SHT35 device driver
 * @author      Gunar Schorcht <gunar@schorcht.net>
 * @file
 * @{
 * @}
 */
#include <stdio.h>
#include <string.h>
#include "xtimer.h"
#include "sht31.h"
#include "sht31_params.h" 
#include "thread.h"
#include "shell.h"
#include "shell_commands.h" 
#include "modem.h" 

#define INTERVAL (20U * US_PER_SEC)

void on_modem_command_completed_callback(bool with_error) 
{
    printf("modem command completed (success = %i)\n", !with_error);
} 

void on_modem_return_file_data_callback(uint8_t file_id, uint32_t offset, uint32_t size, uint8_t* output_buffer)
{
    printf("modem return file data file %i offset %li size %li buffer %p\n", file_id, offset, size, output_buffer);
}

void on_modem_write_file_data_callback(uint8_t file_id, uint32_t offset, uint32_t size, uint8_t* output_buffer)
{
    printf("modem write file data file %i offset %li size %li buffer %p\n", file_id, offset, size, output_buffer);
}

/* Origineel: SESSION_RESP_MODE_PREFERED 
*/
static d7ap_session_config_t d7_session_config = {
    .qos = {
        .qos_resp_mode = SESSION_RESP_MODE_ALL,  
        .qos_retry_mode = SESSION_RETRY_MODE_NO
    },
    .dormant_timeout = 0,
    .addressee = {
        .ctrl = {
            .nls_method = AES_NONE,
            .id_type = ID_TYPE_NOID
        },
        .access_class = 0x01,
        .id = {0},
    },
}; 

 int main(void)
{
    puts("Welcome to RIOT!");
    sht31_dev_t dev; 

    modem_callbacks_t modem_callbacks = {
        .command_completed_callback = &on_modem_command_completed_callback,
        .return_file_data_callback = &on_modem_return_file_data_callback,
        .write_file_data_callback = &on_modem_write_file_data_callback,
    }; 

    modem_init(UART_DEV(1), &modem_callbacks);

    uint8_t uid[D7A_FILE_UID_SIZE];
    modem_read_file(D7A_FILE_UID_FILE_ID, 0, D7A_FILE_UID_SIZE, uid);
    printf("modem UID: %02X%02X%02X%02X%02X%02X%02X%02X\n", uid[0], uid[1], uid[2], uid[3], uid[4], uid[5], uid[6], uid[7]);
    
    xtimer_ticks32_t last_wakeup = xtimer_now();
    alp_itf_id_t current_interface_id = ALP_ITF_ID_D7ASP;
    void* current_interface_config = (void*)&d7_session_config;
    //added this 
	printf("Switching to D7AP\n");
    current_interface_id = ALP_ITF_ID_D7ASP;
	current_interface_config = &d7_session_config;

    int res;
     puts("SHT31 test application using dash7\n");
     printf("+------------Initializing------------+\n");
     if ((res = sht31_init(&dev, &sht31_params[0])) != SHT31_OK) {
        puts("Initialization failed\n");
        return 1;
    }
    else {
        puts("Initialization successful\n");
    }
     printf("\n+--------Starting Measurements--------+\n");
     uint8_t array[2];
     while (1) {
        int16_t temp;
        int16_t hum;
         if ((res = sht31_read(&dev, &temp, &hum)) == SHT31_OK) {
            // printf("Temperature [°C]: %d.%d\n"
            //        "Relative Humidity [%%]: %d.%d\n"
            //        "+-------------------------------------+\n",
            //        temp / 100, temp % 100,
            //        hum / 100, hum % 100); 
            printf("Temperature [°C]: %d.%d\n"
                   ,temp / 100, temp % 100
                  ); 
            printf("Sending msg in dash7"); 
            array[0] = (temp/100);
            array[1] = (temp%100);
            uint32_t start = xtimer_now_usec(); 
            modem_status_t status = modem_send_unsolicited_response(0x56, 0, 2, array, current_interface_id, current_interface_config);
            uint32_t duration_usec = xtimer_now_usec() - start;
            printf("Command completed in %li ms\n", duration_usec / 1000);
            if(status == MODEM_STATUS_COMMAND_COMPLETED_SUCCESS) {
                printf("Command completed successfully\n");
            } else if(status == MODEM_STATUS_COMMAND_COMPLETED_ERROR) {
                printf("Command completed with error\n");
            } else if(status == MODEM_STATUS_COMMAND_TIMEOUT) {
                printf("Command timed out\n");
            }
            xtimer_periodic_wakeup(&last_wakeup, INTERVAL);
            printf("slept until %" PRIu32 "\n", xtimer_usec_from_ticks(xtimer_now()));
        }
        else {
            printf("Could not read data from sensor, error %d\n", res);
        }
        xtimer_usleep(1000000);
    }
     return 0;
}