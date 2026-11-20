#include "wifi_envio.h"

#include <stdio.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"
#include <time.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#if __has_include("credenciales.h")
#include "credenciales.h"
#else
#error "Falta components/wifi_envio/credenciales.h: copiar credenciales_ejemplo.h y completarlo"
#endif

// ======================================================
// ESTADO DE LA CONEXION
// ======================================================

#define WIFI_CONECTADO_BIT BIT0
#define RESPUESTA_MAX      256

static const char *TAG = "WIFI";
static EventGroupHandle_t wifi_eventos;

// ======================================================
// EVENTOS WIFI / IP
// Reconecta siempre: si se cae la red, los datos quedan en
// temp.csv y la tarea de pendientes los reenvia al volver.
// ======================================================

static void wifi_evento(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START)
    {
        // La primera conexion la hace wifi_init() despues del escaneo
    }
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_event_sta_disconnected_t *ev = (wifi_event_sta_disconnected_t *)data;

        xEventGroupClearBits(wifi_eventos, WIFI_CONECTADO_BIT);
        ESP_LOGW(TAG, "WiFi desconectado (motivo %d). Reintentando...", ev->reason);
        esp_wifi_connect();
    }
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        wifi_ap_record_t ap;

        xEventGroupSetBits(wifi_eventos, WIFI_CONECTADO_BIT);

        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK)
        {
            ESP_LOGI(TAG, "Conectado. IP " IPSTR " | RSSI %d dBm",
                     IP2STR(&ev->ip_info.ip), ap.rssi);
        }
        else
        {
            ESP_LOGI(TAG, "Conectado. IP " IPSTR, IP2STR(&ev->ip_info.ip));
        }
    }
}

// ======================================================
// DIAGNOSTICO: SEGURIDAD DE LA RED CONFIGURADA
// ======================================================

static const char *wifi_nombre_auth(wifi_auth_mode_t m)
{
    switch (m)
    {
        case WIFI_AUTH_OPEN:            return "ABIERTA";
        case WIFI_AUTH_WEP:             return "WEP";
        case WIFI_AUTH_WPA_PSK:         return "WPA";
        case WIFI_AUTH_WPA2_PSK:        return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
        case WIFI_AUTH_WPA3_PSK:        return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/WPA3";
        default:                        return "otro";
    }
}

static void wifi_escanear_red(void)
{
    wifi_scan_config_t scan = {
        .ssid = (uint8_t *)WIFI_SSID,
        .show_hidden = false,
    };

    if (esp_wifi_scan_start(&scan, true) != ESP_OK)
    {
        ESP_LOGW(TAG, "No se pudo escanear");
        return;
    }

    uint16_t n = 1;
    wifi_ap_record_t ap;

    if (esp_wifi_scan_get_ap_records(&n, &ap) != ESP_OK || n == 0)
    {
        ESP_LOGW(TAG, "La red \"%s\" no aparece en el escaneo", WIFI_SSID);
        return;
    }

    ESP_LOGI(TAG, "Red \"%s\": seguridad %s (%d) | canal %d | RSSI %d dBm",
             WIFI_SSID, wifi_nombre_auth(ap.authmode), ap.authmode,
             ap.primary, ap.rssi);

    esp_wifi_clear_ap_list();
}

// ======================================================
// INIT
// ======================================================

void wifi_init(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);

    wifi_eventos = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_evento, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_evento, NULL));

    wifi_config_t wifi_config = { 0 };

    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, WIFI_CLAVE, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;   // WPA2 o superior
    wifi_config.sta.pmf_cfg.capable = true;                    // PMF si el router lo pide
    wifi_config.sta.pmf_cfg.required = false;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;           // WPA3 (SAE)

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_escanear_red();

    ESP_LOGI(TAG, "Conectando a \"%s\"...", WIFI_SSID);
    esp_wifi_connect();

    // No bloquea para siempre: si no hay red, el sistema mide igual
    EventBits_t bits = xEventGroupWaitBits(
        wifi_eventos,
        WIFI_CONECTADO_BIT,
        pdFALSE,
        pdTRUE,
        pdMS_TO_TICKS(WIFI_TIMEOUT_CONEXION_MS)
    );

    if (!(bits & WIFI_CONECTADO_BIT))
    {
        ESP_LOGW(TAG, "Sin WiFi al iniciar. Se sigue midiendo y guardando en SD");
    }
}

// ======================================================
// ESTADO DE LA CONEXION
// ======================================================

bool wifi_conectado(void)
{
    if (wifi_eventos == NULL) return false;

    return (xEventGroupGetBits(wifi_eventos) & WIFI_CONECTADO_BIT) != 0;
}

// ======================================================
// ENVIO AL SERVIDOR
// Trama "d=<epoch>,<valor %.3f>"; exito si HTTP 200 y "OK".
// ======================================================

bool wifi_send_level(uint32_t timestamp, float level_m)
{
    EventBits_t bits = xEventGroupWaitBits(
        wifi_eventos,
        WIFI_CONECTADO_BIT,
        pdFALSE,
        pdTRUE,
        pdMS_TO_TICKS(5000)
    );

    if (!(bits & WIFI_CONECTADO_BIT))
    {
        ESP_LOGW(TAG, "Sin WiFi. No se envia");
        return false;
    }

    char payload[64];

    snprintf(
        payload,
        sizeof(payload),
        "d=%lu,%.3f",
        (unsigned long)timestamp,
        level_m
    );

    ESP_LOGI(TAG, "Enviando: %s", payload);

    // No se conoce el codigo del PHP: la trama va en la URL (?d=...) y
    // tambien en el cuerpo del POST. Asi sirve si el PHP lee $_GET,
    // $_POST o $_REQUEST; es una sola peticion, inserta una sola vez.
    char url[256];

    snprintf(
        url,
        sizeof(url),
        "%s%c%s",
        SERVIDOR_URL,
        (strchr(SERVIDOR_URL, '?') != NULL) ? '&' : '?',
        payload
    );

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = WIFI_TIMEOUT_HTTP_MS,
    };

    esp_http_client_handle_t cliente = esp_http_client_init(&config);

    if (cliente == NULL)
    {
        ESP_LOGE(TAG, "No se pudo crear el cliente HTTP");
        return false;
    }

    esp_http_client_set_header(cliente, "Content-Type", "application/x-www-form-urlencoded");

    int largo_envio = strlen(payload);
    bool ok = false;
    esp_err_t err = esp_http_client_open(cliente, largo_envio);

    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "No se pudo conectar al servidor: %s", esp_err_to_name(err));
        esp_http_client_cleanup(cliente);
        return false;
    }

    if (esp_http_client_write(cliente, payload, largo_envio) != largo_envio)
    {
        ESP_LOGW(TAG, "Fallo al escribir el cuerpo");
        esp_http_client_close(cliente);
        esp_http_client_cleanup(cliente);
        return false;
    }

    esp_http_client_fetch_headers(cliente);

    int estado = esp_http_client_get_status_code(cliente);

    char respuesta[RESPUESTA_MAX];
    int leidos = esp_http_client_read_response(cliente, respuesta, sizeof(respuesta) - 1);

    if (leidos < 0)
    {
        leidos = 0;
    }

    respuesta[leidos] = '\0';

    ESP_LOGI(TAG, "HTTP %d | respuesta: %s", estado, respuesta);

    if (estado == 200 && strstr(respuesta, "OK") != NULL)
    {
        ok = true;
    }
    else
    {
        ESP_LOGW(TAG, "El servidor no confirmo el dato");
    }

    esp_http_client_close(cliente);
    esp_http_client_cleanup(cliente);

    return ok;
}

// ======================================================
// HORA POR NTP
// El DS3231 puede perder la hora (pila BT1): con WiFi se corrige.
// ======================================================

#define NTP_SERVIDOR     "pool.ntp.org"
#define NTP_TIMEOUT_MS   15000
#define EPOCH_MINIMO     1735689600UL   // 2025-01-01: antes de esto no es hora real

bool wifi_hora_utc(uint32_t *utc)
{
    static bool sntp_iniciado = false;

    if (utc == NULL) return false;

    EventBits_t bits = xEventGroupGetBits(wifi_eventos);

    if (!(bits & WIFI_CONECTADO_BIT))
    {
        ESP_LOGW(TAG, "Sin WiFi: no se puede sincronizar la hora");
        return false;
    }

    if (!sntp_iniciado)
    {
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(NTP_SERVIDOR);

        if (esp_netif_sntp_init(&config) != ESP_OK)
        {
            ESP_LOGW(TAG, "No se pudo iniciar SNTP");
            return false;
        }

        sntp_iniciado = true;
    }
    else
    {
        esp_netif_sntp_start();
    }

    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(NTP_TIMEOUT_MS)) != ESP_OK)
    {
        ESP_LOGW(TAG, "NTP no respondio (puede estar bloqueado en la red)");
        return false;
    }

    time_t ahora = time(NULL);

    if ((uint32_t)ahora < EPOCH_MINIMO)
    {
        ESP_LOGW(TAG, "NTP devolvio una hora invalida");
        return false;
    }

    *utc = (uint32_t)ahora;

    ESP_LOGI(TAG, "Hora NTP (UTC): %lu", (unsigned long)*utc);

    return true;
}
