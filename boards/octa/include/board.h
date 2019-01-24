/**
 * 
 * 
 * @brief       Support for OCTA board
 * @{
 *
 * @file
 * @brief       Common pin definitions and board configuration options
 *
 */

#ifndef BOARD_H
#define BOARD_H

#include "board_nucleo.h"
#include "arduino_pinmap.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    LED pin definitions and handlers
 * @{
 */
#define LED0_PIN            GPIO_PIN(PORT_D, 14)
#define LED0_MASK           (1 << 14)
#define LED0_ON             (GPIOD->BSRR = (LED0_MASK << 16))
#define LED0_OFF            (GPIOD->BSRR = LED0_MASK)
#define LED0_TOGGLE         (GPIOD->ODR  ^= LED0_MASK)

#define LED1_PIN            GPIO_PIN(PORT_B, 0)
#define LED1_MASK           (1 << 0)
#define LED1_ON             (GPIOB->BSRR = (LED1_MASK << 16))
#define LED1_OFF            (GPIOB->BSRR = LED1_MASK)
#define LED1_TOGGLE         (GPIOB->ODR  ^= LED1_MASK)

#define LED2_PIN            GPIO_PIN(PORT_D, 15)
#define LED2_MASK           (1 << 15)
#define LED2_ON             (GPIOD->BSRR = (LED2_MASK << 16))
#define LED2_OFF            (GPIOD->BSRR = LED2_MASK)
#define LED2_TOGGLE         (GPIOD->ODR  ^= LED2_MASK) 


/* the Nucleo144 boards always use LED0, as there is no dual use of its pin */
#define AUTO_INIT_LED0
/** @} */

/**
 * @name    User button
 * @{
 */
#define BTN0_PIN            GPIO_PIN(PORT_C, 13)
#define BTN0_MODE           GPIO_IN_PD
/** @} */


#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
/** @} */