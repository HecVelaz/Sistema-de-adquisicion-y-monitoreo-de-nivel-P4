# Pendientes

Tareas y próximos pasos. Tachar o mover a `docs/ESTADO.md` cuando se terminen.

## Movido desde CLAUDE.md (2026-10-22)

### Fecha límite

- **16/11/2026**: entrega del 2.º parcial de P4.

### Antes de que llegue el radar (llega entre la 1.ª y la 2.ª semana de noviembre de 2026)

- Ensayar con el sensor ultrasónico de P3: ≥ 10 cortes (perdidos/duplicados, tiempo de recuperación), disponibilidad en 7 días (≥ 95 %), consumo y autonomía.
- Conseguir el manual del radar con el mapa de registros RS485 (la ficha comercial no lo trae): hace falta antes de reescribir `sensor_rs485`.

### Con el radar

- Driver RS485 del radar en `sensor_rs485` (115200 baudios, CRC, mapa de registros).
- Implementar la trama nueva `d=<epoch>,<id>,<distancia>,<velocidad>,<estado>` (ID, UTC, estado en vez de `-1`).
- Montaje en banco de laboratorio (antena nivelada, haz de 14° libre) y exactitud: error ≤ ±10 mm vs cinta métrica.

### Interfaz

- Web propia del equipo: históricos, antigüedad del dato, estado del sensor, alarma; probar con la BD histórica de P3 y luego con los datos del radar.

### Prolijidad (detectado, sin resolver)

- `README.md` raíz: estructura de `components/` distinta a la real.
- `Hardware/V3_Estacion_Nivel/ERC.rpt`: 4 errores `power_pin_not_driven` (faltan PWR_FLAG); `DRC.rpt` es de una versión vieja.
- `sdkconfig` del firmware final declara flash de 2 MB aunque el módulo es N16R8; `Firmware_Pruebas/sdkconfig.defaults` habla de N8R8.
