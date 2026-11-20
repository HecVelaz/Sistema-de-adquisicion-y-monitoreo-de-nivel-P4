# Estado del proyecto — 2026-11-20

Foto del estado actual. Se reescribe al cerrar cada sesión (`/cerrar`); la historia está en `docs/BITACORA.md`.

## Contexto

- P3 terminado (línea de base). P4 1.er parcial: 46/50; lo más flojo fue validación y objetivos.
- Informes del 1.er parcial y su fuente LaTeX: `documentos/Reportes/P4_1erParcial/` (incluye `Respuesta_evaluacion_1P_P4.pdf`, con lo prometido para el 2.º parcial). Material del 2.º parcial: `documentos/Reportes/P4_2doParcial/`.
- La fecha de entrega registrada (16/11/2026) ya pasó: falta confirmar la nueva.

## Radar FM02B

- **Conectado a J3 y funcionando con el firmware** (115200, dirección 1). `0x0001` = distancia en mm, confirmado con cinta a una altura (0,197 m contra ~19,7 cm).
- Señal 15–18 dB sobre superficies (el manual pide > 30); **todavía no se midió sobre agua**.
- Velocidad (`0x01F4`) se lee: 0,000 m/s en el banco.
- **Consumo: 126 mA a 12 V (~1,5 W)**. No tiene modo de reposo.
- Configuración (Bluetooth): calibración −46 mm, potencia de nivel 7 (mínima), sensibilidad 5. Sin cambios hasta medir con agua.
- No hay datasheet oficial publicado; los tres PDFs de `P4_2doParcial/` son la referencia.

## Placa V3

- Conectores: **J2** jack 12 V (solo alimenta al radar) · **J9** USB-C o **J4** jack 5 V (ESP32) · **J3** radar (1 +12 V, 2 A, 3 B, 4 GND) · **J8** sin uso.
- El esquemático PDF de `Hardware/V3_Estacion_Nivel/planos/` y el BOM `.csv` están **desactualizados**; el `.kicad_pcb` y los `.kicad_sch` son los correctos.
- Pila BT1 del DS3231 sin verificar (el RTC estuvo en el año 2000; hoy lo corrige el NTP al arrancar).

## Firmware (`Software/Firmware_Final`)

- **Probado en la placa con el radar**, a 5 min, enviando por WiFi (`LabEquiposMedicos`, RSSI −43 dBm; FIUNA también funciona).
- Cada medición: id correlativo (NVS), distancia, velocidad, señal y estado → `datos.csv` en la SD.
- Al servidor va la trama de siempre `d=<epoch>,<distancia>` (`-1` si el radar falla): el script de inserción acepta solo 2 campos y no se puede editar.
- SD con mutex, `fsync` y recuperación de la cola tras un corte (compila y funciona; **el corte durante la escritura falta ensayarlo**).
- La tarea de pendientes no reintenta sin WiFi (falta verlo en un corte).
- Modo banco (`-DMODO_BANCO`, cada 2 s) e intervalo configurable (`-DINTERVALO_MEDICION_MIN`). Detalle: `Software/CLAUDE.md`.
- **La placa quedó midiendo sola** (otra fuente, sin notebook) desde el 20/11 ~00:30.

## Ensayos

| Ensayo | Resultado | Falta |
| --- | --- | --- |
| Cortes de red | 1 de ≥ 10: 7/7 recuperados, 0 perdidos, 0 duplicados, 1 min 36 s | 9 más, de distintos tipos |
| Cortes de energía | — | ≥ 10, incluso durante la escritura en la SD |
| Exactitud | 1 altura en seco | 4 alturas × 10 lecturas sobre agua |
| Consumo | Radar 126 mA | ESP32 (medidor USB) |
| Disponibilidad 7 días | — | Empezar después de los cortes |
| Autonomía | — | Falta la UPS |

## Base de datos

- Columna generada **`estado`** (OK / FALLA_SENSOR / RECUPERADO) y vista **`Masi_estado_estacion`** (EN LINEA / CAIDA, > 15 min sin datos). Sin permiso para disparadores.
- Hay datos de banco en la tabla de la estación desde el 19/11 19:55 (incluidos los `-1` con fecha del año 2000). No se borraron: antes hay que saber si la predicción lee esa tabla.
- Respaldo local de la tabla del 19/11 (fuera del repo).

## Documentación

- Guía de armado: `P4_2doParcial/Guia_conexion_radar_FM02B.md` y `.pdf`. Le faltan los datos nuevos de los manuales (sin reposo, `0xEA60`, `0x07D8`, `0x07D7`, `0x07D9`).
- `README.md` raíz: describe el módem DTU y una estructura de `components/` que ya no coincide.
- En la respuesta a la evaluación se escribió "tenemos acceso al servidor para adaptarlo": no es así (solo a la BD). Explicarlo en el 2.º parcial.
