#ifndef CREDENCIALES_H
#define CREDENCIALES_H

// ======================================================
// PLANTILLA. Copiar como credenciales.h (esta en .gitignore)
// y completar. NUNCA subir credenciales.h: el repo es publico.
// ======================================================

// Red WiFi de 2,4 GHz con WPA2 (sin portal cautivo)
#define WIFI_SSID   "nombre_de_la_red"
#define WIFI_CLAVE  "clave_de_la_red"

// URL del script PHP que inserta en la base de datos, SIN la trama.
// El firmware agrega "?d=<epoch>,<valor>" y manda lo mismo en el cuerpo.
// Ejemplo: "http://servidor/carpeta/script.php"
#define SERVIDOR_URL "http://0.0.0.0/ruta/script.php"

#endif
