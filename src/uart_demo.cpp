/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial */
/** @file uart_demo.cpp @brief Two timestamped UART waveform channels. */
#include "demo_selection.h"
#include <KymoCore.h>
#if (KYMO_DEMO_MODE == KYMO_DEMO_UART) && KYMO_EFFECTIVE_RUNTIME
#include "../examples/common/arduino_serial_config.h"
static KymoContext uartContext;

/** @brief Start UART and its fixed runtime configuration. */
/*******************************************************************************
 * demoUartInit
 ******************************************************************************/
void demoUartInit(void)
{
    Serial.begin(115200u);
    (void)Kymo_Init(&uartContext, &example_config);
}

/** @brief Advance acquisition and transmission cooperatively. */
/*******************************************************************************
 * demoUartMain
 ******************************************************************************/
void demoUartMain(void)
{
    (void)Kymo_Main(&uartContext);
}
#endif
