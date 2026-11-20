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
#include "lwip/sockets.h"
#include "esp_crt_bundle.h"
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
// REDES CONOCIDAS (WIFI_REDES en credenciales.h)
// Al iniciar se usa la primera de la lista que aparezca en el escaneo
// (orden = preferencia); si una falla 3 veces seguidas, la siguiente.
// ======================================================

#ifndef WIFI_REDES
#define WIFI_REDES { { WIFI_SSID, WIFI_CLAVE } }   // formato viejo: una sola red
#endif

typedef struct
{
    const char *ssid;
    const char *clave;
} wifi_red_t;

static const wifi_red_t redes[] = WIFI_REDES;

#define N_REDES                 ((int)(sizeof(redes) / sizeof(redes[0])))
#define FALLOS_PARA_CAMBIAR_RED 3
#define MAX_APS_ESCANEO         32

static int red_actual = 0;
static int fallos_seguidos = 0;

static void wifi_usar_red(int i)
{
    wifi_config_t wifi_config = { 0 };

    strncpy((char *)wifi_config.sta.ssid, redes[i].ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, redes[i].clave, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;   // WPA2 o superior
    wifi_config.sta.pmf_cfg.capable = true;                    // PMF si el router lo pide
    wifi_config.sta.pmf_cfg.required = false;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;           // WPA3 (SAE)

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    red_actual = i;

    ESP_LOGI(TAG, "Conectando a \"%s\" (red %d de %d)...", redes[i].ssid, i + 1, N_REDES);
}

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
        ESP_LOGW(TAG, "WiFi desconectado de \"%s\" (motivo %d). Reintentando...",
                 redes[red_actual].ssid, ev->reason);

        if (N_REDES > 1 && ++fallos_seguidos >= FALLOS_PARA_CAMBIAR_RED)
        {
            fallos_seguidos = 0;
            wifi_usar_red((red_actual + 1) % N_REDES);
        }

        esp_wifi_connect();
    }
    else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        wifi_ap_record_t ap;

        xEventGroupSetBits(wifi_eventos, WIFI_CONECTADO_BIT);
        fallos_seguidos = 0;

        if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK)
        {
            ESP_LOGI(TAG, "Conectado a \"%s\". IP " IPSTR " | RSSI %d dBm",
                     (const char *)ap.ssid, IP2STR(&ev->ip_info.ip), ap.rssi);
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

static int wifi_elegir_red(void)
{
    static wifi_ap_record_t aps[MAX_APS_ESCANEO];
    wifi_scan_config_t scan = { .show_hidden = false };

    if (esp_wifi_scan_start(&scan, true) != ESP_OK)
    {
        ESP_LOGW(TAG, "No se pudo escanear. Se prueba \"%s\"", redes[0].ssid);
        return 0;
    }

    uint16_t n = MAX_APS_ESCANEO;

    if (esp_wifi_scan_get_ap_records(&n, aps) != ESP_OK)
    {
        n = 0;
    }

    int elegida = -1;

    for (int i = 0; i < N_REDES; i++)
    {
        for (int k = 0; k < n; k++)
        {
            if (strcmp((const char *)aps[k].ssid, redes[i].ssid) != 0) continue;

            ESP_LOGI(TAG, "Red \"%s\": seguridad %s (%d) | canal %d | RSSI %d dBm",
                     redes[i].ssid, wifi_nombre_auth(aps[k].authmode), aps[k].authmode,
                     aps[k].primary, aps[k].rssi);

            if (elegida < 0) elegida = i;   // la primera de la lista que aparece
            break;   // los AP vienen ordenados por senal: el primero es el mejor
        }
    }

    esp_wifi_clear_ap_list();

    if (elegida < 0)
    {
        ESP_LOGW(TAG, "Ninguna red conocida aparece en el escaneo. Se prueba \"%s\"",
                 redes[0].ssid);
        return 0;
    }

    return elegida;
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

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_usar_red(wifi_elegir_red());
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
// ENVIO A LA PC POR UDP
// Lectura completa (con velocidad) para la PC del equipo.
// El servidor PHP sigue recibiendo solo d=<epoch>,<distancia>.
// ======================================================

bool wifi_send_pc(const char *linea)
{
#ifndef PC_IP
    (void)linea;
    return false;   // envio a la PC desactivado en credenciales.h
#else
    if (linea == NULL || !wifi_conectado()) return false;

    struct sockaddr_in destino = {
        .sin_family = AF_INET,
        .sin_port   = htons(PC_PUERTO),
    };

    if (inet_pton(AF_INET, PC_IP, &destino.sin_addr) != 1)
    {
        ESP_LOGE(TAG, "PC_IP invalida: %s", PC_IP);
        return false;
    }

    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (s < 0)
    {
        ESP_LOGW(TAG, "No se pudo crear el socket UDP");
        return false;
    }

    // Permite PC_IP = "255.255.255.255": llega a cualquier PC de la red
    int si = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &si, sizeof(si));

    int n = sendto(s, linea, strlen(linea), 0,
                   (struct sockaddr *)&destino, sizeof(destino));
    close(s);

    if (n < 0)
    {
        ESP_LOGW(TAG, "Fallo el envio UDP a la PC");
        return false;
    }

    ESP_LOGI(TAG, "Enviado a la PC (%s:%d): %s", PC_IP, PC_PUERTO, linea);
    return true;
#endif
}

// ======================================================
// ENVIO A GITHUB (PAGINA DE MONITOREO)
// POST https://api.github.com/repos/<GITHUB_REPO>/dispatches
// El Action .github/workflows/lectura.yml agrega la linea a datos.json
// (rama "datos") y la pagina la muestra. Exito = HTTP 204.
// ======================================================

bool wifi_send_github(const char *linea)
{
#if !defined(GITHUB_REPO) || !defined(GITHUB_TOKEN)
    (void)linea;
    return false;   // pagina desactivada en credenciales.h
#else
    if (linea == NULL || !wifi_conectado()) return false;

    char url[160];
    snprintf(url, sizeof(url), "https://api.github.com/repos/%s/dispatches", GITHUB_REPO);

    char cuerpo[192];
    snprintf(cuerpo, sizeof(cuerpo),
             "{\"event_type\":\"lectura\",\"client_payload\":{\"linea\":\"%s\"}}",
             linea);

    esp_http_client_config_t config = {
        .url               = url,
        .method            = HTTP_METHOD_POST,
        .timeout_ms        = WIFI_TIMEOUT_HTTP_MS,
        .crt_bundle_attach = esp_crt_bundle_attach,   // certificados raiz incluidos
        .buffer_size_tx    = 1024,                    // el token va en la cabecera
    };

    esp_http_client_handle_t cliente = esp_http_client_init(&config);

    if (cliente == NULL)
    {
        ESP_LOGW(TAG, "GitHub: no se pudo crear el cliente HTTPS");
        return false;
    }

    esp_http_client_set_header(cliente, "Accept", "application/vnd.github+json");
    esp_http_client_set_header(cliente, "Authorization", "Bearer " GITHUB_TOKEN);
    esp_http_client_set_header(cliente, "X-GitHub-Api-Version", "2022-11-28");
    esp_http_client_set_header(cliente, "User-Agent", "estacion-nivel-esp32");
    esp_http_client_set_header(cliente, "Content-Type", "application/json");
    esp_http_client_set_post_field(cliente, cuerpo, strlen(cuerpo));

    bool ok = false;
    esp_err_t err = esp_http_client_perform(cliente);

    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "GitHub: fallo HTTPS (%s)", esp_err_to_name(err));
    }
    else
    {
        int estado = esp_http_client_get_status_code(cliente);
        ok = (estado == 204);

        if (ok)
        {
            ESP_LOGI(TAG, "GitHub: lectura enviada a la pagina");
        }
        else if (estado == 401 || estado == 403 || estado == 404)
        {
            ESP_LOGE(TAG, "GitHub: HTTP %d (token invalido, vencido o sin permiso Contents)", estado);
        }
        else
        {
            ESP_LOGW(TAG, "GitHub: HTTP %d", estado);
        }
    }

    esp_http_client_cleanup(cliente);
    return ok;
#endif
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
