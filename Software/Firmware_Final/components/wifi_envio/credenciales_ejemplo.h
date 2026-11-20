#ifndef CREDENCIALES_H
#define CREDENCIALES_H

// ======================================================
// PLANTILLA. Copiar como credenciales.h (esta en .gitignore)
// y completar. NUNCA subir credenciales.h: el repo es publico.
// ======================================================

// Redes WiFi de 2,4 GHz con WPA2 (sin portal cautivo), en orden de preferencia.
// Al arrancar se usa la primera que aparezca; si falla 3 veces, la siguiente.
#define WIFI_REDES { \
    { "red_preferida", "clave_1" }, \
    { "red_de_respaldo", "clave_2" }, \
}

// URL del script PHP que inserta en la base de datos, SIN la trama.
// El firmware agrega "?d=<epoch>,<valor>" y manda lo mismo en el cuerpo.
// Ejemplo: "http://servidor/carpeta/script.php"
#define SERVIDOR_URL "http://0.0.0.0/ruta/script.php"

// PC que recibe la lectura completa por UDP (Software/receptor_pc/receptor_udp.py).
// "255.255.255.255" = broadcast: llega a cualquier PC de la misma red, sin
// importar su IP. Comentar PC_IP para no enviar a la PC.
#define PC_IP     "255.255.255.255"
#define PC_PUERTO 5005

#endif
