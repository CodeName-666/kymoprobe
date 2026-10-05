/* SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KymoCore-Commercial
 * Copyright (c) 2026 Christof Seidel */
/**
 * @brief Optional STM32 application adapters for the generic C++ stream API.
 *
 * @details Include the actual HAL header before this file to expose UARTStream when
 * HAL_UART_MODULE_ENABLED is defined. SDK dependencies stay outside KymoCore.
 * Both adapters borrow transmit buffers and therefore implement busy().
 * @file kymo_stm32.h
 * @defgroup example_stm32_stream STM32 stream integration
 * @{
 * @par Usage
 * Configure peripherals/IRQs first, construct an adapter, then bind it to Kymo.
 */
#ifndef KYMO_STM32_H
/**
 * @brief Include guard for kymo_stm32.h.
 *
 * @details Internal preprocessor symbol; do not define it in application code.
 * @par Usage
 * Include this header normally; the compiler manages this guard.
 */
#define KYMO_STM32_H

#include "kymo_stream.h"

#if defined(HAL_UART_MODULE_ENABLED)
/**
 * @brief Interrupt-driven HAL UART adapter with explicit completion state.
 *
 * @details Requires a configured handle and enabled UART IRQ forwarding to HAL_UART_IRQHandler.
 * One producer must own the UART; the adapter does not configure or shut down hardware.
 * @par Usage
 * UARTStream uart(&huart2); Kymo sender(uart);
 */
class UARTStream : public KymoStream {
    /**
     * @brief Borrowed HAL handle used by writes and readiness checks.
     *
     * @details Input/output driver state; must outlive this adapter and its active transfers.
     * @par Usage
     * Supply the initialized peripheral handle to the constructor.
     */
    UART_HandleTypeDef *handle;
public:
    /**
     * @brief Bind an initialized UART handle without taking ownership.
     *
     * @details A null handle is allowed but keeps the adapter busy and unable to transmit.
     * @param[in,out] uart Persistent HAL UART handle or nullptr for an unavailable adapter.
     * @par Usage
     * UARTStream uart(&huart2);
     */
    explicit UARTStream(UART_HandleTypeDef *uart) : handle(uart) {}
    /**
     * @brief Check whether HAL permits a new transmit buffer.
     *
     * @details Reports busy for a null handle or any gState other than HAL_UART_STATE_READY.
     * HAL IRQ processing must update state after completion.
     * @return True when unavailable/in flight; false when ready.
     * @par Usage
     * Called by Kymo before encoding or advancing writes.
     */
    bool busy() const override {
        return !handle || handle->gState != HAL_UART_STATE_READY;
    }
    /**
     * @brief Start a nonblocking UART interrupt transfer of the supplied span.
     *
     * @details Accepts the whole span or zero. A successful HAL call retains the buffer until
     * UART completion; busy protects its lifetime. No bytes are copied by this wrapper.
     * @param[in] data Readable buffer retained by HAL after acceptance; must remain alive/unchanged.
     * @param[in] length Requested size; values above UINT16_MAX are rejected.
     * @return length when HAL accepts the transfer; zero when busy, oversized or HAL rejects it.
     * @pre UART IRQ forwarding is enabled and the span is valid for length bytes.
     * @par Usage
     * Call through Kymo so its persistent member buffer remains protected.
     */
    size_t write(const uint8_t *data, size_t length) override {
        size_t written = 0;
        if (!busy() && (length <= UINT16_MAX)) {
            if (HAL_UART_Transmit_IT(handle, const_cast<uint8_t *>(data),
                                    static_cast<uint16_t>(length)) == HAL_OK) {
                written = length;
            }
        }
        return written;
    }
};
#endif

/**
 * @brief CubeMX-style USB CDC adapter with application-supplied readiness.
 *
 * @details The transmit function retains the frame. The busy function must report true while
 * USB is unconfigured/disconnected or CDC TxState is nonzero. Only one producer may
 * use the CDC endpoint; both function pointers are required for a usable adapter.
 * @par Usage
 * CDCStream usb(CDC_Transmit_FS, application_usb_busy); Kymo sender(usb);
 */
class CDCStream : public KymoStream {
public:
    /**
     * @brief USB transmit function matching the CubeMX CDC call shape.
     *
     * @details The mutable pointer matches the SDK signature; implementations must treat payload
     * bytes as read-only. Acceptance borrows the span until the busy callback clears.
     * @param[in] buffer Payload span retained on success; valid for length bytes.
     * @param[in] length Requested payload size representable in uint16_t.
     * @return Zero when accepted; nonzero for busy/error.
     * @par Usage
     * Pass CDC_Transmit_FS or an equivalent application wrapper.
     */
    typedef uint8_t (*TransmitFn)(uint8_t *buffer, uint16_t length);
    /**
     * @brief USB readiness and transmit-completion query.
     *
     * @details Takes no parameters; access application USB state through a free/static wrapper.
     * Must protect an accepted transmit span until the SDK releases it.
     * @return True if disconnected, unconfigured or transmitting; false when buffer reuse is safe.
     * @par Usage
     * Pass a function checking USB configuration and CDC TxState.
     */
    typedef bool (*BusyFn)();
    /**
     * @brief Bind USB transmit and readiness callbacks.
     *
     * @details Does not initialize USB. Null callbacks make the adapter permanently busy.
     * @param[in] transmitFn Persistent transmit entrypoint; zero means accepted.
     * @param[in] busyFn Readiness/completion function protecting borrowed payload data.
     * @par Usage
     * CDCStream usb(CDC_Transmit_FS, usb_busy);
     */
    CDCStream(TransmitFn transmitFn, BusyFn busyFn)
        : transmit(transmitFn), isBusy(busyFn) {}
    /**
     * @brief Query availability and borrowed-buffer ownership.
     *
     * @details Missing callbacks are treated as unavailable without dereferencing them.
     * @return True if either callback is missing or the application reports busy.
     * @par Usage
     * Called by Kymo before reusing the frame buffer.
     */
    bool busy() const override { return !transmit || !isBusy || isBusy(); }
    /**
     * @brief Submit one complete USB payload when ready.
     *
     * @details Accepts the entire span or zero. Does not copy data; busyFn must remain true
     * until the USB stack releases the pointer after successful submission.
     * @param[in] data Readable persistent span; must remain alive/unchanged until completion.
     * @param[in] length Byte count; values greater than UINT16_MAX are rejected.
     * @return length on accepted transfer; zero for busy, oversized or rejected transfer.
     * @par Usage
     * Use through Kymo to retain the frame while USB is transmitting.
     */
    size_t write(const uint8_t *data, size_t length) override {
        size_t written = 0;
        if (!busy() && (length <= UINT16_MAX)) {
            if (transmit(const_cast<uint8_t *>(data), static_cast<uint16_t>(length)) == 0) {
                written = length;
            }
        }
        return written;
    }
private:
    /**
     * @brief Borrowed USB transmit function pointer.
     *
     * @details Set by construction; NULL disables the adapter.
     * @par Usage
     * Supply the application transmit entrypoint when constructing CDCStream.
     */
    TransmitFn transmit;
    /**
     * @brief Borrowed USB readiness/completion function pointer.
     *
     * @details Set by construction; NULL prevents writes to protect buffer lifetime.
     * @par Usage
     * Supply a callback covering both connection state and active transfer state.
     */
    BusyFn isBusy;
};

/** @} */
#endif /* KYMO_STM32_H */
