# CLAUDE.md — Software (firmware)

Reglas para `Software/Firmware_Final/` y `Software/Firmware_Pruebas/`. Las reglas generales están en el `CLAUDE.md` raíz.

## Proyectos

- `Firmware_Final/` — **firmware de producción** (PlatformIO + ESP-IDF, env `freenove_esp32_s3_wroom`). `src/main.c` + componentes en `components/{reloj,sdcard,sensor_rs485,wifi_envio,led}`. `modem_dtu` sigue en el árbol pero no se usa (P4 envía por WiFi).
- `Firmware_Pruebas/` — pruebas aisladas por módulo (se comenta/descomenta en `main/main.c`). API distinta a la final; no reutilizar sin revisar.

## Comandos

PlatformIO está en `~/.platformio/penv/bin/pio` (no está en el PATH).

```bash
cd Software/Firmware_Final        # o Software/Firmware_Pruebas
~/.platformio/penv/bin/pio run                       # compilar
~/.platformio/penv/bin/pio run -t upload             # cargar
~/.platformio/penv/bin/pio device monitor -b 115200  # monitor serie (USB Serial/JTAG)
```

- La plataforma está fijada en `espressif32@6.13.0` (**ESP-IDF 5.5.3**). **No quitar el pin** (motivo en `docs/DECISIONES.md`).
- Primera compilación ~15 min (descarga del framework); después ~2 min.
- No hay tests automatizados ni CI; "compila" no significa "probado en la placa".

## Flujo del firmware final

1. `app_main`: init RTC → SD → sensor (hace una lectura de descarte) → WiFi (espera hasta 15 s, no bloquea si no hay red) → LED; crea `envio_mutex` y `task_pendientes` (prio 4, stack 8192).
2. Loop: lee RTC, espera al próximo slot (**cada 5 min**), toma un **id correlativo** de la NVS (`id_siguiente`, sobrevive a reinicios), lee sensor hasta 3 intentos (`sensor_rs485_leer`: distancia, señal, velocidad y estado), `timestamp = epoch(RTC) + 3*3600` (RTC en hora local PY UTC-3 → se convierte a UTC), guarda siempre la línea completa en `/sdcard/datos.csv`, envía `d=<epoch>,<distancia>` (`-1` si falló); si el envío falla, agrega a `/sdcard/temp.csv`.
3. `task_pendientes`: cada 5 s, **si hay WiFi** y faltan >35 s al próximo slot, toma el primer pendiente FIFO, lo envía (mutex con 1 s de espera) y si hay `OK` reescribe `temp.csv` sin esa línea.

## Particularidades que hay que conocer

- Intervalo: `INTERVALO_MEDICION_MIN` en `components/reloj/reloj.h` (5 por defecto, 288 lecturas/día); `reloj_segundos_hasta_proximo_slot()`. Para pruebas: `PLATFORMIO_BUILD_FLAGS=-DINTERVALO_MEDICION_MIN=1 pio run -t upload`. En P3 era `2 * 60` para pruebas; no mencionarlo en los informes.
- **Modo banco**: `PLATFORMIO_BUILD_FLAGS=-DMODO_BANCO pio run -t upload` lee el radar cada 2 s y solo lo muestra en el monitor (sin RTC, SD ni WiFi). Para volver al normal, cargar sin flags.
- `datos.csv`: `epoch,id,distancia_m,velocidad_ms,senal_db,estado` (campo vacío = sin dato). Estado: 0 OK, 1 sin respuesta, 2 respuesta inválida (CRC/excepción), 3 fuera de rango. `datalog.csv` (formato P3) ya no se escribe. El id está en la NVS (`medicion`/`id`): un `erase_flash` lo reinicia.
- El radar FM02B entrega **distancia** `d` (registro 0x0001, mm → m; < 100 mm → estado 3). También se leen la señal (0x0004) y la velocidad filtrada (0x01F4–F5, int32 BE ×0,001 m/s); las dos son informativas (no invalidan la lectura) y van solo a `datos.csv`: el PHP del servidor acepta exactamente 2 campos. El informe define `Nivel = H − d`, pero el firmware **envía y guarda `d` crudo**; H no está implementado.
- `CONFIG_FREERTOS_HZ=100` (tick 10 ms): los `pdMS_TO_TICKS` pequeños se redondean (p. ej. `uart_wait_tx_done(..., 15 ms)` = 1 tick).
- Toda función pública de `sdcard` toma un mutex interno (main y la tarea de pendientes tocan `temp.csv`). Cada escritura hace `fflush` + `fsync`. Quitar un pendiente escribe `temp_aux.csv` completo, borra `temp.csv` y renombra; `recuperar_cola()` (al montar y antes de cada operación de cola) arregla un corte en el medio.
- **Servidor** (`insertarMasi.php`, sin acceso para editarlo): rechaza tramas que no tengan exactamente 2 campos (`ERROR_INVALID_FORMAT`) e inserta con `INSERT` simple. No cambiar la trama ni poner índices `UNIQUE` en `Masi`: la placa reintentaría el dato para siempre. La columna `Masi.estado` es una columna generada en la BD (OK / FALLA_SENSOR / RECUPERADO).
- **Hora:** al arrancar (y siempre que el RTC tenga año < 2025) se sincroniza por NTP (`pool.ntp.org`, `wifi_hora_utc`) y se escribe en el DS3231 en hora PY (`reloj_set`, borra el flag OSF). Sin hora válida **no se mide** (`reloj_hora_valida`). La tarea de pendientes **descarta** los de `temp.csv` con epoch < 2025-01-01 en vez de enviarlos. Motivo: el 19/11/2026 el RTC estaba en el año 2000 (¿pila BT1?) y se enviaron `-1` con esas fechas a la base.
- `wifi_send_level` hace un POST a `SERVIDOR_URL?d=<epoch>,<valor>` con el mismo `d=...` en el cuerpo (no se conoce el PHP: así sirve con `$_GET`, `$_POST` o `$_REQUEST`); éxito = HTTP 200 y `OK` en la respuesta. Timeout 10 s. La URL (script PHP de inserción) va solo en `credenciales.h`: no escribirla en el repo.
- `components/wifi_envio/credenciales.h` (SSID, clave, URL) está en `.gitignore`; la plantilla es `credenciales_ejemplo.h`. **Nunca subir el real.**
- Partición `partitions_singleapp_large.csv` (app 1,5 MB) puesta en `platformio.ini`: PlatformIO ignora la del `sdkconfig`. Con WiFi la app no entra en 1 MB.
- Main task stack 8192 (antes 3584) por el cliente HTTP.
- FATFS sin LFN (`CONFIG_FATFS_LFN_NONE`): nombres de archivo en formato 8.3.
- I2C usa el driver legacy `driver/i2c.h` (deprecado en IDF 5.x).
- `sdkconfig` del final declara flash de 2 MB aunque el módulo es N16R8; `Firmware_Pruebas/sdkconfig.defaults` habla de N8R8.
- El README raíz describe una estructura de `components/` ligeramente distinta a la real (nombres `led_blink`, `dtu`, `rtc`, `sensor` en Pruebas).
- Radar de P4: RS485 a 115200, dirección 1 (**confirmados por Bluetooth el 19/11/2026**; la tabla del manual dice 9600 de fábrica, pero no aplica a nuestra unidad). DE/RE (GPIO4) manejado por hardware (`UART_MODE_RS485_HALF_DUPLEX`), CRC verificado. Manual y mapa de registros en `documentos/Reportes/P4_2doParcial/`.

## Convenciones

- C estilo ESP-IDF, `ESP_LOGx` con `TAG` por módulo, banners `// ====` entre secciones, llaves en línea propia. Funciones en español con prefijo de módulo (`reloj_*`, `sdcard_*`, `dtu_*`, `sensor_rs485_*`, `led_*`).
- Cada componente: `.c`, `.h` y `CMakeLists.txt` con `idf_component_register(... REQUIRES ...)`.
- Pines definidos como `#define` en el `.h`/`.c` de cada componente; si se cambia un pin, actualizar README y el cuadro de pines del informe.
- No subir `.pio/` ni binarios.
