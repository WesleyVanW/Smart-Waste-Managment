/*
 * Copyright (C) 2018 Gunar Schorcht
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */
 /**
 * @ingroup     tests
 * @brief       Test application for Smart Waste Managment
 * @authors      Mouhcine Oulad Ali, Gregory Gonzalez Lopez, Wesley Van Wijnsberghe
 * @file
 * @{
 * @}
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "xtimer.h"
#include "sht31.h"
#include "sht31_params.h" 
#include "lsm303agr.h"
#include "lsm303agr_params.h"
#include "thread.h"
#include "shell.h"
#include "shell_commands.h" 
#include "modem.h" 
#include "timex.h"
#include "periph/gpio.h"
#include "periph_conf.h"
#include "periph/pm.h"
#include "srf04_params.h"
#include "srf04.h"
#include "getCoo.h"

#define INTERVAL (20U * US_PER_SEC) //20 seconds

#define LORAWAN_NETW_SESSION_KEY  { 0xC6, 0xA0, 0x55, 0x22, 0x4F, 0x25, 0xD2, 0x77, 0x9C, 0x83, 0x5C, 0x15, 0x38, 0xBC, 0x7A, 0x9D }
#define LORAWAN_APP_SESSION_KEY  { 0x2F, 0xDC, 0x45, 0x93, 0x0B, 0x5E, 0x44, 0x54, 0x7B, 0xB1, 0xC1, 0x73, 0x4C, 0x9B, 0x16, 0x3D }
#define LORAWAN_DEV_ADDR 0x26011921
#define LORAWAN_NETW_ID 0x000000

static int mode;                                                                    //0=sleep, 1=active

void on_modem_command_completed_callback(bool with_error)  {
    printf("modem command completed (success = %i)\n", !with_error);
} 

void on_modem_return_file_data_callback(uint8_t file_id, uint32_t offset, uint32_t size, uint8_t* output_buffer) {
    printf("modem return file data file %i offset %li size %li buffer %p\n", file_id, offset, size, output_buffer);
}

void on_modem_write_file_data_callback(uint8_t file_id, uint32_t offset, uint32_t size, uint8_t* output_buffer) {
    printf("modem write file data file %i offset %li size %li buffer %p\n", file_id, offset, size, output_buffer);
}

static void acc_callback(void *arg) {
    if (arg == NULL) {

    }
    if(mode != 1) {
        puts("Motion detected - switching to active mode");
        mode = 1; 
    }
}

static d7ap_session_config_t d7_session_config = {
    .qos = {
        .qos_resp_mode = SESSION_RESP_MODE_ALL,   // Origineel: SESSION_RESP_MODE_PREFERED 
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

static lorawan_session_config_abp_t lorawan_session_config = {
    .appSKey = LORAWAN_APP_SESSION_KEY,
    .nwkSKey = LORAWAN_NETW_SESSION_KEY,
    .devAddr = LORAWAN_DEV_ADDR,
    .request_ack = false,
    .network_id = LORAWAN_NETW_ID,
    .application_port = 1
};

int main(void) {
    puts("Welcome to RIOT! - Smart Waste Managment");
    sht31_dev_t dev; 
    lsm303agr_t dev2; 
    srf04_t dev3;
    int res;

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

    puts("Smart Waste Managment Application"); 
    printf("+------------Initial configuration: D7 mode------------+\n");
    alp_itf_id_t current_interface_id = ALP_ITF_ID_D7ASP;
    void* current_interface_config = (void*)&d7_session_config;
    
    printf("+------------Initializing Sensors------------+\n");
    if ((res = sht31_init(&dev, &sht31_params[0])) != SHT31_OK) {
        puts("Initialization failed\n");
        return 1;
    }
    else {
        puts("SHT31 initialization successful\n");
    } 
    
    if(lsm303agr_init(&dev2, &lsm303agr_params[0]) == 0) {
        puts("lsm303agr initialization successful\n");
    }
    else { 
        puts("lsm303agr initialization failed\n");
        return 1;
    } 

    if(lsm303agr_enable_interrupt(&dev2) != 0)
    {
        puts("lsm303agr interrupt not enabled");
    } 

    if (srf04_init(&dev3, &srf04_params[0]) != SRF04_OK) {
        puts("srf04 initialization failed\n");
        return 1;
    }
   
    gpio_init_int(GPIO_PIN(PORT_B,13), GPIO_IN , GPIO_RISING, acc_callback, (void*) 0);
    gpio_irq_enable(GPIO_PIN(PORT_B,13));
    
    printf("\n-------Initialization fully completed & succesfull--------\n");
    
    uint8_t array[14];
    uint8_t counter = 0; 
        
    mode = 1; 

    int distance = 0;
    float lati; 
    float longi;
    bool connectionFailed = false;
    int length = 0; 

    while (1) {
        int16_t temp;
        int16_t hum; 
        lsm303agr_3d_data_t acc_value; 

        if (mode) {
            uint32_t start = xtimer_now_usec();
            
            //Only use LoRa if no d7 connection
            if(current_interface_id == ALP_ITF_ID_D7ASP && connectionFailed) {
                printf("\nSwitching to LoRaWAN mode\n");
                current_interface_id = ALP_ITF_ID_LORAWAN_ABP;
                current_interface_config = &lorawan_session_config;
            } 
            else if (current_interface_id == ALP_ITF_ID_LORAWAN_ABP) {
                printf("\nSwitching to D7 mode\n");
                current_interface_id = ALP_ITF_ID_D7ASP;
                current_interface_config = &d7_session_config;
            }
            
            //Use LoRa fist then after D7
            // if(current_interface_id == ALP_ITF_ID_D7ASP) {
            //     printf("\nSwitching to LoRaWAN mode\n");
            //     current_interface_id = ALP_ITF_ID_LORAWAN_ABP;
            //     current_interface_config = &lorawan_session_config;
            // } 
            // else {
            //     printf("\nSwitching to D7 mode\n");
            //     current_interface_id = ALP_ITF_ID_D7ASP;
            //     current_interface_config = &d7_session_config;
            // }

            //Measuring payload
            if(counter == 0) {
                // Measuring temperature
                if ((res = sht31_read(&dev, &temp, &hum)) == SHT31_OK) {
                    printf("Temperature [°C]: %d.%d\n",temp / 100, temp % 100);  
                    array[0] = (temp/100);
                    array[1] = (temp%100);
                }
                else {
                    printf("\nCould not read data from SHT31 sensor - Temperature, error %d\n", res);
                } 

                //Measuring accelerometer 
                if (lsm303agr_read_acc(&dev2, &acc_value) == 0) {
                    printf("Accelerometer x: %i y: %i z: %i\n", acc_value.x_axis, acc_value.y_axis, acc_value.z_axis);
                    array[2] = acc_value.x_axis; 
                    array[3] = acc_value.y_axis; 
                    array[4] = acc_value.z_axis;  
                }
                else {
                    puts("\nCould not read data from lsm303agr sensor - Accelerometer\n");
                }

                //Measuring distance
                distance = srf04_get_distance(&dev3);
                if (distance < SRF04_OK) {
                    puts("Could not read data from srf04 sensor - Ultrasone");
                } 
                else { 
                    printf("Distance: %d mm\n", distance);
                    array[5] = (uint8_t) distance;
                } 

                //Measuring coordinates 
                getGPSCoordinates(&lati, &longi); 
                printf("\nLatitude = %f & Longitude = %f",lati,longi);
                printf("\n");
                //Conversion to sendable lenghts 
                int32_t gps_latitude_payload = round(lati * 1000000);
                array[6] = (gps_latitude_payload & 0xFF000000) >> 24;
                array[7] = (gps_latitude_payload & 0x00FF0000) >> 16;
                array[8] = (gps_latitude_payload & 0x0000FF00) >> 8;
                array[9] = (gps_latitude_payload & 0X000000FF);

                int32_t gps_longitude_payload = round(longi * 1000000);
                array[10] = (gps_longitude_payload & 0xFF000000) >> 24;
                array[11] = (gps_longitude_payload & 0x00FF0000) >> 16;
                array[12] = (gps_longitude_payload & 0x0000FF00) >> 8;
                array[13] = (gps_longitude_payload & 0X000000FF);

                
            }
            //Communication
            printf("Sending information to backend...");
            if(current_interface_id == ALP_ITF_ID_D7ASP) { 
                length = 6;
            }
            else { 
                length = 14;
            }
            modem_status_t status = modem_send_unsolicited_response(0x56, 0, length, array, current_interface_id, current_interface_config);
            if(status == MODEM_STATUS_COMMAND_COMPLETED_SUCCESS) {
                printf("Command completed successfully\n");
                connectionFailed = false;
            } 
            else if(status == MODEM_STATUS_COMMAND_COMPLETED_ERROR) {
                printf("Command completed with error\n");
                connectionFailed = true;
            } 
            else if(status == MODEM_STATUS_COMMAND_TIMEOUT) {
                printf("Command timed out\n");
                connectionFailed = true;
            }
            //Comment this if-else if you want to send both LoRa & D7
            if (connectionFailed) {
                counter++;
            }
            else { 
                counter = 0;
                mode = 0;
                uint32_t duration_usec = xtimer_now_usec() - start;
                printf("Command completed in %li ms\n", duration_usec / 1000);
            }
            if (counter > 1) { 
                counter = 0;
                mode = 0;
                uint32_t duration_usec = xtimer_now_usec() - start;
                printf("Command completed in %li ms\n", duration_usec / 1000);
            }
        }
        else {  
            printf("inactive...\n");
            xtimer_periodic_wakeup(&last_wakeup, INTERVAL);
            //printf("slept until %" PRIu32 "\n", xtimer_usec_from_ticks(xtimer_now()));
            printf("slept for 20s\n");
        }
    }
    return 0;
}