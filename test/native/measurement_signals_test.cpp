/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
/** @file measurement_signals_test.cpp @brief Emit actual fixture codec frames.
 * @details Cross-language test: the Studio decoder consumes the output file. */
#include <stdio.h>
#include <stdint.h>
#include <kymo_protocol.h>
#include "Waveforms.h"

/** @brief Generate exactly 2048 sample instants of all channels.
 * @param[in] argc Argument count. @param[in] argv Output filename in argv[1].
 * @return Zero on success, one for file or codec errors. */
/*******************************************************************************
 * main
 ******************************************************************************/
int32_t main(int32_t argc, char **argv)
{
    FILE *output = nullptr;
    int32_t result = 1;
    bool isValid = true;
    KymoDataPoint point = {};
    uint8_t bytes[KYMO_FRAME_MAX_SIZE] = {};
    size_t length = 0u;
    if ((argc == 2) && (argv != nullptr) && (argv[1] != nullptr)) {
        output = fopen(argv[1], "wb");
    }
    if (output != nullptr) {
        for (uint32_t sampleIndex = 0u; sampleIndex < 2048u; ++sampleIndex) {
            const MeasurementSet_T set = calculateMeasurementSet(sampleIndex * SAMPLE_PERIOD_MS);
            point.flags = KYMO_FLAG_TIMESTAMP;
            point.timestamp_ms = set.timestampMs;
            for (uint32_t channel = 0u; channel < MEASUREMENT_CHANNEL_COUNT; ++channel) {
                point.id = static_cast<uint8_t>(channel);
                point.value = set.values[channel];
                length = kymo_encode_data(&point, bytes, sizeof(bytes));
                isValid = (length != 0u) && isValid;
                isValid = (fwrite(bytes, 1u, length, output) == length) && isValid;
            }
        }
        isValid = (fclose(output) == 0) && isValid;
        result = isValid ? 0 : 1;
    }
    return result;
}
