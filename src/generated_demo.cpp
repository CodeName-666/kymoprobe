/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial */
/** @file generated_demo.cpp @brief Configurator-driven UART demonstration. */
#include "demo_selection.h"
#include <KymoCore.h>
#if (KYMO_DEMO_MODE == KYMO_DEMO_GENERATED) && KYMO_EFFECTIVE_RUNTIME
#include <Arduino.h>
#include <kymo_generated_config.h>
static KymoContext generatedContext;

/** @brief Start hardware before initializing the generated configuration. */
/*******************************************************************************
 * demoGeneratedInit
 ******************************************************************************/
void demoGeneratedInit(void)
{
    Serial.begin(115200u);
    (void)Kymo_Init(&generatedContext, &kymo_generated_config);
}

/** @brief Advance the runtime selected by the immutable profile. */
/*******************************************************************************
 * demoGeneratedMain
 ******************************************************************************/
void demoGeneratedMain(void)
{
    (void)Kymo_Main(&generatedContext);
}
#endif
