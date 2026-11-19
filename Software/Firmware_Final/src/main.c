#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "reloj.h"
#include "led_blink.h"
#include "sdcard.h"
#include "sensor_rs485.h"
#include "wifi_envio.h"

static const char *TAG = "MAIN";
static SemaphoreHandle_t envio_mutex;

// Paraguay: UTC-3 fijo (el RTC guarda hora local; el epoch enviado es UTC)
#define OFFSET_PY_S     (3 * 3600)
#define EPOCH_MINIMO    1735689600UL   // 2025-01-01: timestamps anteriores son basura

// ======================================================
// SINCRONIZAR EL RTC POR NTP (si hay WiFi)
// ======================================================

static bool sincronizar_rtc(void)
{
    uint32_t utc;

    if (!wifi_hora_utc(&utc))
    {
        return false;
    }

    time_t local = (time_t)(utc - OFFSET_PY_S);
    struct tm t;
    gmtime_r(&local, &t);

    rtc_datetime_t dt = {
        .year   = (uint16_t)(t.tm_year + 1900),
        .month  = (uint8_t)(t.tm_mon + 1),
        .day    = (uint8_t)t.tm_mday,
        .hour   = (uint8_t)t.tm_hour,
        .minute = (uint8_t)t.tm_min,
        .second = (uint8_t)t.tm_sec,
    };

    if (!reloj_set(&dt))
    {
        return false;
    }

    char fecha[32];
    reloj_format(&dt, fecha, sizeof(fecha));
    ESP_LOGI(TAG, "RTC sincronizado por NTP: %s (hora PY)", fecha);

    return true;
}


// ======================================================
// ENVIO WIFI PARA CICLO PRINCIPAL
// Espera mas tiempo porque tiene prioridad.
// ======================================================

static bool envio_level_main(uint32_t timestamp, float nivel)
{
    bool ok = false;

    if (xSemaphoreTake(envio_mutex, pdMS_TO_TICKS(30000)) == pdTRUE)
    {
        ok = wifi_send_level(timestamp, nivel);
        xSemaphoreGive(envio_mutex);
    }
    else
    {
        ESP_LOGW(TAG, "Envio ocupado. No se pudo enviar dato actual");
    }

    return ok;
}

// ======================================================
// ENVIO WIFI PARA PENDIENTES
// Espera poco. Si el envio esta ocupado, no molesta al ciclo principal.
// ======================================================

static bool envio_level_bg(uint32_t timestamp, float nivel)
{
    bool ok = false;

    if (xSemaphoreTake(envio_mutex, pdMS_TO_TICKS(1000)) == pdTRUE)
    {
        ok = wifi_send_level(timestamp, nivel);
        xSemaphoreGive(envio_mutex);
    }
    else
    {
        ESP_LOGW(TAG, "Envio ocupado. Pendiente queda en cola");
    }

    return ok;
}

// ======================================================
// TASK DE PENDIENTES EN SEGUNDO PLANO
// ======================================================
#define MARGEN_BG_SLOT_S 35
static void task_pendientes(void *pv)
{
    while (1)
    {
        rtc_datetime_t dt;

        if (reloj_now(&dt))
        {
            uint32_t espera_s = reloj_segundos_hasta_proximo_slot_5min(&dt);

            if (espera_s > MARGEN_BG_SLOT_S)
            {
                uint32_t ts;
                float nivel;

                if (sdcard_has_pending() &&
                    sdcard_read_first_pending(&ts, &nivel) &&
                    ts < EPOCH_MINIMO)
                {
                    // Dato guardado con el RTC sin hora (ano 2000): no se envia
                    ESP_LOGW(TAG,
                             "BG descarta pendiente con fecha invalida: d=%lu,%.3f",
                             (unsigned long)ts,
                             nivel);
                    sdcard_remove_first_pending();
                }
                else if (sdcard_has_pending() &&
                         sdcard_read_first_pending(&ts, &nivel))
                {
                    ESP_LOGI(TAG,
                             "BG reenviando pendiente: d=%lu,%.3f",
                             (unsigned long)ts,
                             nivel);

                    if (envio_level_bg(ts, nivel))
                    {
                        sdcard_remove_first_pending();
                        ESP_LOGI(TAG, "BG pendiente enviado y eliminado");
                    }
                    else
                    {
                        ESP_LOGW(TAG, "BG fallo reenvio. Cola conservada");
                    }
                }
            }
            else
            {
                ESP_LOGI(TAG,
                         "BG no envia: faltan %lu s para el proximo slot",
                         (unsigned long)espera_s);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// ======================================================
// APP MAIN
// ======================================================

void app_main(void)
{   


    
    ESP_LOGI(TAG, "Iniciando sistema solo nivel...");

    reloj_init();
    sdcard_init();
    sensor_rs485_init();
    wifi_init();
    led_init();
    envio_mutex = xSemaphoreCreateMutex();

    sincronizar_rtc();

    xTaskCreate(
             task_pendientes,
             "task_pendientes",
             8192,
             NULL,
             4,
            NULL
              );

    while (1)
    {
        rtc_datetime_t dt;

        if (!reloj_now(&dt))
        {
            ESP_LOGE(TAG, "No se pudo leer RTC");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        if (!reloj_hora_valida(&dt))
        {
            // Sin hora real no se mide: el dato quedaria con fecha falsa
            ESP_LOGE(TAG, "RTC sin hora valida (%04d). Reintentando NTP en 60 s", dt.year);

            if (!sincronizar_rtc())
            {
                vTaskDelay(pdMS_TO_TICKS(60000));
            }

            continue;
        }

        uint32_t espera_s = reloj_segundos_hasta_proximo_slot_5min(&dt);

        if (espera_s > 0)
        {
            ESP_LOGI(TAG,
                     "Esperando %lu s hasta proximo slot",
                     (unsigned long)espera_s);

            vTaskDelay(pdMS_TO_TICKS(espera_s * 1000));
            continue;
        }
   
        float nivel_m = 0.0f;
        uint32_t timestamp = reloj_datetime_to_epoch(&dt) + OFFSET_PY_S;

        char fecha[32];
        reloj_format(&dt, fecha, sizeof(fecha));

        ESP_LOGI(TAG, "Procesando slot RTC: %s", fecha);

        bool lectura_ok = false;

        for (int intento = 1; intento <= 3; intento++)
        {
            ESP_LOGI(TAG, "Intento lectura sensor %d/3", intento);

            if (sensor_rs485_read_level(&nivel_m))
            {
                led_blink_sensor_ok();
                lectura_ok = true;
                break;
            }

            ESP_LOGW(TAG, "Fallo lectura sensor, reintentando...");
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        if (!lectura_ok)
        {
            ESP_LOGE(TAG, "No se pudo leer nivel luego de 3 intentos");
            nivel_m = -1.0f;
        }

        // Guardar SIEMPRE en datalog.csv
        sdcard_append_level(timestamp, nivel_m);

        // Enviar SIEMPRE el dato actual primero
        if (!envio_level_main(timestamp, nivel_m))
        {
            ESP_LOGW(TAG, "Fallo envio actual. Guardando pendiente");
            sdcard_append_pending(timestamp, nivel_m);
        }

        ESP_LOGI(TAG,
                 "Ciclo terminado: d=%lu,%.3f",
                 (unsigned long)timestamp,
                 nivel_m);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}