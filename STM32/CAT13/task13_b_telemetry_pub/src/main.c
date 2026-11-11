/**
 * @file main.c
 * @brief STM32 NUCLEO-F401RE - Task 13B: BME280 I2C Sensor to MQTT Telemetry Publisher
 * @course 25CS2741 Fundamentals of Internet of Things + Lab
 * @institution Shiv Nadar University - Chennai
 * @author Pranavakumar
 * @date 2026-10-27
 *
 * @details
 * Complete end-to-end IoT Edge Publisher node combining the Bosch Sensortec BME280
 * multi-parameter environmental sensor (via I2C1) and ESP8266 Wi-Fi modem running custom
 * MQTT 3.1.1 protocol driver (ESPLib over USART2):
 * 1. Initializes I2C1 (PB6 SCL, PB7 SDA) and verifies BME280 Chip ID (0x60).
 * 2. Reads factory trim compensation parameters and configures sensor oversampling.
 * 3. Initializes ESP8266 on USART2 (115200 baud) in Station mode and connects to local Wi-Fi.
 * 4. Connects to EMQX public MQTT broker (broker.emqx.io:1883) via TCP socket.
 * 5. Sends binary MQTT CONNECT packet (Clean Session, KeepAlive 500s).
 * 6. In main loop: samples calibrated Temperature (°C), Humidity (%RH), Pressure (hPa).
 * 7. Formats JSON payload:
 *    {"status":"success","temperature":28.45,"humidity":62.30,"pressure":1008.72}
 * 8. Publishes payload to topic "stm32/test/data" at QoS 0 every 1 second.
 * 9. Streams telemetry and debug status over USART1 to PC serial monitor.
 *
 * Pin Mapping:
 * - PB6:  I2C1_SCL -> BME280 SCL (4.7k pull-up to 3.3V)
 * - PB7:  I2C1_SDA -> BME280 SDA (4.7k pull-up to 3.3V)
 * - PA2:  USART2_TX -> ESP8266 RX (115200-8-N-1)
 * - PA3:  USART2_RX <- ESP8266 TX
 * - PA9:  USART1_TX -> PC Terminal (115200-8-N-1)
 * - PA10: USART1_RX <- PC Terminal
 * - PA5:  LD2 (Green User LED - Active-HIGH publication strobe)
 * - 3.3V / GND: Common system logic power rail
 */

#include "stm32f4xx_hal.h"
#include "bme280.h"
#include "ESPLib.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define WIFI_SSID       "LAB_IOT_WIFI"
#define WIFI_PASSWORD   "iotlab2026"
#define MQTT_BROKER     "broker.emqx.io"
#define MQTT_PORT       "1883"
#define MQTT_CLIENT_ID  "SNU_STM32_BME280_PUB"
#define MQTT_TOPIC      "stm32/test/data"

/* Peripheral Handles */
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1; /* PC Debug Console */
UART_HandleTypeDef huart2; /* ESP8266 Modem */

/* BME280 Driver Handles */
static struct bme280_dev bme_dev;
static uint8_t dev_addr = BME280_I2C_ADDR_PRIM; /* 0x76 */
static struct bme280_data comp_data;

/* Message Buffers */
static char json_message[128];
static char uart_message[160];

/* Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
void Error_Handler(void);
static void Debug_Print(const char *msg);

/* Bosch Sensor API Callbacks */
static int8_t user_i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len, void *intf_ptr);
static int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *data, uint32_t len, void *intf_ptr);
static void user_delay_us(uint32_t period, void *intf_ptr);
static bool BME280_App_Init(void);
static void MQTT_App_Init(void);
static void Publish_BME280_Telemetry(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    Debug_Print("\r\n=======================================================\r\n");
    Debug_Print("  STM32F401RE Task 13B: BME280 MQTT Telemetry Publisher\r\n");
    Debug_Print("  Topic: stm32/test/data | Broker: broker.emqx.io:1883 \r\n");
    Debug_Print("=======================================================\r\n");

    /* Step 1: Initialize BME280 Sensor */
    if (!BME280_App_Init())
    {
        Debug_Print("[FATAL] BME280 initialization failed! Halting.\r\n");
        Error_Handler();
    }
    Debug_Print("[OK] BME280 sensor initialized (Chip ID: 0x60, Normal Mode).\r\n");

    /* Step 2: Initialize ESP8266 and Connect to MQTT Broker */
    MQTT_App_Init();
    Debug_Print("[READY] Starting periodic 1 Hz MQTT telemetry publisher...\r\n\r\n");

    while (1)
    {
        Publish_BME280_Telemetry();

        /* Strobe Activity LED */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

        /* 1 second polling period */
        HAL_Delay(900);
    }
}

/**
 * @brief Initialize BME280 hardware & Bosch Sensor API
 */
static bool BME280_App_Init(void)
{
    bme_dev.intf = BME280_I2C_INTF;
    bme_dev.read = user_i2c_read;
    bme_dev.write = user_i2c_write;
    bme_dev.delay_us = user_delay_us;
    bme_dev.intf_ptr = &dev_addr;

    int8_t rslt = bme280_init(&bme_dev);
    if (rslt != BME280_OK || bme_dev.chip_id != BME280_CHIP_ID)
    {
        return false;
    }

    struct bme280_settings settings;
    settings.osr_h = BME280_OVERSAMPLING_1X;
    settings.osr_p = BME280_OVERSAMPLING_1X;
    settings.osr_t = BME280_OVERSAMPLING_1X;
    settings.filter = BME280_FILTER_COEFF_4;
    settings.standby_time = BME280_STANDBY_TIME_1000_MS;

    uint8_t settings_sel = BME280_SEL_OSR_PRESS | BME280_SEL_OSR_TEMP |
                           BME280_SEL_OSR_HUM   | BME280_SEL_FILTER   |
                           BME280_SEL_STANDBY;

    bme280_set_sensor_settings(settings_sel, &settings, &bme_dev);
    bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, &bme_dev);

    return true;
}

/**
 * @brief Initialize Wi-Fi connection and establish MQTT broker session
 */
static void MQTT_App_Init(void)
{
    Debug_Print("[INFO] Initializing ESP8266 AT modem on USART2...\r\n");
    if (ESP8266_Init(StationMode, &huart2) != funcOk)
    {
        Debug_Print("[ERROR] ESP8266 AT initialization failed!\r\n");
        Error_Handler();
    }

    Debug_Print("[INFO] Connecting to Wi-Fi AP...\r\n");
    if (ESP8266_wifiConnect(WIFI_SSID, WIFI_PASSWORD) != funcOk)
    {
        Debug_Print("[ERROR] ESP8266 Wi-Fi connection failed!\r\n");
        Error_Handler();
    }

    Debug_Print("[INFO] Connecting to broker.emqx.io:1883...\r\n");
    if (ESP8266_portConnect("TCP", MQTT_BROKER, MQTT_PORT) != funcOk)
    {
        Debug_Print("[ERROR] MQTT broker TCP socket connection failed!\r\n");
        Error_Handler();
    }

    MQTT_InitTypeDef_t mqttSetting;
    mqttSetting.clientID = MQTT_CLIENT_ID;
    mqttSetting.username = NULL;
    mqttSetting.password = NULL;
    mqttSetting.keepAlive = 500U;
    mqttSetting.connectFlag = CleanSession;

    MQTT_connectBroker(&mqttSetting);
    HAL_Delay(500);
    Debug_Print("[OK] MQTT session connected to broker.emqx.io.\r\n");
}

/**
 * @brief Read compensated BME280 telemetry and publish to MQTT topic
 */
static void Publish_BME280_Telemetry(void)
{
    int8_t rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, &bme_dev);
    if (rslt != BME280_OK)
    {
        Debug_Print("[WARN] BME280 measurement read failed.\r\n");
        return;
    }

    double temp = comp_data.temperature;
    double hum  = comp_data.humidity;
    double pres = comp_data.pressure / 100.0; /* Convert Pa to hPa */

    /* Format standard JSON payload */
    int len = snprintf(json_message, sizeof(json_message),
                       "{\"status\":\"success\",\"temperature\":%.2f,\"humidity\":%.2f,\"pressure\":%.2f}",
                       temp, hum, pres);

    if (len <= 0 || (size_t)len >= sizeof(json_message))
    {
        Debug_Print("[ERROR] JSON formatting overflow.\r\n");
        return;
    }

    /* Print diagnostic summary to PC serial monitor */
    snprintf(uart_message, sizeof(uart_message),
             "[BME280] Temp: %.2f C | Hum: %.2f %%RH | Press: %.2f hPa\r\n",
             temp, hum, pres);
    Debug_Print(uart_message);

    /* Publish packet to MQTT Broker */
    MQTT_publishTopic(MQTT_TOPIC, json_message, QoS0);
}

static int8_t user_i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_id = *(uint8_t *)intf_ptr;
    uint16_t hal_addr = (uint16_t)(dev_id << 1);
    if (HAL_I2C_Mem_Read(&hi2c1, hal_addr, reg_addr, I2C_MEMADD_SIZE_8BIT, data, (uint16_t)len, 1000) == HAL_OK)
    {
        return BME280_OK;
    }
    return BME280_E_COMM_FAIL;
}

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

static void user_delay_us(uint32_t period, void *intf_ptr)
{
    (void)intf_ptr;
    HAL_Delay((period / 1000) + 1);
}

static void Debug_Print(const char *msg)
{
    if (msg != NULL)
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)msg, (uint16_t)strlen(msg), 2000);
    }
}

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

static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

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

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* Configure PA5 (LD2 LED) */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

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

/**
 * @brief SysTick Interrupt Handler - Servicing ESPLib MQTT Keep-Alive Timer
 */
void SysTick_Handler(void)
{
    MQTTTimer();
    HAL_IncTick();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(200);
    }
}
