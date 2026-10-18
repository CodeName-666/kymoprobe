/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
#include <kymo_config.h>

#ifndef KYMO_EXPECTED_ENABLED
#error "The profile test must define KYMO_EXPECTED_ENABLED"
#endif

#if KYMO_ENABLED != KYMO_EXPECTED_ENABLED
#error "Application translation unit resolved the wrong global profile"
#endif

#if KYMO_EFFECTIVE_RUNTIME != KYMO_EXPECTED_ENABLED
#error "Application translation unit resolved the wrong effective runtime"
#endif

int kymo_probe_config_contract(void)
{
    return KYMO_CONFIG_API_VERSION;
}
