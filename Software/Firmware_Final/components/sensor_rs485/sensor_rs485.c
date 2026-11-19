#include "sensor_rs485.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

// ======================================================
// RADAR LUDA/POLIVIR FM02B (LDFM02_B) - MODBUS RTU
// Protocolo: "Flowmeter Radar Protocol Specification Document V3"
// 8N1, direccion 1, funcion 03, CRC16 Modbus (polinomio A001)
// ======================================================

#define SENSOR_UART_PORT UART_NUM_1
#define SENSOR_TX_PIN    GPIO_NUM_1
#define SENSOR_RX_PIN    GPIO_NUM_2
#define SENSOR_DE_RE_PIN GPIO_NUM_4

// Direccion 1 y 115200 confirmados por Bluetooth (19/11/2026).
// (La tabla del manual dice 9600 de fabrica; no aplica a nuestra unidad.)
#define SENSOR_BAUD_RATE 115200
#define SENSOR_BUF_SIZE  256

#define SENSOR_ADDR         0x01
#define SENSOR_TIMEOUT_MS   500

// Registros del radar
#define REG_DISTANCIA       0x0001   // distancia radar -> agua, mm (filtrada)
#define REG_SENAL           0x0004   // intensidad de senal, dB (>30 = buena)

// Fuera de rango del radar (0,1 - 40 m)
#define DISTANCIA_MIN_MM    100

static const char *TAG = "SENSOR_RS485";

// ======================================================
// CRC16 MODBUS
// ======================================================

static uint16_t modbus_crc16(const uint8_t *data, int len)
{
    uint16_t crc = 0xFFFF;

    for (int i = 0; i < len; i++)
    {
        crc ^= data[i];

        for (int b = 0; b < 8; b++)
        {
            if (crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}

// ======================================================
// LECTURA DE UN REGISTRO (FUNCION 03, CANTIDAD 1)
// ======================================================

static bool sensor_rs485_leer_registro(uint16_t reg, uint16_t *valor)
{
    uint8_t cmd[8] = {
        SENSOR_ADDR, 0x03,
        (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF),
        0x00, 0x01,
        0x00, 0x00
    };

    uint16_t crc = modbus_crc16(cmd, 6);
    cmd[6] = crc & 0xFF;   // CRC: byte bajo primero
    cmd[7] = crc >> 8;

    uart_flush_input(SENSOR_UART_PORT);

    // DE/RE lo maneja el UART por hardware (modo RS485 half-duplex)
    uart_write_bytes(SENSOR_UART_PORT, (const char *)cmd, sizeof(cmd));
    uart_wait_tx_done(SENSOR_UART_PORT, pdMS_TO_TICKS(100));

    // Respuesta esperada: 01 03 02 HH LL CRCL CRCH (7 bytes)
    uint8_t rx[7];

    int len = uart_read_bytes(
        SENSOR_UART_PORT,
        rx,
        sizeof(rx),
        pdMS_TO_TICKS(SENSOR_TIMEOUT_MS)
    );

    if (len <= 0)
    {
        ESP_LOGW(TAG, "Sin respuesta del radar (reg 0x%04X)", reg);
        return false;
    }

    if (len >= 5 && rx[0] == SENSOR_ADDR && rx[1] == 0x83)
    {
        ESP_LOGW(TAG, "Excepcion Modbus 0x%02X (reg 0x%04X)", rx[2], reg);
        return false;
    }

    if (len != 7 || rx[0] != SENSOR_ADDR || rx[1] != 0x03 || rx[2] != 0x02)
    {
        ESP_LOGW(TAG, "Respuesta inesperada (reg 0x%04X, %d bytes)", reg, len);
        ESP_LOG_BUFFER_HEX(TAG, rx, len);
        return false;
    }

    uint16_t crc_rx = (uint16_t)rx[5] | ((uint16_t)rx[6] << 8);

    if (crc_rx != modbus_crc16(rx, 5))
    {
        ESP_LOGW(TAG, "CRC invalido (reg 0x%04X)", reg);
        ESP_LOG_BUFFER_HEX(TAG, rx, len);
        return false;
    }

    *valor = ((uint16_t)rx[3] << 8) | rx[4];

    return true;
}

// ======================================================
// INIT
// ======================================================

bool sensor_rs485_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = SENSOR_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(SENSOR_UART_PORT, SENSOR_BUF_SIZE * 2, 0, 0, NULL, 0);

    uart_param_config(SENSOR_UART_PORT, &uart_config);

    // GPIO4 como RTS: el UART conmuta DE/RE del SP3485 al transmitir
    // (mismo esquema que el canal del modem, validado en P3)
    uart_set_pin(
        SENSOR_UART_PORT,
        SENSOR_TX_PIN,
        SENSOR_RX_PIN,
        SENSOR_DE_RE_PIN,
        UART_PIN_NO_CHANGE
    );

    uart_set_mode(SENSOR_UART_PORT, UART_MODE_RS485_HALF_DUPLEX);

    ESP_LOGI(TAG, "Radar RS485 inicializado (%d baudios)", SENSOR_BAUD_RATE);

    // ==============================
    // Lectura de descarte
    // ==============================

    float dummy;

    sensor_rs485_read_level(&dummy);

    vTaskDelay(pdMS_TO_TICKS(500));

    return true;
}

// ======================================================
// LECTURA DE DISTANCIA (d, en metros)
// ======================================================

bool sensor_rs485_read_level(float *level_m)
{
    if (level_m == NULL) return false;

    uint16_t raw_mm;

    if (!sensor_rs485_leer_registro(REG_DISTANCIA, &raw_mm))
    {
        return false;
    }

    if (raw_mm < DISTANCIA_MIN_MM)
    {
        ESP_LOGW(TAG, "Distancia fuera de rango: %u mm", raw_mm);
        return false;
    }

    *level_m = raw_mm / 1000.0f;

    // Senal solo informativa: no invalida la lectura
    uint16_t senal_db;

    if (sensor_rs485_leer_registro(REG_SENAL, &senal_db))
    {
        ESP_LOGI(TAG, "Distancia: %.3f m | senal: %u dB", *level_m, senal_db);
    }
    else
    {
        ESP_LOGI(TAG, "Distancia: %.3f m", *level_m);
    }

    return true;
}
