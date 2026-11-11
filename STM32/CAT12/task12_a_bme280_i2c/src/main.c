/**
 * @file main.c
 * @brief STM32 NUCLEO-F401RE - Task 12A: BME280 I2C Sensor Interface with Bosch Sensor API
 * @course 25CS2741 Fundamentals of Internet of Things + Lab
 * @institution Shiv Nadar University - Chennai
 * @author Pranavakumar
 * @date 2026-10-23
 *
 * @details
 * Integrates the official Bosch Sensortec BME280 Sensor API over I2C1 (PB6 SCL, PB7 SDA)
 * on the ARM Cortex-M4 NUCLEO-F401RE platform:
 * 1. Implements custom HAL I2C read, write, and delay callback wrappers.
 * 2. Initializes the sensor, confirms device presence via chip ID (0x60).
 * 3. Configures oversampling (Temp x2, Press x16, Hum x1), IIR filter (coeff 4), and standby time (1000 ms).
 * 4. Puts sensor into Normal measurement mode.
 * 5. Acquires compensated temperature, pressure, and humidity every 1 second and streams telemetry
 *    formatted over USART2 to the PC terminal.
 *
 * Pin Mapping:
 * - PB6:  I2C1_SCL -> BME280 SCL (with 4.7k pull-up to 3.3V)
 * - PB7:  I2C1_SDA -> BME280 SDA (with 4.7k pull-up to 3.3V)
 * - PA2:  USART2_TX -> PC Terminal (115200-8-N-1)
 * - PA3:  USART2_RX <- PC Terminal
 * - 3.3V: VDD logic supply & CSB pin tied to 3.3V (selects I2C mode)
 * - GND:  Common Ground & SDO pin tied to GND (sets 7-bit address 0x76)
 */

#include "stm32f4xx_hal.h"
#include "bme280.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* Peripheral Handles */
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

/* BME280 Sensor Device Structure */
static struct bme280_dev bme_dev;
static uint8_t dev_addr = BME280_I2C_ADDR_PRIM; /* 0x76 (SDO=GND) */

/* Private Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
static void UART_Log(const char *str);

/* Bosch Sensor API Interface Callbacks */
static int8_t user_i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len, void *intf_ptr);
static int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *data, uint32_t len, void *intf_ptr);
static void user_delay_us(uint32_t period, void *intf_ptr);

void Error_Handler(void);

/**
 * @brief Application entry point.
 * @retval int
 */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();

    UART_Log("\r\n=======================================================\r\n");
    UART_Log("  STM32F401RE - Task 12A: BME280 Bosch Sensor API Test \r\n");
    UART_Log("  I2C1: PB6 (SCL) / PB7 (SDA) | Target Addr: 0x76      \r\n");
    UART_Log("=======================================================\r\n");

    /* Step 1: Populate Bosch API Function Pointers and Interface Context */
    bme_dev.intf = BME280_I2C_INTF;
    bme_dev.read = user_i2c_read;
    bme_dev.write = user_i2c_write;
    bme_dev.delay_us = user_delay_us;
    bme_dev.intf_ptr = &dev_addr;

    /* Step 2: Initialize the Sensor & Read Calibration Parameters */
    int8_t rslt = bme280_init(&bme_dev);
    if (rslt != BME280_OK)
    {
        char err_msg[64];
        snprintf(err_msg, sizeof(err_msg), "[ERROR] BME280 Init Failed! Code: %d\r\n", rslt);
        UART_Log(err_msg);
        UART_Log("Check I2C wiring (PB6/PB7) and pull-up resistors.\r\n");
        while (1) { }
    }

    /* Step 3: Verify Chip ID (Expected: 0x60 for BME280) */
    char id_msg[64];
    snprintf(id_msg, sizeof(id_msg), "[SUCCESS] BME280 Initialized! Chip ID: 0x%02X\r\n", bme_dev.chip_id);
    UART_Log(id_msg);

    /* Step 4: Configure Sensor Oversampling, Standby Time, and IIR Filter */
    struct bme280_settings settings;
    settings.osr_h = BME280_OVERSAMPLING_1X;
    settings.osr_p = BME280_OVERSAMPLING_16X;
    settings.osr_t = BME280_OVERSAMPLING_2X;
    settings.filter = BME280_FILTER_COEFF_4;
    settings.standby_time = BME280_STANDBY_TIME_1000_MS;

    uint8_t settings_sel = BME280_SEL_OSR_PRESS | BME280_SEL_OSR_TEMP |
                           BME280_SEL_OSR_HUM   | BME280_SEL_FILTER   |
                           BME280_SEL_STANDBY;

    rslt = bme280_set_sensor_settings(settings_sel, &settings, &bme_dev);
    if (rslt != BME280_OK)
    {
        UART_Log("[WARN] Failed to configure sensor settings.\r\n");
    }

    /* Step 5: Activate Normal Measurement Mode */
    rslt = bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, &bme_dev);
    if (rslt != BME280_OK)
    {
        UART_Log("[ERROR] Failed to set normal mode.\r\n");
        while (1) { }
    }

    UART_Log("[READY] Sensor in Normal Mode. Streaming telemetry @ 1 Hz:\r\n\r\n");

    struct bme280_data comp_data;
    char log_buf[128];

    /* Measurement Loop */
    while (1)
    {
        /* Read compensated telemetry from the sensor */
        rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, &bme_dev);
        if (rslt == BME280_OK)
        {
            /* Format: Temperature (°C), Pressure (hPa), Humidity (%RH) */
            snprintf(log_buf, sizeof(log_buf),
                     "Temp: %.2f degC  |  Press: %.2f hPa  |  Hum: %.2f %%RH\r\n",
                     comp_data.temperature,
                     comp_data.pressure / 100.0, /* Convert Pa to hPa */
                     comp_data.humidity);
            UART_Log(log_buf);
        }
        else
        {
            UART_Log("[WARN] Failed to read sensor data.\r\n");
        }

        /* 1 second sample interval */
        HAL_Delay(1000);
    }
}

/**
 * @brief  Bosch API I2C read callback.
 * @param  reg_addr Starting register address to read.
 * @param  data Pointer to data buffer.
 * @param  len Number of bytes to read.
 * @param  intf_ptr User interface pointer containing device 7-bit address.
 * @retval 0 for success, non-zero for communication failure.
 */
static int8_t user_i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_id = *(uint8_t *)intf_ptr;
    /* Shift 7-bit address to 8-bit format for STM32 HAL */
    uint16_t hal_addr = (uint16_t)(dev_id << 1);

    if (HAL_I2C_Mem_Read(&hi2c1, hal_addr, reg_addr, I2C_MEMADD_SIZE_8BIT, data, (uint16_t)len, 1000) == HAL_OK)
    {
        return BME280_OK;
    }
    return BME280_E_COMM_FAIL;
}

/**
 * @brief  Bosch API I2C write callback.
 * @param  reg_addr Starting register address to write.
 * @param  data Pointer to data buffer.
 * @param  len Number of bytes to write.
 * @param  intf_ptr User interface pointer containing device 7-bit address.
 * @retval 0 for success, non-zero for communication failure.
 */
static int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_id = *(uint8_t *)intf_ptr;
    uint16_t hal_addr = (uint16_t)(dev_id << 1);

    if (HAL_I2C_Mem_Write(&hi2c1, hal_addr, reg_addr, I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, (uint16_t)len, 1000) == HAL_OK)
    {
        return BME280_OK;
    }
    return BME280_E_COMM_FAIL;
}

/**
 * @brief  Bosch API delay callback in microseconds.
 */
static void user_delay_us(uint32_t period, void *intf_ptr)
{
    (void)intf_ptr;
    HAL_Delay((period / 1000) + 1);
}

/**
 * @brief Transmits string over USART2 (PC Virtual COM Port).
 */
static void UART_Log(const char *str)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)str, (uint16_t)strlen(str), 1000);
}

/**
 * @brief I2C1 Initialization Function (Standard Mode 100 kHz)
 */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief USART2 Initialization Function (115200-8-N-1)
 */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief GPIO Initialization Function
 */
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
}

/**
 * @brief System Clock Configuration (HSI -> PLL -> 84 MHz)
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
