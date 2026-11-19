# Bitácora

Registro por fecha de lo hecho en cada sesión. El estado actual está en `docs/ESTADO.md`; lo que falta, en `docs/PENDIENTES.md`.

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
