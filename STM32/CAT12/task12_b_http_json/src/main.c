/**
 * @file main.c
 * @brief STM32 NUCLEO-F401RE - Task 12B: BME280 Environmental HTTP JSON Server via ESP8266
 * @course 25CS2741 Fundamentals of Internet of Things + Lab
 * @institution Shiv Nadar University - Chennai
 * @author Pranavakumar
 * @date 2026-10-23
 *
 * @details
 * Combines the Bosch Sensortec BME280 I2C sensor driver with an ESP8266 Wi-Fi modem
 * running AT firmware to host an embedded HTTP JSON REST endpoint:
 * 1. Samples calibrated Temperature, Pressure, and Humidity via I2C1 (PB6/PB7).
 * 2. Communicates with ESP8266 Wi-Fi module over USART1 (PA9 TX, PA10 RX) at 115200 baud.
 * 3. Joins local Wi-Fi AP in Station mode (AT+CWMODE=1, AT+CWJAP).
 * 4. Starts multi-connection TCP server on port 80 (AT+CIPMUX=1, AT+CIPSERVER=1,80).
 * 5. Handles incoming HTTP GET requests (+IPD,<link_id>,<len>:GET ...).
 * 6. Dynamically constructs standard JSON payload and computes exact Content-Length.
 * 7. Streams HTTP/1.1 200 OK response via two-step AT+CIPSEND=<id>,<len> and closes connection.
 * 8. Streams live telemetry and HTTP server status diagnostics to PC via USART2 (PA2/PA3).
 *
 * Pin Mapping:
 * - PB6:  I2C1_SCL -> BME280 SCL (4.7k pull-up to 3.3V)
 * - PB7:  I2C1_SDA -> BME280 SDA (4.7k pull-up to 3.3V)
 * - PA9:  USART1_TX -> ESP8266 RX (through voltage divider or 3.3V tolerant)
 * - PA10: USART1_RX <- ESP8266 TX
 * - PA2:  USART2_TX -> PC Virtual COM Port (115200-8-N-1)
 * - PA3:  USART2_RX <- PC Virtual COM Port
 * - PA5:  LD2 (Green LED - Active-HIGH HTTP activity indicator)
 * - 3.3V / GND: Power supply rail commoned across STM32, ESP8266, and BME280
 */

#include "stm32f4xx_hal.h"
#include "bme280.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define WIFI_SSID       "LAB_IOT_WIFI"
#define WIFI_PASSWORD   "iotlab2026"
#define HTTP_SERVER_PORT 80
#define RX_BUFFER_SIZE  1024
#define HTTP_RESP_SIZE  512
#define JSON_BUF_SIZE   256

/* Peripheral Handles */
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1; /* ESP8266 AT Modem */
UART_HandleTypeDef huart2; /* PC Debug Console */

/* BME280 Driver Handles */
static struct bme280_dev bme_dev;
static uint8_t dev_addr = BME280_I2C_ADDR_PRIM; /* 0x76 */
static struct bme280_data comp_data;

/* UART Circular / DMA Buffers */
static uint8_t rx_byte;
static char esp_rx_buffer[RX_BUFFER_SIZE];
static volatile uint16_t esp_rx_index = 0;

/* Function Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
void Error_Handler(void);

/* Diagnostic & AT Helpers */
static void UART_Log(const char *str);
static void ESP_Send(const char *str);
static bool ESP_WaitForPattern(const char *pattern, uint32_t timeout_ms);
static void ESP_ClearBuffer(void);
static bool ESP_Init_Server(void);
static void Process_HTTP_Request(void);

/* Bosch Sensor API Callbacks */
static int8_t user_i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len, void *intf_ptr);
static int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *data, uint32_t len, void *intf_ptr);
static void user_delay_us(uint32_t period, void *intf_ptr);
static bool BME280_App_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    UART_Log("\r\n=======================================================\r\n");
    UART_Log("  STM32F401RE Task 12B: BME280 HTTP JSON REST Server   \r\n");
    UART_Log("=======================================================\r\n");

    /* Initialize BME280 Sensor */
    if (!BME280_App_Init())
    {
        UART_Log("[FATAL] BME280 initialization failed! Halting.\r\n");
        while (1)
        {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            HAL_Delay(200);
        }
    }
    UART_Log("[INFO] BME280 calibrated and configured in Normal Mode.\r\n");

    /* Start non-blocking UART receive from ESP8266 */
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

    /* Initialize ESP8266 TCP Web Server */
    UART_Log("[INFO] Configuring ESP8266 Wi-Fi and TCP Server on Port 80...\r\n");
    if (!ESP_Init_Server())
    {
        UART_Log("[ERROR] ESP8266 Server setup failed! Re-trying...\r\n");
    }
    else
    {
        UART_Log("[READY] HTTP JSON Server live on port 80!\r\n");
        UART_Log("[READY] Awaiting incoming GET / requests...\r\n\r\n");
    }

    uint32_t last_sensor_read = 0;

    while (1)
    {
        /* Periodic local sensor read every 1000ms */
        if (HAL_GetTick() - last_sensor_read >= 1000)
        {
            last_sensor_read = HAL_GetTick();
            if (bme280_get_sensor_data(BME280_ALL, &comp_data, &bme_dev) == BME280_OK)
            {
                /* Telemetry heartbeat */
                char dbg[96];
                snprintf(dbg, sizeof(dbg), "[BME280] T: %.2f C | P: %.2f hPa | H: %.2f %%RH\r\n",
                         comp_data.temperature,
                         comp_data.pressure / 100.0,
                         comp_data.humidity);
                UART_Log(dbg);
            }
        }

        /* Check for pending incoming HTTP connections */
        Process_HTTP_Request();
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
    if (rslt != BME280_OK)
    {
        return false;
    }

    struct bme280_settings settings;
    settings.osr_h = BME280_OVERSAMPLING_1X;
    settings.osr_p = BME280_OVERSAMPLING_16X;
    settings.osr_t = BME280_OVERSAMPLING_2X;
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
 * @brief Configures ESP8266 as Wi-Fi Station and Port 80 Multi-Client Server
 */
static bool ESP_Init_Server(void)
{
    ESP_ClearBuffer();
    ESP_Send("AT\r\n");
    if (!ESP_WaitForPattern("OK", 2000)) return false;

    ESP_ClearBuffer();
    ESP_Send("ATE0\r\n");
    ESP_WaitForPattern("OK", 1000);

    ESP_ClearBuffer();
    ESP_Send("AT+CWMODE=1\r\n");
    if (!ESP_WaitForPattern("OK", 2000)) return false;

    char join_cmd[128];
    snprintf(join_cmd, sizeof(join_cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASSWORD);
    ESP_ClearBuffer();
    ESP_Send(join_cmd);
    ESP_WaitForPattern("OK", 15000);

    ESP_ClearBuffer();
    ESP_Send("AT+CIPMUX=1\r\n");
    if (!ESP_WaitForPattern("OK", 2000)) return false;

    char srv_cmd[48];
    snprintf(srv_cmd, sizeof(srv_cmd), "AT+CIPSERVER=1,%d\r\n", HTTP_SERVER_PORT);
    ESP_ClearBuffer();
    ESP_Send(srv_cmd);
    if (!ESP_WaitForPattern("OK", 2000)) return false;

    ESP_ClearBuffer();
    ESP_Send("AT+CIFSR\r\n");
    ESP_WaitForPattern("OK", 3000);

    return true;
}

/**
 * @brief Parses incoming +IPD TCP packets and serves HTTP JSON responses
 */
static void Process_HTTP_Request(void)
{
    char *ipd_ptr = strstr(esp_rx_buffer, "+IPD,");
    if (ipd_ptr == NULL) return;

    /* Turn ON activity LED */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

    /* Parse connection ID (+IPD,<link_id>,<len>:...) */
    int link_id = 0;
    if (sscanf(ipd_ptr, "+IPD,%d,", &link_id) != 1)
    {
        link_id = 0;
    }

    char dbg_req[64];
    snprintf(dbg_req, sizeof(dbg_req), "[HTTP] Incoming request on link %d. Serving JSON...\r\n", link_id);
    UART_Log(dbg_req);

    /* Acquire latest fresh BME280 sensor data */
    bme280_get_sensor_data(BME280_ALL, &comp_data, &bme_dev);

    /* 1. Format dynamic JSON payload */
    char json_payload[JSON_BUF_SIZE];
    snprintf(json_payload, sizeof(json_payload),
             "{\"status\":\"success\",\"temperature\":%.2f,\"humidity\":%.2f,\"pressure\":%.2f}\r\n",
             comp_data.temperature,
             comp_data.humidity,
             comp_data.pressure / 100.0);

    int content_length = (int)strlen(json_payload);

    /* 2. Format complete HTTP/1.1 response with exact Content-Length */
    char http_response[HTTP_RESP_SIZE];
    snprintf(http_response, sizeof(http_response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "\r\n"
             "%s",
             content_length,
             json_payload);

    int total_response_len = (int)strlen(http_response);

    /* 3. Send AT+CIPSEND=<id>,<len> */
    char send_cmd[48];
    snprintf(send_cmd, sizeof(send_cmd), "AT+CIPSEND=%d,%d\r\n", link_id, total_response_len);
    ESP_ClearBuffer();
    ESP_Send(send_cmd);

    /* Wait for '>' prompt */
    if (ESP_WaitForPattern(">", 2000))
    {
        /* Transmit response body */
        ESP_Send(http_response);
        ESP_WaitForPattern("SEND OK", 3000);
    }

    /* 4. Close TCP connection */
    char close_cmd[32];
    snprintf(close_cmd, sizeof(close_cmd), "AT+CIPCLOSE=%d\r\n", link_id);
    ESP_Send(close_cmd);
    ESP_WaitForPattern("OK", 1000);

    /* Clear receive buffer and reset index */
    ESP_ClearBuffer();

    /* Turn OFF activity LED */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
}

/**
 * @brief USART RX Interrupt Callback
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        if (esp_rx_index < RX_BUFFER_SIZE - 1)
        {
            esp_rx_buffer[esp_rx_index++] = (char)rx_byte;
            esp_rx_buffer[esp_rx_index] = '\0';
        }
        else
        {
            esp_rx_index = 0; /* Wrap around on overflow */
        }
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

static void ESP_Send(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, (uint16_t)strlen(str), 1000);
}

static void ESP_ClearBuffer(void)
{
    memset(esp_rx_buffer, 0, sizeof(esp_rx_buffer));
    esp_rx_index = 0;
}

static bool ESP_WaitForPattern(const char *pattern, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < timeout_ms)
    {
        if (strstr(esp_rx_buffer, pattern) != NULL)
        {
            return true;
        }
        HAL_Delay(10);
    }
    return false;
}

static void UART_Log(const char *str)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)str, (uint16_t)strlen(str), 1000);
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

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
