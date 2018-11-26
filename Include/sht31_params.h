/*
 * Copyright (C) 2018 Gunar Schorcht
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */
 /**
 * @ingroup     drivers_sht3x
 * @brief       Default configuration for Sensirion SHT30/SHT31/SHT35 devices
 * @author      Gunar Schorcht <gunar@schorcht.net>
 * @file
 * @{
 */
 #ifndef SHT31_PARAMS_H
#define SHT31_PARAMS_H
 #include "board.h"
#include "sht31.h"
#include "saul_reg.h"
 #ifdef __cplusplus
extern "C" {
#endif
 /**
 * @name    SHT3x default configuration parameters
 * @{
 */
#ifndef SHT31_PARAM_I2C_DEV
#define SHT31_PARAM_I2C_DEV     (I2C_DEV(0))
#endif
// #ifndef SHT31_PARAM_I2C_ADDR
// #define SHT31_PARAM_I2C_ADDR    (SHT31_I2C_ADDR_2)
// #endif 
#ifndef SHT31_PARAM_I2C_ADDR
#define SHT31_PARAM_I2C_ADDR    (0x44)
#endif
#ifndef SHT31_PARAM_MODE
#define SHT31_PARAM_MODE        (sht31_periodic_2mps)
#endif
#ifndef SHT31_PARAM_REPEAT
#define SHT31_PARAM_REPEAT      (sht31_high)
#endif
 #ifndef SHT31_PARAMS
#define SHT31_PARAMS    { .i2c_dev  = SHT31_PARAM_I2C_DEV,  \
                          .i2c_addr = SHT31_PARAM_I2C_ADDR, \
                          .mode     = SHT31_PARAM_MODE,     \
                          .repeat   = SHT31_PARAM_REPEAT    \
                        }
#endif
#ifndef SHT31_SAUL_INFO
#define SHT31_SAUL_INFO { .name = "sht3x" }
#endif
/**@}*/
 /**
 * @brief   SHT3x configuration
 */
static const sht31_params_t sht31_params[] =
{
    SHT31_PARAMS
};
 /**
 * @brief   Additional meta information to keep in the SAUL registry
 */
static const saul_reg_info_t sht31_saul_info[] =
{
    SHT31_SAUL_INFO
};
 #ifdef __cplusplus
}
#endif
 #endif /* SHT31_PARAMS_H */
/** @} */