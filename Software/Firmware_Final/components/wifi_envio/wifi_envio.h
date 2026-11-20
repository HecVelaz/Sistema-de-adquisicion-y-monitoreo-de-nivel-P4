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

// Hora UTC por NTP (necesita WiFi). false si no se pudo sincronizar.
bool wifi_hora_utc(uint32_t *utc);

#endif
