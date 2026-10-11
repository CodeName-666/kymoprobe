/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
#include <Arduino.h>
#include "kymo_config.h"

#if (!defined(KYMO_DEMO_NETWORK_ENABLE) || !KYMO_DEMO_NETWORK_ENABLE) && \
    (!defined(KYMO_STRESS_ENABLE) || !KYMO_STRESS_ENABLE)
#include <kymo_generated_config.h>
#if KYMO_EFFECTIVE_RUNTIME
static KymoContext kymo;

/*******************************************************************************
 * setup
 ******************************************************************************/
void setup() {
    Serial.begin(115200);
    (void)Kymo_Init(&kymo, &kymo_generated_config);
}

/*******************************************************************************
 * loop
 ******************************************************************************/
void loop() {
    (void)Kymo_Main(&kymo);
}
#endif
#endif

#if !KYMO_EFFECTIVE_RUNTIME
/*******************************************************************************
 * setup
 ******************************************************************************/
void setup() {}

/*******************************************************************************
 * loop
 ******************************************************************************/
void loop() {}
#endif
