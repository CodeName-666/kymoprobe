/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
#include <Arduino.h>
#include <math.h>
#include <KymoCore.h>
#include "demo_selection.h"

#if (KYMO_DEMO_MODE == KYMO_DEMO_GENERATED) && KYMO_EFFECTIVE_RUNTIME

/*******************************************************************************
 * kymo_port_clock_ms
 ******************************************************************************/
extern "C" uint32_t kymo_port_clock_ms(void *user)
{
    uint32_t result = millis();
    (void)user;
    return result;
}

/*******************************************************************************
 * kymo_port_sample
 ******************************************************************************/
extern "C" uint8_t kymo_port_sample(void *user, uint8_t id, KymoSample *out)
{
    uint8_t result = 1u;
    const float phase = (millis() % 2000u) / 2000.0f;
    (void)user;
    out->value = id == 0u ? sinf(phase * 6.283185307f) : 2.0f * phase - 1.0f;
    return result;
}

/*******************************************************************************
 * kymo_port_write
 ******************************************************************************/
extern "C" uint8_t kymo_port_write(void *user, const uint8_t *bytes, uint8_t length)
{
    uint8_t result = 0u;
    int available = Serial.availableForWrite();
    (void)user;
    if ((bytes != nullptr) && (available > 0)) {
        if (available < length) {
            length = static_cast<uint8_t>(available);
        }
        result = static_cast<uint8_t>(Serial.write(bytes, length));
    }
    return result;
}

/*******************************************************************************
 * kymo_port_busy
 ******************************************************************************/
extern "C" uint8_t kymo_port_busy(void *user)
{
    uint8_t result = Serial.availableForWrite() <= 0 ? 1u : 0u;
    (void)user;
    return result;
}

/*******************************************************************************
 * kymo_port_service
 ******************************************************************************/
extern "C" void kymo_port_service(void *user)
{
    (void)user;
}
#endif
