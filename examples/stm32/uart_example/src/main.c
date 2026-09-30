#include "board.h"

UART_HandleTypeDef huart2;
static PlotterContext plotter;

/*******************************************************************************
 * SysTick_Handler
 ******************************************************************************/
void SysTick_Handler(void) { HAL_IncTick(); }

/*******************************************************************************
 * USART2_IRQHandler
 ******************************************************************************/
void USART2_IRQHandler(void) { HAL_UART_IRQHandler(&huart2); }

/*******************************************************************************
 * uart_init
 ******************************************************************************/
static void uart_init(void) {
    GPIO_InitTypeDef gpio = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_2; // USART2 TX, connect to adapter RX (3.3 V)
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
#if !defined(STM32F103xB)
    gpio.Alternate = GPIO_AF7_USART2;
#endif
    HAL_GPIO_Init(GPIOA, &gpio);
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) while (1) {}
    HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}

/*******************************************************************************
 * main
 ******************************************************************************/
int main(void) {
    // Reset-default HSI clock: no external oscillator/CubeMX files required.
    HAL_Init();
    uart_init();
    if (Plotter_Init(&plotter, &example_config) != PLOTTER_OK) while (1) {}
    while (1) { (void)Plotter_Main(&plotter); }
}
