#include "stm32f1xx_hal.h"

#include <stdio.h>
#include <string.h>

#include "board_config.h"
#include "fault_manager.h"
#include "meter_protocol.h"
#include "modbus_rtu.h"

#ifndef METER_SLAVE_ID
#define METER_SLAVE_ID 1U
#endif

#ifndef METER_POLL_INTERVAL_MS
#define METER_POLL_INTERVAL_MS 1000U
#endif

#define METER_REGISTER_COUNT 10U
#define MODBUS_RESPONSE_LENGTH (5U + METER_REGISTER_COUNT * 2U)

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

static FaultManager fault_manager;
static MeterMeasurements measurements;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void Error_Handler(void);
static void debug_write(const char *text);
static bool poll_meter(void);

void SysTick_Handler(void) {
    HAL_IncTick();
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    fault_manager_init(&fault_manager);
    RS485_RECEIVE_MODE();
    debug_write("\r\nSTM32 Modbus energy terminal started\r\n");

    uint32_t last_poll = HAL_GetTick() - METER_POLL_INTERVAL_MS;
    while (1) {
        const uint32_t now = HAL_GetTick();
        if ((now - last_poll) >= METER_POLL_INTERVAL_MS) {
            last_poll = now;
            if (poll_meter()) {
                fault_manager_record_success(&fault_manager);
                HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, GPIO_PIN_RESET);
                char line[196];
                (void)snprintf(
                    line,
                    sizeof(line),
                    "OK Ua=%.1fV Ub=%.1fV Uc=%.1fV Ia=%.1fA Ib=%.1fA Ic=%.1fA P=%.1fkW PF=%.3f F=%.2fHz Load=%.1f%%\r\n",
                    measurements.phase_voltage_v[0], measurements.phase_voltage_v[1], measurements.phase_voltage_v[2],
                    measurements.phase_current_a[0], measurements.phase_current_a[1], measurements.phase_current_a[2],
                    measurements.active_power_kw, measurements.power_factor, measurements.frequency_hz,
                    measurements.load_rate_pct);
                debug_write(line);
            } else {
                fault_manager_record_failure(&fault_manager);
                HAL_GPIO_TogglePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin);
                char line[96];
                (void)snprintf(
                    line,
                    sizeof(line),
                    "ERR poll failed consecutive=%u total=%lu status=%u\r\n",
                    (unsigned int)fault_manager.consecutive_failures,
                    (unsigned long)fault_manager.failed_polls,
                    (unsigned int)fault_manager.status);
                debug_write(line);
            }
        }
    }
}

static bool poll_meter(void) {
    uint8_t request[8];
    uint8_t response[MODBUS_RESPONSE_LENGTH];
    const size_t request_length = modbus_build_read_input_request(
        METER_SLAVE_ID, 0U, METER_REGISTER_COUNT, request, sizeof(request));
    if (request_length == 0U) {
        return false;
    }

    RS485_TRANSMIT_MODE();
    HAL_Delay(1U);
    const HAL_StatusTypeDef tx_status = HAL_UART_Transmit(&huart2, request, (uint16_t)request_length, 100U);
    if (tx_status != HAL_OK) {
        RS485_RECEIVE_MODE();
        return false;
    }
    const uint32_t wait_started = HAL_GetTick();
    while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) == RESET) {
        if ((HAL_GetTick() - wait_started) > 10U) {
            RS485_RECEIVE_MODE();
            return false;
        }
    }
    RS485_RECEIVE_MODE();

    memset(response, 0, sizeof(response));
    const HAL_StatusTypeDef rx_status = HAL_UART_Receive(&huart2, response, sizeof(response), 300U);
    if (rx_status != HAL_OK) {
        return false;
    }
    if (!modbus_validate_read_response(response, sizeof(response), METER_SLAVE_ID, METER_REGISTER_COUNT)) {
        return false;
    }
    return meter_parse_input_registers(response, sizeof(response), &measurements);
}

static void debug_write(const char *text) {
    if (text == NULL) {
        return;
    }
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)text, (uint16_t)strlen(text), 200U);
}

void HAL_UART_MspInit(UART_HandleTypeDef *uart_handle) {
    GPIO_InitTypeDef gpio = {0};
    if (uart_handle->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        gpio.Pin = GPIO_PIN_9;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &gpio);
        gpio.Pin = GPIO_PIN_10;
        gpio.Mode = GPIO_MODE_INPUT;
        HAL_GPIO_Init(GPIOA, &gpio);
    } else if (uart_handle->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        gpio.Pin = GPIO_PIN_2;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &gpio);
        gpio.Pin = GPIO_PIN_3;
        gpio.Mode = GPIO_MODE_INPUT;
        HAL_GPIO_Init(GPIOA, &gpio);
    }
}

static void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    HAL_GPIO_WritePin(RS485_DE_GPIO_Port, RS485_DE_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STATUS_LED_GPIO_Port, STATUS_LED_Pin, GPIO_PIN_SET);
    gpio.Pin = RS485_DE_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(RS485_DE_GPIO_Port, &gpio);
    gpio.Pin = STATUS_LED_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STATUS_LED_GPIO_Port, &gpio);
}

static void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 9600;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

static void SystemClock_Config(void) {
    RCC_OscInitTypeDef oscillator = {0};
    RCC_ClkInitTypeDef clock = {0};

    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    oscillator.HSIState = RCC_HSI_ON;
    oscillator.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    oscillator.PLL.PLLMUL = RCC_PLL_MUL16;
    if (HAL_RCC_OscConfig(&oscillator) != HAL_OK) {
        Error_Handler();
    }

    clock.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clock.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clock.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clock.APB1CLKDivider = RCC_HCLK_DIV2;
    clock.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clock, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

static void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}
