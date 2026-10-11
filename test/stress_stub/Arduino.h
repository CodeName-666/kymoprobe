/** @file Arduino.h
 * @brief Host-only Serial adapter simulating partial writes and backpressure.
 * @details This test fixture uses heap-backed storage to capture output on the PC.
 */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <vector>

extern uint32_t testClockMs;
extern bool testIsBlocked;
extern std::vector<uint8_t> testWire;

/** @brief Minimal SDK surface consumed by the stress application. */
class TestSerial {
public:
    /** @brief Remember the requested baudrate. @param[in] baud UART rate. */
    /***************************************************************************
     * TestSerial::begin
     **************************************************************************/
    void begin(uint32_t baud) { m_baud = baud; }
    /** @brief Accept startup allocation request. @param[in] size TX ring bytes.
     * @return Requested size. */
    /***************************************************************************
     * TestSerial::setTxBufferSize
     **************************************************************************/
    size_t setTxBufferSize(size_t size) { return size; }
    /** @brief Simulate alternating UART stalls. @return Advertised TX capacity. */
    /***************************************************************************
     * TestSerial::availableForWrite
     **************************************************************************/
    int32_t availableForWrite() { return testIsBlocked ? 0 : 32; }
    /** @brief Accept only a short prefix of an advertised write.
     * @param[in] bytes Borrowed bytes. @param[in] length Requested bytes.
     * @return Consumed bytes, at most seven. */
    /***************************************************************************
     * TestSerial::write
     **************************************************************************/
    size_t write(const uint8_t *bytes, size_t length)
    {
        const size_t written = (bytes != nullptr && !testIsBlocked) ?
                               (length < 7u ? length : 7u) : 0u;
        if (written > 0u) {
            testWire.insert(testWire.end(), bytes, bytes + written);
        }
        return written;
    }

public:
    uint32_t m_baud = 0u; /**< Requested rate, exposed for test assertions. */
};

extern TestSerial Serial;

/** @brief Read fake clock. @return Milliseconds modulo 2^32. */
/*******************************************************************************
 * millis
 ******************************************************************************/
static inline uint32_t millis() { return testClockMs; }
/** @brief Advance fake time instead of sleeping. */
/*******************************************************************************
 * yield
 ******************************************************************************/
static inline void yield() { ++testClockMs; }
