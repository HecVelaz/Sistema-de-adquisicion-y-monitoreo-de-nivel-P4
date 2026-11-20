#ifndef SENSOR_RS485_H
#define SENSOR_RS485_H

#include <stdbool.h>
#include <stdint.h>

// Estado de una lectura (se guarda en la SD; mismo codigo que la trama nueva)
typedef enum
{
    SENSOR_OK                 = 0,
    SENSOR_SIN_RESPUESTA      = 1,   // el radar no contesta (cable, alimentacion)
    SENSOR_RESPUESTA_INVALIDA = 2,   // CRC malo, excepcion Modbus o trama rara
    SENSOR_FUERA_DE_RANGO     = 3    // distancia < 0,1 m
} sensor_estado_t;

typedef struct
{
    float    distancia_m;    // NAN si no hay dato
    float    velocidad_ms;   // NAN si no se pudo leer (no invalida la lectura)
    int      senal_db;       // -1 si no se pudo leer (no invalida la lectura)
} sensor_lectura_t;

bool sensor_rs485_init(void);

// Lee distancia (0x0001), senal (0x0004) y velocidad filtrada (0x01F4-F5).
// El estado depende solo de la distancia.
sensor_estado_t sensor_rs485_leer(sensor_lectura_t *lectura);

#endif
