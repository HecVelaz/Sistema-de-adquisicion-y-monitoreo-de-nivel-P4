#ifndef DTU_H
#define DTU_H

#include <stdbool.h>
#include <stdint.h>

// Modem USR-GPRS232-730 en modo HTTPD (POST al PHP, timeout HTTP del modem 10 s).
// Con el filtro de cabeceras (AT+HTPPK "on") al serial llega solo el cuerpo: "OK".
#define DTU_RESPONSE_TIMEOUT_MS 15000   // timeout HTTP del modem + margen
#define DTU_SILENCIO_MS         500     // fin de respuesta: 500 ms sin bytes

void dtu_init(void);

bool dtu_send_level(
    uint32_t timestamp,
    float level_m
);

#endif