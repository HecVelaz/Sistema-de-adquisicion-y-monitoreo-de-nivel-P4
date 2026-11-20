# Bitácora

Registro por fecha de lo hecho en cada sesión. El estado actual está en `docs/ESTADO.md`; lo que falta, en `docs/PENDIENTES.md`.

## 2026-11-19/20 — Radar en la placa, corte de red, estado en la BD y firmware con id

### Hecho

- **Radar conectado a J3 y probado con el firmware**: respondió a la primera (115200, dirección 1, sin invertir A/B, sin errores de CRC). Registro `0x0001` confirmado como distancia: **0,197 m del radar contra ~19,7 cm con cinta** (una sola altura, sobre una superficie; falta el agua). Señal 15–18 dB apuntando a superficies (< 30). Velocidad (`0x01F4`) = 0,000 m/s.
- **Consumo del radar: 126 mA a 12 V (~1,5 W)** medido en la fuente (ficha: 130/140/160 mA). La fuente de 12 V alimenta solo al radar; el ESP32 + WiFi toma de los 5 V del USB (para medirlo hace falta un medidor USB).
- **Manuales releídos completos** (texto + páginas que son solo imagen). Datos nuevos: **no hay modo de reposo** (consume siempre ~130 mA); `0xEA60` devuelve `0x1A91` (= inicio del n.º de serie, identifica la unidad); calibración `0x07D8` en pasos de 1 mm; potencia de nivel `0x07D7` (0 máx., 7 mín.) y sensibilidad `0x07D9` escribibles por Modbus; filtro de 128 muestras (tarda en dar el primer dato). No hay registro de temperatura. El manual dice que los registros "están en decimal", pero las tramas de ejemplo son hexadecimales y son las que funcionaron.
- **Datasheet oficial**: no hay uno publicado. El LDFM02 se vende con varias marcas (Shanghai JIANT en Alibaba, app "JET Radar Debugger"; ZLY TECH como ZLYLDFM02) y las páginas de revendedores no coinciden entre sí. Pedir al vendedor la versión exacta (serie y firmware V25 de la app).
- **WiFi**: FIUNA RSSI −50 dBm; `LabEquiposMedicos` funciona con la clave corregida (RSSI −43 dBm).
- **Corte de red n.º 1** (router apagado, intervalo de 1 min): WiFi caído 22:15:29 → 22:22:31; **7 pendientes (22:16–22:22), 7 reenviados en orden FIFO, 0 perdidos, 0 duplicados** (confirmado en la BD); cola vacía 1 min 36 s después de volver el WiFi. El dato de las 22:09 falta porque la placa se estaba cargando: cada reinicio o recarga cuesta un intervalo (no es pérdida del Store and Forward).
- **Servidor**: el equipo solo tiene acceso a la BD, no a los scripts. El script de inserción acepta **exactamente 2 campos** en `d=` y hace un `INSERT` simple: la trama nueva no se puede usar sin que el administrador suba otro script (ver `docs/DECISIONES.md`).
- **BD**: respaldo local de la tabla antes de tocarla (fuera del repo). Sin permiso para crear disparadores, se agregó una **columna generada `estado`** (`FALLA_SENSOR` si nivel < 0, `RECUPERADO` si llegó > 120 s tarde, si no `OK`) y una **vista `Masi_estado_estacion`** (`EN LINEA` / `CAIDA` si pasan > 15 min sin datos). Funcionan.
- **Firmware** (`Software/Firmware_Final`):
  - `sensor_rs485_leer()`: distancia + señal + **velocidad** y **estado** (0 OK, 1 sin respuesta, 2 respuesta inválida, 3 fuera de rango); lectura genérica de 1 o 2 registros.
  - `sdcard`: **mutex** en todas las funciones, `fflush` + `fsync`, quitar pendiente con `temp_aux.csv` + `recuperar_cola()` (al montar y antes de cada operación de cola). Archivo nuevo **`datos.csv`**: `epoch,id,distancia_m,velocidad_ms,senal_db,estado`. `datalog.csv` ya no se escribe.
  - **id correlativo en NVS** (`medicion`/`id`), comprobado: siguió en 2 tras un reinicio.
  - `task_pendientes` no reintenta sin WiFi (`wifi_conectado()`); se sacó el log "BG no envia" cada 5 s.
  - Intervalo configurable `INTERVALO_MEDICION_MIN` (5 por defecto) y **modo banco** `-DMODO_BANCO` (lee cada 2 s, sin guardar ni enviar).
  - La trama al servidor no cambia (`d=<epoch>,<distancia>`, `-1` si falla). Probado en placa a 5 min: `Guardado SD: 1795144800,2,0.864,0.000,18,0` + `HTTP 200 OK`.
- **Placa dejada midiendo** a 5 min con otra fuente (sin notebook) desde el 20/11 ~00:30.
- `.gitignore`: carpeta `P4_2doParcial/Fotos/captura_servidor/` (las capturas del servidor muestran datos que no van al repo público).

### Problemas y lecciones

- Leer el puerto USB-JTAG con pyserial cambiando DTR/RTS deja al ESP32-S3 en **modo descarga** (`waiting for download`). Solución: `esptool.py --chip esp32s3 -p /dev/ttyACM0 --after hard_reset chip_id` y abrir el puerto con los valores por defecto, sin tocar DTR/RTS. Si el puerto queda abierto durante el reinicio, la placa vuelve como `/dev/ttyACM1`.
- `pkill -f` / `pgrep -f` con un patrón que aparece en el mismo comando mata la propia shell: buscar el PID con `ps -eo pid,args | grep "[p]ython.*captura.py"`.
- Datos de banco en la tabla de la estación (no son del arroyo): 19/11 desde las 19:55 (cada 1 min de 21:41 a 00:14) y desde el 20/11 00:15 a 5 min.

### Comandos útiles

```bash
cd Software/Firmware_Final
PLATFORMIO_BUILD_FLAGS=-DMODO_BANCO ~/.platformio/penv/bin/pio run -t upload               # radar cada 2 s
PLATFORMIO_BUILD_FLAGS=-DINTERVALO_MEDICION_MIN=1 ~/.platformio/penv/bin/pio run -t upload  # 1 min
~/.platformio/penv/bin/pio run -t upload                                                    # producción (5 min)
```

```sql
SELECT * FROM Masi_estado_estacion;  -- EN LINEA / CAIDA
-- duplicados
SELECT timestamp, COUNT(*) v FROM Masi WHERE timestamp >= '...' GROUP BY timestamp HAVING v > 1;
-- huecos (intervalo 5 min)
SELECT COUNT(*) recibidos, FLOOR(TIMESTAMPDIFF(MINUTE, MIN(timestamp), MAX(timestamp))/5)+1 esperados
  FROM Masi WHERE timestamp >= '...';
```

### Commits

- Commit de cierre de esta sesión: firmware con id, velocidad, estado y SD segura; modo banco; intervalo configurable.

## 2026-11-19 — Radar FM02B, WiFi y guía de conexión

### Hecho

- **Manuales del radar** (`documentos/Reportes/P4_2doParcial/`): ficha LDFM02_B, protocolo Modbus V3 (mapa de registros) e instrucciones de montaje. Hilos: rojo Vcc 9–24 V, negro GND, verde 485A, amarillo 485B (pines de aviación 1, 2, 3, 4). Distancia en el registro `0x0001` (mm), señal en `0x0004` (> 30 dB buena), velocidad filtrada en `0x01F4` (int32 × 0,001 m/s).
- **Configuración leída por Bluetooth** (app WeChat "JET Radar Debugger", capturas en `P4_2doParcial/Fotos/Aplicacion/`): dirección 1, **115200** (la tabla del manual dice 9600 de fábrica: no aplica), rango 40 m, altura de instalación 500 cm, **calibración −46 mm**, sensibilidad de nivel 5, **potencia de nivel 7 (mínima)**, velocidad bidireccional con algoritmo espectral apagado y límite 5 m/s, versión V25, serie `1a91000400000019`. Lectura de prueba: distancia 0,849 m, **señal 4** (baja), ángulo 56,1°.
- **Placa real ≠ esquemático PDF**: según el `.kicad_pcb` y el `vcc_to_3v3.kicad_sch`, J2 = jack 12 V, J9 USB-C o J4 jack = 5 V, **J3 = radar** (1 +12 V, 2 A1, 3 B1, 4 GND; bornera derecha), J8 = canal del módem. El PDF de `planos/` y el BOM `.csv` muestran J4/J5 de 2 polos que no existen.
- **Guía de conexión** `P4_2doParcial/Guia_conexion_radar_FM02B.md` + `.pdf` (8 págs.) con el diagrama `diagrama_conexion_radar.jpg` sobre la foto de la placa (generado con `diagrama_conexion_radar.py`).
- **Firmware** (`Software/Firmware_Final`):
  - `sensor_rs485`: radar a 115200, registro `0x0001`, CRC verificado, excepción Modbus, descarta < 100 mm, DE/RE por hardware (`UART_MODE_RS485_HALF_DUPLEX`), muestra la señal en el log.
  - `wifi_envio` (nuevo): WiFi STA con reconexión, POST a `SERVIDOR_URL?d=...` con `d=...` también en el cuerpo (no se conoce el PHP), éxito = HTTP 200 + `OK`. Escaneo al arrancar que muestra la seguridad de la red. WPA3/PMF habilitados.
  - `reloj`: `reloj_set`, `reloj_hora_valida`, aviso de OSF. `main`: sincroniza el DS3231 por NTP al arrancar; sin hora válida no mide; los pendientes con fecha < 2025 se descartan.
  - Partición `partitions_singleapp_large.csv` en `platformio.ini` (con WiFi la app ocupa 1,03 MB: no entraba en 1 MB). Stack de main y de la tarea de pendientes en 8192.
  - `modem_dtu` queda en el árbol sin usar.
- **Prueba en placa (sin radar)**: arranque correcto (RTC, SD de 32 GB, RS485 sin respuesta como es esperable). WiFi `LabEquiposMedicos` rechazó la clave (motivo 15); con **`FIUNA` (WPA2, canal 1) conectó, RSSI −63 dBm, servidor `HTTP 200 OK`**. El usuario confirmó un dato llegado a las 19:55 con hora correcta.
- **Problema detectado**: el DS3231 estaba en el **año 2000** (¿pila BT1?) y se enviaron a la base `-1.000` con epoch ~946 696 200. Llevó al cambio de NTP + bloqueo de fechas inválidas.
- **Repo**: `documentos/Reportes/P4/` se renombró a `P4_1erParcial/`. El `.gitignore` apuntaba a la ruta vieja y dejaba expuestos la planilla de notas, `tarea.jpeg` y la captura del profesor: se corrigió antes del push.

### Comandos útiles

```bash
# Firmware
cd Software/Firmware_Final
~/.platformio/penv/bin/pio run -t upload && ~/.platformio/penv/bin/pio device monitor -b 115200
# Si el monitor muestra "waiting for download": apretar RST (en Linux, abrir el puerto USB-JTAG
# activa DTR y el reinicio por software entra en modo descarga).

# PDF de la guía (pdflatex; LuaLaTeX no tiene la caché de fuentes). Desde P4_2doParcial/:
# copiar el .md, cambiar la imagen por un bloque {=latex} con \includegraphics y usar un
# encabezado con \usepackage{newunicodechar,graphicx} y \newunicodechar para → ← ↔ Ω − ≥ ≤ ≈
pandoc guia.md -o Guia_conexion_radar_FM02B.pdf --resource-path=. --pdf-engine=pdflatex \
  --shift-heading-level-by=-1 -f markdown+pipe_tables+task_lists+raw_attribute -H cabecera.tex \
  -V lang=es -V geometry:margin=2cm -V fontsize=10pt -V fontfamily=lmodern
```

### Commits

- `2ac8db8` Renombrar Reportes/P4 a P4_1erParcial y actualizar rutas y .gitignore
- `973005b` Firmware para el radar FM02B y envío por WiFi; guía de conexión (push a `origin/main`)

## 2026-10-22 y anteriores — movido desde `ESTADO.md`

- **Proyecto 3 (2026-1C)** terminado: PCB propia + firmware. Informes `documentos/Reportes/1f_*`, `2p_*`, `P3_final.pptx`. Pruebas de P3 (sensor, SD, RTC, módem, sistema integrado, Store and Forward; ~9 meses de datos en Grafana) = línea de base de P4.
- **Evaluación 1.ª entrega de P4 (20/09/2026)**: nota oficial 46/50 (informe 21/25, mecánica 25/25); en la planilla 71/100 (85,7 % es relativo al mejor equipo, 82,8). Criterios más bajos: Validación y seguridad 2,5/5, Objetivos y alcance 3/5. Mejora prioritaria: "cerrar la metrología del nivel y demostrar recuperación de datos ante cortes". Prof. Fretes: el resumen no empieza con "El presente informe…"; objetivos de 15 a 5–6 (al menos electrónica, mecánica e interfaz). Prof. Gaona: rescatar puntos a mejorar y fuera de alcance → `Respuesta_evaluacion_1P_P4.pdf`.
- **Ficha comercial del radar** (antes de tener los manuales): nivel 80 GHz 0,1–40 m ±3 mm, haz 14°; velocidad 24 GHz 0,05–20 m/s; 9–24 V, 130–160 mA; 0,8 kg; kit con soporte, placa, 3 abrazaderas y 5 m de cable; USD 690,41 + 64,80 envío. Tenía inconsistencias (velocidad mínima, corriente) y no traía el mapa de registros. Fotos en `P4_1erParcial/sensor_nuevo/`.
- **Informes del 1.er parcial**: `Respuesta_evaluacion_1P_P4.pdf` (10 debilidades marcadas REALIZADO / 2.º PARCIAL / FUERA DE ALCANCE) e `Informe_puntos_de_mejora_P4.pdf`, con fuentes LaTeX en `P4_1erParcial/latex_*`.
- **Firmware** compilaba en Linux (2026-10-06) con `espressif32@6.13.0`; intervalo 5 min (en P3 `2 * 60`).
- 2026-10-22: `CLAUDE.md` adelgazado (estado, decisiones y pendientes a `docs/`); `AGENTS.md` como symlink a `CLAUDE.md`.
