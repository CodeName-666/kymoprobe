/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
/**
 * @brief Optional application-side Arduino Print adapter.
 *
 * @details This adapter is outside KymoCore. It wraps a copying Print implementation;
 * blocking behavior depends on that implementation. It does not configure baud rate.
 * @file kymo_arduino.h
 * @defgroup example_arduino_stream Arduino Print integration
 * @{
 * @par Usage
 * PrintStream serial(Serial); Kymo sender(serial);
 */
#ifndef EXAMPLE_KYMO_ARDUINO_H
/**
 * @brief Include guard for kymo_arduino.h.
 *
 * @details Internal preprocessor symbol; do not define it in application code.
 * @par Usage
 * Include this header normally; the compiler manages this guard.
 */
#define EXAMPLE_KYMO_ARDUINO_H

#include <Print.h>
#include <kymo_stream.h>

/**
 * @brief Borrow an Arduino Print object through the generic stream interface.
 *
 * @details The Print object must outlive this wrapper. Default busy() is inherited and returns
 * false, so write must copy/consume the supplied data before returning.
 * @par Usage
 * Initialize Serial, construct PrintStream serial(Serial), then pass serial to Kymo.
 */
class PrintStream : public KymoStream {
private:
    /**
     * @brief Borrowed Print object, or nullptr while unattached.
     *
     * @details Read during write; set by construction or attach. It is never deleted by this adapter.
     * @par Usage
     * Use attach rather than modifying this member.
     */
    Print* printObj;

public:
    /**
     * @brief Construct an unattached adapter.
     *
     * @details Writes return zero until a Print object is attached.
     * @par Usage
     * PrintStream stream; stream.attach(Serial);
     */
    PrintStream() : printObj(nullptr) {}

    /**
     * @brief Construct an adapter borrowing a Print object.
     *
     * @details Does not initialize hardware or take ownership.
     * @param[in,out] p Persistent Print object used to consume bytes.
     * @par Usage
     * PrintStream stream(Serial);
     */
    PrintStream(Print& p) : printObj(&p) {}

    /**
     * @brief Replace the borrowed Print object.
     *
     * @details Does not reset a Kymo sender using this stream. Only rebind while its frame
     * is fully accepted and no transfer retains data.
     * @param[in,out] p New persistent Print instance.
     * @par Usage
     * stream.attach(Serial); after safely stopping the old connection.
     */
    void attach(Print& p) { printObj = &p; }

    /**
     * @brief Delegate a byte write to the attached Print implementation.
     *
     * @details Returns zero when unattached. Does not retain the buffer itself; the selected
     * Print implementation must also consume/copy before returning.
     * @param[in] data Readable byte span; valid for length bytes.
     * @param[in] length Requested number of bytes.
     * @return Number accepted by Print, or zero when unattached.
     * @par Usage
     * Called by Kymo; use send/flush to handle short writes.
     */
    size_t write(const uint8_t* data, size_t length) override {
        return printObj ? printObj->write(data, length) : 0;
    }
};

/** @} */
#endif /* EXAMPLE_KYMO_ARDUINO_H */
