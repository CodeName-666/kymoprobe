/**
 * @brief Hardware boundary for the STM32 UART example.
 *
 * @details Selects STM32F1 HAL for STM32F103xB, otherwise STM32F4 HAL. This application-only
 * header is intentionally outside the portable library. Peripheral setup and IRQ
 * forwarding live in the example main.c.
 * @file board.h
 * @defgroup example_stm32_board STM32 example board
 * @{
 * @par Usage
 * Include only from the STM32 UART application and its configuration module.
 */
#ifndef EXAMPLE_BOARD_H
/**
 * @brief Include guard for board.h.
 *
 * @details Internal preprocessor symbol; do not define it in application code.
 * @par Usage
 * Include this header normally; the compiler manages this guard.
 */
#define EXAMPLE_BOARD_H

#if defined(STM32F103xB)
#include "stm32f1xx_hal.h"
#else
#include "stm32f4xx_hal.h"
#endif
#include <kymo_runtime.h>

/**
 * @brief USART2 driver handle shared by startup, IRQ and transport callbacks.
 *
 * @details Input/output HAL state initialized by uart_init in main.c. The interrupt handler
 * updates transfer state; the configured busy callback prevents premature buffer reuse.
 * @par Usage
 * Initialize before Kymo_Init and forward USART2 IRQ to HAL_UART_IRQHandler(&huart2).
 */
extern UART_HandleTypeDef huart2;

/**
 * @brief Persistent C/HAL channel and callback configuration.
 *
 * @details Input to Kymo_Init; binds interrupt-driven UART writes and the HAL millisecond clock.
 * Its pointed-to scheduling state is owned by the application and modified by the runtime.
 * @par Usage
 * Kymo_Init(&kymo, &example_config);
 */
extern const KymoConfig example_config;

/** @} */
#endif /* EXAMPLE_BOARD_H */
