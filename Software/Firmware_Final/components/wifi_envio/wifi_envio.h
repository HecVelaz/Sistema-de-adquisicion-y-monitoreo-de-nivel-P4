#ifndef WIFI_ENVIO_H
#define WIFI_ENVIO_H

#include <stdbool.h>
#include <stdint.h>

// Reemplaza al modem DTU: WiFi (STA) + HTTP al mismo servidor.
// SSID, clave y URL van en credenciales.h (no se sube al repo).

#define WIFI_TIMEOUT_CONEXION_MS 15000
#define WIFI_TIMEOUT_HTTP_MS     10000

void wifi_init(void);

// true si hay conexion WiFi en este momento (no espera)
bool wifi_conectado(void);

bool wifi_send_level(
    uint32_t timestamp,
    float level_m
);

// Envia una linea de texto por UDP a la PC (PC_IP:PC_PUERTO en credenciales.h).
// No espera respuesta ni reintenta: la SD guarda todo igual.
// false si no hay WiFi, si PC_IP no esta definida o si fallo el envio.
bool wifi_send_pc(const char *linea);

// Envia la misma linea a GitHub (repository_dispatch, evento "lectura") para
// la pagina de monitoreo. Necesita GITHUB_REPO y GITHUB_TOKEN en credenciales.h;
// si faltan, no hace nada. Bloquea hasta WIFI_TIMEOUT_HTTP_MS. Sin reintento.
bool wifi_send_github(const char *linea);

// Hora UTC por NTP (necesita WiFi). false si no se pudo sincronizar.
bool wifi_hora_utc(uint32_t *utc);

#endif
