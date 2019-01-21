/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */

/*
 * @ingroup     sys_auto_init_saul
 * @{
 *
 * @file
 * @brief       Auto initialization of LSM303DLHC accelerometer/magnetometer
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#ifdef MODULE_LSM303AGR

#include "log.h"
#include "saul_reg.h"
#include "lsm303agr.h"
#include "lsm303agr_params.h"

/**
 * @brief   Define the number of configured sensors
 */
#define LSM303AGR_NUM    (sizeof(lsm303agr_params) / sizeof(lsm303agr_params[0]))

/**
 * @brief   Allocate memory for the device descriptors
 */
static lsm303agr_t lsm303agr_devs[LSM303AGR_NUM];

/**
 * @brief   Memory for the SAUL registry entries
 */
static saul_reg_t saul_entries[LSM303AGR_NUM * 2];

/**
 * @brief   Define the number of saul info
 */
#define LSM303AGR_INFO_NUM    (sizeof(lsm303agr_saul_info) / sizeof(lsm303agr_saul_info[0]))

/**
 * @name    Reference the driver structs
 * @{
 */
extern saul_driver_t lsm303agr_saul_acc_driver;
extern saul_driver_t lsm303agr_saul_mag_driver;
/** @} */

void auto_init_lsm303agr(void)
{
    assert(LSM303AGR_NUM == LSM303AGR_INFO_NUM);

    for (unsigned int i = 0; i < LSM303AGR_NUM; i++) {
        LOG_DEBUG("[auto_init_saul] initializing lsm303dlhc #%u\n", i);

        if (lsm303agr_init(&lsm303agr_devs[i], &lsm303agr_params[i]) < 0) {
            LOG_ERROR("[auto_init_saul] error initializing lsm303dlhc #%u\n", i);
            continue;
        }

        saul_entries[(i * 2)].dev = &(lsm303agr_devs[i]);
        saul_entries[(i * 2)].name = lsm303agr_saul_info[i].name;
        saul_entries[(i * 2)].driver = &lsm303agr_saul_acc_driver;
        saul_entries[(i * 2) + 1].dev = &(lsm303agr_devs[i]);
        saul_entries[(i * 2) + 1].name = lsm303agr_saul_info[i].name;
        saul_entries[(i * 2) + 1].driver = &lsm303agr_saul_mag_driver;
        saul_reg_add(&(saul_entries[(i * 2)]));
        saul_reg_add(&(saul_entries[(i * 2) + 1]));
    }
}

#else
typedef int dont_be_pedantic;
#endif /* MODULE_LSM303AGR */
