/*
 * Copyright (C) 2014-2017 Freie Universität Berlin
 *               2015 Lari Lehtomäki
 *               2015 TriaGnoSys GmbH
 *               2016-2017 Inria
 *               2016-2017 OTA keys
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     octa
 * @{
 *
 * @file
 * @brief       Board initialization code for octa board
 *
 *
 * @}
 */

#include "board.h"
#include "periph/gpio.h"

void board_init(void)
{
    /* initialize the CPU */
    cpu_init();

    /* initialization of on-board LEDs
     * NOTE: LED0 must be explicitly enabled as it is also used for SPI_DEV(0) */
#ifdef AUTO_INIT_LED0
    gpio_init(LED0_PIN, GPIO_OUT);
    LED0_OFF;
#endif
#ifdef LED1_PIN
    gpio_init(LED1_PIN, GPIO_OUT);
    LED1_OFF;
#endif
#ifdef LED2_PIN
    gpio_init(LED2_PIN, GPIO_OUT);
    LED2_OFF;
#endif 
}
