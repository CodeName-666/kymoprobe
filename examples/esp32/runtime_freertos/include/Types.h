/** @file Types.h @brief Platform-independent workload descriptions.
 * @details Describes bounded work cycles without SDK handles or heap ownership.
 * @par Usage Include from the test adapter and initialize one record per worker.
 */
#pragma once
#include <stdint.h>

/** @brief Fixed worker parameters; busy intervals exclude the blocking period.
 * @details Minimum work must not exceed maximum work; period must be nonzero.
 * @par Usage Configure before releasing the worker startup gate.
 */
struct WorkerConfig_T {
    uint32_t contextId; /**< Independent registered task context. */
    uint32_t periodMs; /**< Delay between work cycles in milliseconds. */
    uint32_t minimumWorkUs; /**< Minimum busy work duration in microseconds. */
    uint32_t maximumWorkUs; /**< Maximum busy work duration in microseconds. */
};
