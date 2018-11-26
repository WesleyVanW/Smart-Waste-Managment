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
 int main(void)
{
    sht31_dev_t dev;
    int res;
     puts("SHT31 test application\n");
     printf("+------------Initializing------------+\n");
     if ((res = sht31_init(&dev, &sht31_params[0])) != SHT31_OK) {
        puts("Initialization failed\n");
        return 1;
    }
    else {
        puts("Initialization successful\n");
    }
     printf("\n+--------Starting Measurements--------+\n");
     while (1) {
        int16_t temp;
        int16_t hum;
         if ((res = sht31_read(&dev, &temp, &hum)) == SHT31_OK) {
            printf("Temperature [°C]: %d.%d\n"
                   "Relative Humidity [%%]: %d.%d\n"
                   "+-------------------------------------+\n",
                   temp / 100, temp % 100,
                   hum / 100, hum % 100);
        }
        else {
            printf("Could not read data from sensor, error %d\n", res);
        }
        xtimer_usleep(1000000);
    }
     return 0;
}