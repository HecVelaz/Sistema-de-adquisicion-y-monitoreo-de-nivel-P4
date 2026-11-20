#ifndef SDCARD_H
#define SDCARD_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "driver/gpio.h"

// ======================================================
// PINES SD SPI
// ======================================================

#define SD_PIN_CS    ((gpio_num_t)10)
#define SD_PIN_MOSI  ((gpio_num_t)11)
#define SD_PIN_CLK   ((gpio_num_t)12)
#define SD_PIN_MISO  ((gpio_num_t)13)

// Detect
#define SD_PIN_DET   ((gpio_num_t)15)

#define MOUNT_POINT "/sdcard"

// ======================================================
// ARCHIVOS (FATFS sin LFN: nombres 8.3)
// ======================================================

// Registro completo de cada medicion (desde P4):
//   epoch,id,distancia_m,velocidad_ms,senal_db,estado
// Campo vacio = sin dato. datalog.csv (formato de P3) ya no se escribe.
#define DATOS_FILE      "/sdcard/datos.csv"

// Cola de pendientes (lo que no llego al servidor): epoch,valor
#define TEMP_QUEUE_FILE "/sdcard/temp.csv"
#define TEMP_AUX_FILE   "/sdcard/temp_aux.csv"

// ======================================================
// FUNCIONES
// Todas toman un mutex interno: se pueden llamar desde varias tareas.
// ======================================================

bool detectar_tarjeta(void);
bool sdcard_init(void);

// velocidad_ms / distancia_m = NAN y senal_db < 0 se guardan como campo vacio
bool sdcard_guardar_medicion(
    uint32_t epoch,
    uint32_t id,
    float distancia_m,
    float velocidad_ms,
    int senal_db,
    int estado
);

bool sdcard_append_pending(uint32_t timestamp, float level_m);
bool sdcard_read_first_pending(uint32_t *timestamp, float *level_m);
bool sdcard_remove_first_pending(void);

#endif
