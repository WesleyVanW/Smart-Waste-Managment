/*
 * Copyright (C) 2018 Gunar Schorcht
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */
 /**
 * @ingroup     drivers_sht31
 * @brief       Device Driver for Sensirion SHT30/SHT31/SHT35 Humidity and Temperature Sensors
 * @author      Gunar Schorcht <gunar@schorcht.net>
 * @file
 */
 #define ENABLE_DEBUG (0)
#include "debug.h"
#include "log.h"
 #include <errno.h>
#include <string.h>
 #include "sht31.h"
#include "xtimer.h"
 #define ASSERT_PARAM(cond) \
    if (!(cond)) { \
        DEBUG("[sht31] %s: %s\n", \
              __func__, "parameter condition (" #cond ") not fulfilled"); \
        assert(cond); \
    }
 #define DEBUG_DEV(f, d, ...) \
        DEBUG("[sht31] %s dev=%d addr=%02x: " f "\n", \
              __func__, d->i2c_dev, d->i2c_addr, ## __VA_ARGS__);
 #define ERROR_DEV(f, d, ...) \
        LOG_ERROR("[sht31] dev=%d addr=%x: " f "\n", \
                  d->i2c_dev, d->i2c_addr, ## __VA_ARGS__);
 /** SHT31 common commands */
#define SHT31_STATUS_CMD               0xF32D
#define SHT31_CLEAR_STATUS_CMD         0x3041
#define SHT31_RESET_CMD                0x30A2
#define SHT31_FETCH_DATA_CMD           0xE000
#define SHT31_HEATER_OFF_CMD           0x3066
 /** SHT31 status register flags */
#define SHT31_STATUS_REG_MASK   (0xbc13)
#define SHT31_STATUS_REG_CRC    (1 << 0)
#define SHT31_STATUS_REG_CMD    (1 << 1)
 /** SHT31 measurement period times in us */
uint32_t SHT31_MEASURE_PERIOD[] = {
          0,   /* [SINGLE_SHOT] */
    2000000,   /* [PERIODIC_05] */
    1000000,   /* [PERIODIC_1 ] */
     500000,   /* [PERIODIC_2 ] */
     250000,   /* [PERIODIC_4 ] */
     100000    /* [PERIODIC_10] */
};
 /** SHT31 measurement command sequences */
const uint16_t SHT31_MEASURE_CMD[6][3] = {
    {0x2400, 0x240b, 0x2416},   /* [SINGLE_SHOT][H, M, L] without clock stretching */
    {0x2032, 0x2024, 0x202f},   /* [PERIODIC_05][H, M, L] */
    {0x2130, 0x2126, 0x212d},   /* [PERIODIC_1 ][H, M, L] */
    {0x2236, 0x2220, 0x222b},   /* [PERIODIC_2 ][H, M, L] */
    {0x2234, 0x2322, 0x2329},   /* [PERIODIC_4 ][H, M, L] */
    {0x2737, 0x2721, 0x272a}    /* [PERIODIC_10][H, M, L] */
};
 /** maximum measurement durations dependent on repatability in ms */
#define SHT31_MEAS_DURATION_REP_HIGH   16
#define SHT31_MEAS_DURATION_REP_MEDIUM 7
#define SHT31_MEAS_DURATION_REP_LOW    5
 #define SHT31_RAW_DATA_SIZE 6
 /** measurement durations in us */
const uint16_t SHT31_MEAS_DURATION_US[3] = { SHT31_MEAS_DURATION_REP_HIGH   * 1000,
                                             SHT31_MEAS_DURATION_REP_MEDIUM * 1000,
                                             SHT31_MEAS_DURATION_REP_LOW    * 1000 };
 /** functions for internal use */
static int _get_raw_data(sht31_dev_t* dev, uint8_t* raw_data);
static int _compute_values (uint8_t* raw_data, int16_t* temp, int16_t* hum);
 /** sensor commands */
static int _start_measurement (sht31_dev_t* dev);
static int _reset (sht31_dev_t* dev);
static int _status (sht31_dev_t* dev, uint16_t* status);
 /** sensor access functions */
static int _send_command(sht31_dev_t* dev, uint16_t cmd);
static int _read_data(sht31_dev_t* dev, uint8_t *data, uint8_t len);
 /** helper functions */
static uint8_t _crc8 (uint8_t data[], int len);
 /** ------------------------------------------------ */
 int sht31_init (sht31_dev_t *dev, const sht31_params_t *params)
{
    ASSERT_PARAM(dev != NULL);
    ASSERT_PARAM(params != NULL);
     int res = SHT31_OK;
     /* inititalize sensor data structure */
    dev->i2c_dev  = params->i2c_dev;
    dev->i2c_addr = params->i2c_addr;
    dev->mode     = params->mode;
    dev->repeat   = params->repeat;
     dev->meas_start_time = 0;
    dev->meas_duration = 0;
    dev->meas_started = false;
     /* try to reset the sensor */
    if ((res = _reset(dev)) != SHT31_OK) {
        return res;
    }
     DEBUG_DEV("sensor initialized", dev);
     /* start periodic measurements */
    if ((dev->mode != sht31_single_shot) &&
        (res = _start_measurement (dev)) != SHT31_OK){
        return res;
    }
     return res;
}
 int sht31_read (sht31_dev_t* dev, int16_t* temp, int16_t* hum)
{
    ASSERT_PARAM(dev != NULL);
    ASSERT_PARAM(temp != NULL || hum != NULL);
     int res = SHT31_OK;
     uint8_t raw_data[SHT31_RAW_DATA_SIZE];
     /*
     * fetches raw sensor data, implicitly starts the measurement if necessary,
     * e.g., in single-shot measurement mode, and waits until sensor data are
     * available
     */
    if ((res = _get_raw_data (dev, raw_data)) != SHT31_OK) {
        return res;
    }
     return _compute_values (raw_data, temp, hum);
}
 /**
 * Functions for internal use only
 */
 static int _start_measurement (sht31_dev_t* dev)
{
    /* start measurement according to selected mode */
    if (_send_command(dev, SHT31_MEASURE_CMD[dev->mode][dev->repeat]) != SHT31_OK)
    {
        DEBUG_DEV("start measurement failed, "
                  "could not send SHT31_MEASURE_CMD to sensor", dev);
        return -SHT31_ERROR_I2C;
    }
     /*
     * in periodic modes, we check whether the command were processed, in
     * single shot mode, reading results has to follow directly.
     */
    if (dev->mode != sht31_single_shot) {
        /* sensor needs up to 250 us to process the measurement command */
        xtimer_usleep (1000);
         uint16_t status;
        int res;
         /* read the status after start measurement command */
        if ((res = _status(dev, &status)) != SHT31_OK) {
            return res;
        }
         if (status & SHT31_STATUS_REG_CMD) {
            DEBUG_DEV("start measurement failed, "
                      "SHT31_MEASURE_CMD were not executed", dev);
            return SHT31_ERROR_MEASURE_CMD_INV;
        }
    }
     dev->meas_start_time = xtimer_now_usec();
    dev->meas_duration = SHT31_MEAS_DURATION_US[dev->repeat];
    dev->meas_started = true;
     return SHT31_OK;
}
 static int _get_raw_data(sht31_dev_t* dev, uint8_t* raw_data)
{
    int res = SHT31_OK;
     /* start the measurement if it has not started yet */
    if (!dev->meas_started &&
        (res = _start_measurement (dev)) != SHT31_OK) {
        return res;
    }
     /* determine the time elapsed since the start of current measurement cycle */
    uint32_t elapsed = xtimer_now_usec() - dev->meas_start_time;
    if (elapsed < dev->meas_duration) {
        /* if necessary, wait until the measurement results become available */
        xtimer_usleep(dev->meas_duration - elapsed);
    }
     /* send fetch command in any periodic mode (mode > 0) before read raw data */
    if (dev->mode != sht31_single_shot &&
        _send_command(dev, SHT31_FETCH_DATA_CMD) != SHT31_OK) {
        DEBUG_DEV("could not send SHT31_FETCH_DATA_CMD to sensor", dev);
        return -SHT31_ERROR_I2C;
    }
     /* read raw data */
    if (_read_data(dev, raw_data, SHT31_RAW_DATA_SIZE) != SHT31_OK) {
        DEBUG_DEV("could not read raw sensor data", dev);
        return -SHT31_ERROR_I2C;
    }
     /* stop measurement in single shot mode by resetting the started flag */
    if (dev->mode == sht31_single_shot) {
        dev->meas_started = false;
    }
    /* start next measurement cycle in periodic modes */
    else {
        dev->meas_start_time = xtimer_now_usec();
        dev->meas_duration = SHT31_MEASURE_PERIOD[dev->mode];
    }
     /* check temperature crc */
    if (_crc8(raw_data,2) != raw_data[2]) {
        DEBUG_DEV("CRC check for temperature data failed", dev);
        return -SHT31_ERROR_CRC;
    }
     /* check humidity crc */
    if (_crc8(raw_data+3,2) != raw_data[5]) {
        DEBUG_DEV("CRC check for humidity data failed", dev);
        return -SHT31_ERROR_CRC;
    }
     return SHT31_OK;
}
 static int _compute_values (uint8_t* raw_data, int16_t* temp, int16_t* hum)
{
    if (temp) {
        uint32_t _tmp = ((uint32_t)raw_data[0] << 8) + raw_data[1];  /* S_T */
        _tmp  *= 17500; /* S_T x 175 scaled to hundredths of degree */
        _tmp >>= 16;    /* S_T x 175 / 65535 (shift inaccuracy < resolution) */
        _tmp  -= 4500;  /* S_T x 175 / 65535 - 45 */
        *temp = _tmp;
    }
     if (hum) {
        uint32_t _tmp = ((uint32_t)raw_data[3] << 8) + raw_data[4]; /* S_RH  */
        _tmp  *= 10000; /* S_RH x 100 scaled to hundredths of degree */
        _tmp >>= 16;    /* S_RH x 100 / 65535 (shift inaccuracy < resolution) */
        *hum = _tmp;
    }
     return SHT31_OK;
}
 static int _send_command(sht31_dev_t* dev, uint16_t cmd)
{
    ASSERT_PARAM (dev != NULL);
     int res = SHT31_OK;
     uint8_t data[2] = { cmd >> 8, cmd & 0xff };
    DEBUG_DEV("send command 0x%02x%02x", dev, data[0], data[1]);
     if (i2c_acquire(dev->i2c_dev) != SHT31_OK) {
        DEBUG_DEV ("could not aquire I2C bus", dev);
        return -SHT31_ERROR_I2C;
    }
     res = i2c_write_bytes(dev->i2c_dev, dev->i2c_addr, (const void*)data, 2, 0);
    i2c_release(dev->i2c_dev);
     if (res != SHT31_OK) {
        DEBUG_DEV("could not send command 0x%02x%02x to sensor, reason=%d",
                  dev, data[0], data[1], res);
        return -SHT31_ERROR_I2C;
    }
     return SHT31_OK;
}
 static int _read_data(sht31_dev_t* dev, uint8_t *data, uint8_t len)
{
    int res = SHT31_OK;
     if (i2c_acquire(dev->i2c_dev) != SHT31_OK) {
        DEBUG_DEV ("could not aquire I2C bus", dev);
        return -SHT31_ERROR_I2C;
    }
     res = i2c_read_bytes(dev->i2c_dev, dev->i2c_addr, (void*)data, len, 0);
    i2c_release(dev->i2c_dev);
     if (res == SHT31_OK) {
        #if ENABLE_DEBUG
        printf("[sht31] %s bus=%d addr=%02x: read following bytes: ",
               __func__, dev->i2c_dev, dev->i2c_addr);
        for (int i=0; i < len; i++)
            printf("%02x ", data[i]);
        printf("\n");
        #endif /* ENABLE_DEBUG */
    }
    else {
        DEBUG_DEV("could not read %d bytes from sensor, reason %d (%s)",
                  dev, len, res, strerror(res * -1));
        return -SHT31_ERROR_I2C;
    }
     return SHT31_OK;
}
 static int _reset (sht31_dev_t* dev)
{
    ASSERT_PARAM (dev != NULL);
     DEBUG_DEV("", dev);
     /**
     * Sensor can only be soft resetted in idle mode. Therefore, we try to
     * start a single shot measurement and wait for its completion. After
     * that the sensor should be in idle mode. We don't check I2C errors
     * at this moment.
     */
    _send_command(dev, SHT31_MEASURE_CMD[sht31_single_shot][sht31_low]);
    xtimer_usleep (3000);
     /* send the soft-reset command */
    if (_send_command(dev, SHT31_RESET_CMD) != SHT31_OK) {
        DEBUG_DEV("reset failed, could not send SHT31_RESET_CMD", dev);
        return -SHT31_ERROR_I2C;
    }
     /* wait for 2 ms, the time needed to restart (according to datasheet 0.5 ms) */
    xtimer_usleep (2000);
     /* send reset command */
    if (_send_command(dev, SHT31_CLEAR_STATUS_CMD) != SHT31_OK) {
        DEBUG_DEV("reset failed, could not send SHT31_CLEAR_STATUS_CMD", dev);
        return -SHT31_ERROR_I2C;
    }
     /* sensor needs some time to process the command */
    xtimer_usleep (500);
     uint16_t status;
    int res = SHT31_OK;
     /* read the status after reset */
    if ((res = _status(dev, &status)) != SHT31_OK) {
        return res;
    }
     /* status register should be 0x0000 after a reset */
    if (status != 0) {
        DEBUG_DEV("reset failed, sensor status is status=%04x", dev, status);
        return -SHT31_ERROR_STATUS;
    }
     DEBUG_DEV("sensor status=%04x", dev, status);
     return res;
}
 static int _status (sht31_dev_t* dev, uint16_t* status)
{
    ASSERT_PARAM (dev != NULL);
    ASSERT_PARAM (status != NULL);
     DEBUG_DEV("", dev);
     uint8_t  data[3];
     /* read sensor status */
    if (_send_command(dev, SHT31_STATUS_CMD) != SHT31_OK) {
        DEBUG_DEV("could not send SHT31_STATUS_CMD to sensor", dev);
        return -SHT31_ERROR_I2C;
    }
    if (_read_data(dev, data, 3) != SHT31_OK) {
        DEBUG_DEV("could not read status data from sensor", dev);
        return -SHT31_ERROR_I2C;
    }
     /* check status crc */
    if (_crc8(data,2) != data[2]) {
        DEBUG_DEV("CRC check for status failed", dev);
        return -SHT31_ERROR_CRC;
    }
     *status = (data[0] << 8 | data[1]) & SHT31_STATUS_REG_MASK;
    DEBUG_DEV("status=%02x", dev, *status);
    return SHT31_OK;
}
 static const uint8_t g_polynom = 0x31;
 static uint8_t _crc8 (uint8_t data[], int len)
{
    /* initialization value */
    uint8_t crc = 0xff;
     /* iterate over all bytes */
    for (int i=0; i < len; i++)
    {
        crc ^= data[i];
         for (int i = 0; i < 8; i++)
        {
            bool xor = crc & 0x80;
            crc = crc << 1;
            crc = xor ? crc ^ g_polynom : crc;
        }
    }
     return crc;
}