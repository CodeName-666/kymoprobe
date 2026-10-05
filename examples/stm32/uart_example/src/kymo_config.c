/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
#include "board.h"
#include <math.h>

/*******************************************************************************
 * clock_ms
 ******************************************************************************/
static uint32_t clock_ms(void *user) { (void)user; return HAL_GetTick(); }

/*******************************************************************************
 * uart_busy
 ******************************************************************************/
static uint8_t uart_busy(void *user) {
    return ((UART_HandleTypeDef *)user)->gState != HAL_UART_STATE_READY;
}

/*******************************************************************************
 * uart_write
 ******************************************************************************/
static uint8_t uart_write(void *user, const uint8_t *bytes, uint8_t length) {
    // HAL borrows the runtime's persistent buffer until the TX interrupt ends.
    return HAL_UART_Transmit_IT((UART_HandleTypeDef *)user, (uint8_t *)bytes,
                                length) == HAL_OK ? length : 0;
}

/*******************************************************************************
 * sample
 ******************************************************************************/
static uint8_t sample(void *user, uint8_t id, KymoSample *out) {
    float phase = (HAL_GetTick() % 2000u) / 2000.0f;
    (void)user;
    out->value = id == 0 ? sinf(phase * 6.283185307f) : 2.0f * phase - 1.0f;
    return 1;
}
static const KymoChannel channels[] = {
    {20, 0, KYMO_FLAG_TIMESTAMP}, {20, 1, KYMO_FLAG_TIMESTAMP}
};
static uint32_t last_sample_ms[2];
const KymoConfig example_config = {
    channels, last_sample_ms, clock_ms, 0, sample, 0,
    uart_write, uart_busy, 0, &huart2, 2
};
