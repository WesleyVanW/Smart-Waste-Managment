/*
 * Copyright (C) 2018 Gunar Schorcht
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */
 /**
 * @ingroup     sys_auto_init_saul
 * @brief       Auto initialization of Sensirion SHT30/SHT31/SHT35 device driver
 * @author      Gunar Schorcht <gunar@schorcht.net>
 * @file
 */
 #ifdef MODULE_SHT31
 #include "assert.h"
#include "log.h"
#include "saul_reg.h"
#include "sht31.h"
#include "sht31_params.h"
 /**
 * @brief   Define the number of configured sensors
 */
#define SHT31_NUM      (sizeof(sht31_params) / sizeof(sht31_params[0]))
 /**
 * @brief   Allocation of memory for device descriptors
 */
static sht31_dev_t sht31_devs[SHT31_NUM];
 /**
 * @brief   Memory for the SAUL registry entries
 */
static saul_reg_t saul_entries[SHT31_NUM * 2];
 /**
 * @brief   Define the number of saul info
 */
#define SHT31_INFO_NUM (sizeof(sht31_saul_info) / sizeof(sht31_saul_info[0]))
 /**
 * @name    Reference the driver structs.
 * @{
 */
extern const saul_driver_t sht31_saul_driver_temperature;
extern const saul_driver_t sht31_saul_driver_humidity;
/** @} */
 void auto_init_sht31(void)
{
    assert(SHT31_INFO_NUM == SHT31_NUM);
     for (unsigned i = 0; i < SHT31_NUM; i++) {
        LOG_DEBUG("[auto_init_saul] initializing sht31 #%u\n", i);
         if (sht31_init(&sht31_devs[i],
                       &sht31_params[i]) != SHT31_OK) {
            LOG_ERROR("[auto_init_saul] error initializing sht31 #%u\n", i);
            continue;
        }
         /* temperature */
        saul_entries[(i * 2)].dev = &(sht31_devs[i]);
        saul_entries[(i * 2)].name = sht31_saul_info[i].name;
        saul_entries[(i * 2)].driver = &sht31_saul_driver_temperature;
         /* relative humidity */
        saul_entries[(i * 2) + 1].dev = &(sht31_devs[i]);
        saul_entries[(i * 2) + 1].name = sht31_saul_info[i].name;
        saul_entries[(i * 2) + 1].driver = &sht31_saul_driver_humidity;
         /* register to saul */
        saul_reg_add(&(saul_entries[(i * 2)]));
        saul_reg_add(&(saul_entries[(i * 2) + 1]));
    }
}
#else
typedef int dont_be_pedantic;
#endif /* MODULE_SHT31 */