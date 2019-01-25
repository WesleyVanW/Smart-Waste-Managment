/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_LSM303AGR
 * @{
 *
 * @file
 * @brief       Device driver implementation for the LSM303AGR 3D accelerometer/magnetometer.
 *
 * @author      Thomas Eichinger <thomas.eichinger@fu-berlin.de>
 * @author      Peter Kietzmann <peter.kietzmann@haw-hamburg.de>
 *
 * @}
 */

#include "lsm303agr.h"
#include "lsm303agr-internal.h"

#define ENABLE_DEBUG    (0)
#include "debug.h"

#define DEV_I2C         (dev->params.i2c)
#define DEV_ACC_ADDR    (dev->params.acc_addr)
#define DEV_ACC_PIN     (dev->params.acc_pin)
#define DEV_ACC_RATE    (dev->params.acc_rate)
#define DEV_ACC_SCALE   (dev->params.acc_scale)
#define DEV_MAG_ADDR    (dev->params.mag_addr)
#define DEV_MAG_PIN     (dev->params.mag_pin)
#define DEV_MAG_RATE    (dev->params.mag_rate)
#define DEV_MAG_GAIN    (dev->params.mag_gain)

int lsm303agr_init(lsm303agr_t *dev, const lsm303agr_params_t *params)
{
    dev->params = *params;

    int res;
    uint8_t tmp;

    /* Acquire exclusive access to the bus. */
    i2c_acquire(DEV_I2C);

    DEBUG("LSM303AGR reboot...");
    res = i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                        LSM303AGR_REG_CTRL5_A, LSM303AGR_REG_CTRL5_A_BOOT, 0);
    /* Release the bus for other threads. */
    i2c_release(DEV_I2C);
    DEBUG("[OK]\n");

    /* configure accelerometer */
    /* enable all three axis, set sample rate (10Hz) and low power mode */
    tmp = (LSM303AGR_CTRL1_A_XEN
          | LSM303AGR_CTRL1_A_YEN
          | LSM303AGR_CTRL1_A_ZEN
          | DEV_ACC_RATE
          | LSM303AGR_CTRL1_A_LOW_POWER);
    i2c_acquire(DEV_I2C);
    res += i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                         LSM303AGR_REG_CTRL1_A, tmp, 0);
    /* update on read, MSB @ low address, scale */
    tmp = (DEV_ACC_SCALE);
    res += i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                         LSM303AGR_REG_CTRL4_A, tmp, 0);
    /* no interrupt generation */
    res += i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                         LSM303AGR_REG_CTRL3_A, LSM303AGR_CTRL3_A_I1_NONE, 0);
    /* configure acc data ready pin */
    gpio_init(DEV_ACC_PIN, GPIO_IN);

    /* configure magnetometer and temperature */
    /* enable temperature output and set sample rate */
    tmp = LSM303AGR_TEMP_EN | DEV_MAG_RATE;
    res += i2c_write_reg(DEV_I2C, DEV_MAG_ADDR,
                         LSM303AGR_REG_CRA_M, tmp, 0);
    /* configure z-axis gain */
    res += i2c_write_reg(DEV_I2C, DEV_MAG_ADDR,
                         LSM303AGR_REG_CRB_M, DEV_MAG_GAIN, 0);
    /* set continuous mode */
    res += i2c_write_reg(DEV_I2C, DEV_MAG_ADDR,
                         LSM303AGR_REG_MR_M, LSM303AGR_MAG_MODE_CONTINUOUS, 0);
    i2c_release(DEV_I2C);
    /* configure mag data ready pin */
    gpio_init(DEV_MAG_PIN, GPIO_IN);

    return (res < 0) ? -1 : 0;
}

int lsm303agr_read_acc(const lsm303agr_t *dev, lsm303agr_3d_data_t *data)
{
    int res;
    uint8_t tmp;

    i2c_acquire(DEV_I2C);
    i2c_read_reg(DEV_I2C, DEV_ACC_ADDR, LSM303AGR_REG_STATUS_A, &tmp, 0);
    DEBUG("LSM303AGR status: %x\n", tmp);
    DEBUG("LSM303AGR: wait for acc values ... ");

    res = i2c_read_reg(DEV_I2C, DEV_ACC_ADDR,
                       LSM303AGR_REG_OUT_X_L_A, &tmp, 0);
    data->x_axis = tmp;
    res += i2c_read_reg(DEV_I2C, DEV_ACC_ADDR,
                        LSM303AGR_REG_OUT_X_H_A, &tmp, 0);
    data->x_axis |= tmp<<8;
    res += i2c_read_reg(DEV_I2C, DEV_ACC_ADDR,
                       LSM303AGR_REG_OUT_Y_L_A, &tmp, 0);
    data->y_axis = tmp;
    res += i2c_read_reg(DEV_I2C, DEV_ACC_ADDR,
                        LSM303AGR_REG_OUT_Y_H_A, &tmp, 0);
    data->y_axis |= tmp<<8;
    res += i2c_read_reg(DEV_I2C, DEV_ACC_ADDR,
                       LSM303AGR_REG_OUT_Z_L_A, &tmp, 0);
    data->z_axis = tmp;
    res += i2c_read_reg(DEV_I2C, DEV_ACC_ADDR,
                        LSM303AGR_REG_OUT_Z_H_A, &tmp, 0);
    data->z_axis |= tmp<<8;
    i2c_release(DEV_I2C);
    DEBUG("read ... ");

    data->x_axis = data->x_axis>>4;
    data->y_axis = data->y_axis>>4;
    data->z_axis = data->z_axis>>4;

    if (res < 0) {
        DEBUG("[!!failed!!]\n");
        return -1;
    }
    DEBUG("[done]\n");

    return 0;
}

int lsm303agr_read_mag(const lsm303agr_t *dev, lsm303agr_3d_data_t *data)
{
    int res;

    DEBUG("LSM303AGR: wait for mag values... ");
    while (gpio_read(DEV_MAG_PIN) == 0){}

    DEBUG("read ... ");

    i2c_acquire(DEV_I2C);
    res = i2c_read_regs(DEV_I2C, DEV_MAG_ADDR,
                        LSM303AGR_REG_OUT_X_H_M, data, 6, 0);
    i2c_release(DEV_I2C);

    if (res < 0) {
        DEBUG("[!!failed!!]\n");
        return -1;
    }
    DEBUG("[done]\n");

    /* interchange y and z axis and fix endiness */
    int16_t tmp = data->y_axis;
    data->x_axis = ((data->x_axis<<8)|((data->x_axis>>8)&0xff));
    data->y_axis = ((data->z_axis<<8)|((data->z_axis>>8)&0xff));
    data->z_axis = ((tmp<<8)|((tmp>>8)&0xff));

    /* compensate z-axis sensitivity */
    /* gain is currently hardcoded to LSM303AGR_GAIN_5 */
    data->z_axis = ((data->z_axis * 400) / 355);

    return 0;
}

int lsm303agr_read_temp(const lsm303agr_t *dev, int16_t *value)
{
    int res;

    i2c_acquire(DEV_I2C);
    res = i2c_read_regs(DEV_I2C, DEV_MAG_ADDR, LSM303AGR_REG_TEMP_OUT_H,
                        value, 2, 0);
    i2c_release(DEV_I2C);

    if (res < 0) {
        return -1;
    }

    *value = (((*value) >> 8) & 0xff) | (*value << 8);

    DEBUG("LSM303AGR: raw temp: %i\n", *value);

    return 0;
}

int lsm303agr_enable_interrupt(const lsm303agr_t *dev)
{
	int res;

	//Default scale = 4G
	//Default ODR = 10 Hz
	//page 55 in manual for registers

    i2c_acquire(DEV_I2C);

    uint8_t temp = {LSM303AGR_CTRL1_A_XEN |
                    LSM303AGR_CTRL1_A_YEN |
                    LSM303AGR_CTRL1_A_ZEN |
                    LSM303AGR_CTRL1_A_10HZ};

	res = i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                        LSM303AGR_REG_CTRL1_A ,temp, 0);
	res = i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                        LSM303AGR_REG_CTRL2_A ,0x00, 0); //high pass filter disabled
	
    temp = LSM303AGR_CTRL3_A_I1_AOI1;
    
    res += i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                        LSM303AGR_REG_CTRL3_A ,temp, 0); //Interrupt driven to INT1 pad
	
    temp = LSM303AGR_CTRL4_A_SCALE_2G;

    res += i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                        LSM303AGR_REG_CTRL4_A ,temp, 0); //FS = 2G
	
    //temp = LSM303AGR_REG_CTRL5_A_LIR_INT1;

    //res += i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
    //                    LSM303AGR_REG_CTRL5_A ,0x08, 0); //Interrupt latched
	res += i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                        LSM303AGR_REG_INT1_THS_A ,0x10, 0); //Set threshold = 250mg //0x10
	res += i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                        LSM303AGR_REG_INT1_DURATION_A ,0x00, 0); //Set minimum event duration

    uint8_t temp1 = {LSM303AGR_INT1_XHIE |
                     LSM303AGR_INT1_YHIE 
              };

    res += i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                        LSM303AGR_REG_INT1_CFG_A ,temp1, 0); //Xl Yl enable
	
    i2c_release(DEV_I2C);
						
	return (res < 0) ? -1 : 0;
}

int lsm303agr_clear_int(const lsm303agr_t *dev, int8_t *value)
{
	int res;

    i2c_acquire(DEV_I2C);
    res = i2c_read_regs(DEV_I2C, DEV_MAG_ADDR, LSM303AGR_REG_INT1_SRC_A,
                        value, 1, 0);
    i2c_release(DEV_I2C);
	
	return res;
}

int lsm303agr_disable(const lsm303agr_t *dev)
{
    int res;

    i2c_acquire(DEV_I2C);
    res = i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                        LSM303AGR_REG_CTRL1_A, LSM303AGR_CTRL1_A_POWEROFF, 0);
    res += i2c_write_reg(DEV_I2C, DEV_MAG_ADDR,
                        LSM303AGR_REG_MR_M, LSM303AGR_MAG_MODE_SLEEP, 0);
    res += i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                        LSM303AGR_REG_CRA_M, LSM303AGR_TEMP_DIS, 0);
    i2c_release(DEV_I2C);

    return (res < 0) ? -1 : 0;
}

int lsm303agr_disable_mag(const lsm303agr_t *dev)
{
    int res = 0;

    i2c_acquire(DEV_I2C);
    res += i2c_write_reg(DEV_I2C, DEV_MAG_ADDR,
                        LSM303AGR_REG_MR_M, LSM303AGR_MAG_MODE_SLEEP, 0);
    i2c_release(DEV_I2C);

    return (res < 0) ? -1 : 0;
}

int lsm303agr_disable_temp(const lsm303agr_t *dev)
{
    int res = 0;

    i2c_acquire(DEV_I2C);
    res += i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                        LSM303AGR_REG_CRA_M, LSM303AGR_TEMP_DIS, 0);
    i2c_release(DEV_I2C);

    return (res < 0) ? -1 : 0;
}

int lsm303agr_disable_acc(const lsm303agr_t *dev)
{
    int res = 0;

    i2c_acquire(DEV_I2C);
    res = i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                        LSM303AGR_REG_CTRL1_A, LSM303AGR_CTRL1_A_POWEROFF, 0);
    i2c_release(DEV_I2C);

    return (res < 0) ? -1 : 0;
}

int lsm303agr_enable_acc(const lsm303agr_t *dev)
{
    int res = 0;

    uint8_t tmp = (LSM303AGR_CTRL1_A_XEN
          | LSM303AGR_CTRL1_A_YEN
          | LSM303AGR_CTRL1_A_ZEN
          | DEV_ACC_RATE
          | LSM303AGR_CTRL1_A_LOW_POWER);

    i2c_acquire(DEV_I2C);
    res += i2c_write_reg(DEV_I2C, DEV_ACC_ADDR,
                         LSM303AGR_REG_CTRL1_A, tmp, 0);

    i2c_release(DEV_I2C);

    return (res < 0) ? -1 : 0;
}

int lsm303agr_enable(const lsm303agr_t *dev)
{
    int res;
    uint8_t tmp = (LSM303AGR_CTRL1_A_XEN
                  | LSM303AGR_CTRL1_A_YEN
                  | LSM303AGR_CTRL1_A_ZEN
                  | LSM303AGR_CTRL1_A_N1344HZ_L5376HZ);
    i2c_acquire(DEV_I2C);
    res = i2c_write_reg(DEV_I2C, DEV_ACC_ADDR, LSM303AGR_REG_CTRL1_A, tmp, 0);

    tmp = (LSM303AGR_CTRL4_A_BDU| LSM303AGR_CTRL4_A_SCALE_2G | LSM303AGR_CTRL4_A_HR);
    res += i2c_write_reg(DEV_I2C, DEV_ACC_ADDR, LSM303AGR_REG_CTRL4_A, tmp, 0);
    res += i2c_write_reg(DEV_I2C, DEV_ACC_ADDR, LSM303AGR_REG_CTRL3_A,
                         LSM303AGR_CTRL3_A_I1_DRDY1, 0);
    gpio_init(DEV_ACC_PIN, GPIO_IN);

    tmp = LSM303AGR_TEMP_EN | LSM303AGR_TEMP_SAMPLE_75HZ;
    res += i2c_write_reg(DEV_I2C, DEV_MAG_ADDR, LSM303AGR_REG_CRA_M, tmp, 0);

    res += i2c_write_reg(DEV_I2C, DEV_MAG_ADDR,
                        LSM303AGR_REG_CRB_M, LSM303AGR_GAIN_5, 0);

    res += i2c_write_reg(DEV_I2C, DEV_MAG_ADDR,
                        LSM303AGR_REG_MR_M, LSM303AGR_MAG_MODE_CONTINUOUS, 0);
    i2c_release(DEV_I2C);

    gpio_init(DEV_MAG_PIN, GPIO_IN);

    return (res < 0) ? -1 : 0;
}