# STM32 portability examples

`uart_example` is a self-contained STM32Cube C project. It uses reset HSI clock,
USART2 TX on PA2, 115200/8N1 and interrupt-driven transmission. Init/Main call
only the C library. The HAL IRQ handler releases the borrowed TX buffer.
Targets: Nucleo F401RE, F411RE, Blue Pill F103C8.

`usb_cdc_example` uses STM32duino to supply a complete USB device stack for a
Blue Pill F103C8. It is a different transport adapter around the same C core.
Connect native USB, flash with ST-Link. Its build status is recorded separately.

For an existing CubeMX USB project, use `PlotterConfig.write` to call
`CDC_Transmit_FS` and `busy` to check configured state plus CDC `TxState`.
Do not report the buffer free merely because CDC_Transmit_FS returned OK;
USB still borrows it until completion. Handle disconnected/uninitialized
class state as busy. No HAL/USB headers belong in the portable C core.

Earlier incomplete templates are archived under `legacy/stm32` and excluded
from builds. See [example build instructions](../README.md).

The optional C++ UART/CDC adapters are in `adapters/plotter_stm32.h`, outside
PlotterLib. Include the actual HAL header first for UARTStream and enable its
IRQ. CDCStream requires transmit and busy callbacks. These are application
integration examples, not dependencies of the portable library.
