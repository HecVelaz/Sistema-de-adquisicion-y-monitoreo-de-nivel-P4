#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include "esp_log.h"
#include "nvs.h"
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
// ID CORRELATIVO DE MEDICION (persistente en NVS)
// Sobrevive a reinicios y cortes: un salto en el id = dato perdido.
// La NVS la inicializa wifi_init().
// ======================================================

static uint32_t id_siguiente(void)
{
    static uint32_t id_ram = 0;   // respaldo si la NVS falla
    uint32_t id = 0;
    nvs_handle_t h;

    if (nvs_open("medicion", NVS_READWRITE, &h) != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS no disponible: id solo en RAM");
        return ++id_ram;
    }

    nvs_get_u32(h, "id", &id);   // si no existe queda en 0
    id++;

    if (nvs_set_u32(h, "id", id) != ESP_OK || nvs_commit(h) != ESP_OK)
    {
        ESP_LOGE(TAG, "No se pudo guardar el id %lu en NVS", (unsigned long)id);
    }

    nvs_close(h);
    id_ram = id;
    return id;
}

// ======================================================
// LECTURA COMPLETA (PC POR UDP Y PAGINA EN GITHUB)
// Mismo formato que datos.csv: epoch,id,distancia_m,velocidad_ms,senal_db,estado
// (campo vacio si no hay dato). La velocidad NO va al PHP: solo aqui y en la SD.
// ======================================================

static void formatear_linea(char *linea, size_t tam, uint32_t epoch, uint32_t id,
                            const sensor_lectura_t *l, sensor_estado_t estado)
{
    char dist[16] = "", vel[16] = "", senal[12] = "";

    if (!isnan(l->distancia_m))  snprintf(dist, sizeof(dist), "%.3f", l->distancia_m);
    if (!isnan(l->velocidad_ms)) snprintf(vel, sizeof(vel), "%.3f", l->velocidad_ms);
    if (l->senal_db >= 0)        snprintf(senal, sizeof(senal), "%d", l->senal_db);

    snprintf(linea, tam, "%lu,%lu,%s,%s,%s,%d",
             (unsigned long)epoch, (unsigned long)id, dist, vel, senal, (int)estado);
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
        uint32_t ts;
        float nivel;

        // Solo con WiFi, lejos del proximo slot y si hay algo en la cola
        if (reloj_now(&dt) &&
            reloj_segundos_hasta_proximo_slot(&dt) > MARGEN_BG_SLOT_S &&
            wifi_conectado() &&
            sdcard_read_first_pending(&ts, &nivel))
        {
            if (ts < EPOCH_MINIMO)
            {
                // Dato guardado con el RTC sin hora (ano 2000): no se envia
                ESP_LOGW(TAG,
                         "BG descarta pendiente con fecha invalida: d=%lu,%.3f",
                         (unsigned long)ts,
                         nivel);
                sdcard_remove_first_pending();
            }
            else
            {
                ESP_LOGI(TAG,
                         "BG reenviando pendiente: d=%lu,%.3f",
                         (unsigned long)ts,
                         nivel);

                if (!envio_level_bg(ts, nivel))
                {
                    ESP_LOGW(TAG, "BG fallo reenvio. Cola conservada");
                }
                else if (sdcard_remove_first_pending())
                {
                    ESP_LOGI(TAG, "BG pendiente enviado y eliminado");
                }
                else
                {
                    // Llego al servidor pero sigue en la cola: se reenviara (duplicado)
                    ESP_LOGE(TAG, "BG enviado pero no se pudo sacar de la cola");
                }
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
#ifdef MODO_BANCO
    // Modo banco: lee el radar cada 2 s, lo muestra en el monitor y lo
    // manda a la PC por UDP (epoch 0: no usa RTC). No usa SD ni el PHP.
    // Compilar con: PLATFORMIO_BUILD_FLAGS=-DMODO_BANCO pio run -t upload
    ESP_LOGW(TAG, "MODO BANCO: lectura cada 2 s a la PC, sin SD ni servidor");

    sensor_rs485_init();
    wifi_init();

    uint32_t n = 0;
    sensor_lectura_t lectura;

    while (1)
    {
        n++;

        sensor_estado_t estado = sensor_rs485_leer(&lectura);

        if (estado != SENSOR_OK)
        {
            ESP_LOGW(TAG, "Lectura %lu: sin dato valido (estado %d)", (unsigned long)n, estado);
        }

        char linea[96];
        formatear_linea(linea, sizeof(linea), 0, n, &lectura, estado);
        wifi_send_pc(linea);   // a GitHub no: seria un evento cada 2 s

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
#endif

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

        uint32_t espera_s = reloj_segundos_hasta_proximo_slot(&dt);

        if (espera_s > 0)
        {
            ESP_LOGI(TAG,
                     "Esperando %lu s hasta proximo slot",
                     (unsigned long)espera_s);

            vTaskDelay(pdMS_TO_TICKS(espera_s * 1000));
            continue;
        }
   
        uint32_t timestamp = reloj_datetime_to_epoch(&dt) + OFFSET_PY_S;
        uint32_t id = id_siguiente();

        char fecha[32];
        reloj_format(&dt, fecha, sizeof(fecha));

        ESP_LOGI(TAG, "Procesando slot RTC: %s | id %lu", fecha, (unsigned long)id);

        sensor_lectura_t lectura;
        sensor_estado_t estado = SENSOR_SIN_RESPUESTA;

        for (int intento = 1; intento <= 3; intento++)
        {
            ESP_LOGI(TAG, "Intento lectura sensor %d/3", intento);

            estado = sensor_rs485_leer(&lectura);

            if (estado == SENSOR_OK)
            {
                led_blink_sensor_ok();
                break;
            }

            ESP_LOGW(TAG, "Fallo lectura sensor (estado %d), reintentando...", estado);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // Al servidor va la trama de siempre: d=<epoch>,<distancia>; -1 si fallo
        float nivel_m = lectura.distancia_m;

        if (estado != SENSOR_OK)
        {
            ESP_LOGE(TAG, "No se pudo leer nivel luego de 3 intentos (estado %d)", estado);
            nivel_m = -1.0f;
        }

        // Guardar SIEMPRE la medicion completa en datos.csv
        sdcard_guardar_medicion(timestamp, id, lectura.distancia_m,
                                lectura.velocidad_ms, lectura.senal_db, estado);

        // Lectura completa (con velocidad) a la PC; al PHP solo la distancia
        char linea[96];
        formatear_linea(linea, sizeof(linea), timestamp, id, &lectura, estado);
        wifi_send_pc(linea);

        // Enviar SIEMPRE el dato actual primero
        if (!envio_level_main(timestamp, nivel_m))
        {
            ESP_LOGW(TAG, "Fallo envio actual. Guardando pendiente");
            sdcard_append_pending(timestamp, nivel_m);
        }

        // Despues del PHP (que tiene prioridad): la pagina de monitoreo
        wifi_send_github(linea);

        ESP_LOGI(TAG,
                 "Ciclo terminado: id %lu | d=%lu,%.3f",
                 (unsigned long)id,
                 (unsigned long)timestamp,
                 nivel_m);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}