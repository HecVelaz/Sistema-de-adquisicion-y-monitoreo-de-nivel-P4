# CLAUDE.md

Contexto para Claude Code en este repositorio. Idioma de trabajo: **español** (código, comentarios, commits y documentación ya están en español; mantenerlo).

## Qué es el proyecto

Estación de adquisición y monitoreo de nivel de agua del **arroyo Mburicaó** (Asunción, Paraguay), instalada en el Colegio San Ignacio de Loyola. Proyecto académico de Ingeniería Mecatrónica, FIUNA (en P3 eran el grupo 10; en P4 no usar ese número): Héctor Velázquez, Mathias Aguilar, Mauricio Tullo.

- **Proyecto 3 (2026-1C)**: sólo software/electrónica. PCB propia + firmware. Terminado (informes `documentos/Reportes/1f_*`, `2p_*`, `P3_final.pptx`).
- **Proyecto 4 (actual, 2026-2C)**: suma **mecánica** e **interfaz** (Grafana → web en GitHub Pages "MburicaoCastAI", repo aparte). Profesores: Prof. Ing. Federico Gaona, MSc. y Prof. Ing. Esteban Fretes. Ver la sección "Estado de Proyecto 4".
- Repos relacionados (no están aquí): `HecVelaz/Proyecto-Mburica-` (repo central, IA, docs), `HecVelaz/Prediccion-de-nivel-de-rios`.

## Estado de Proyecto 4

### Evaluación de la primera entrega (20/09/2026)

- Nota oficial **46/50** (informe 21/25, diseño mecánico 25/25). En la planilla del evaluador: **71/100** en la rúbrica (= Σ peso × puntaje/5); 85,7 % es la valoración *relativa* al mejor equipo (82,8), no un puntaje propio.
- Criterios más bajos: **Validación y seguridad 2,5/5** y **Objetivos y alcance 3/5**. Mejora prioritaria según la planilla: "cerrar la metrología del nivel y demostrar recuperación de datos ante cortes".
- Comentario del Prof. Fretes: el resumen no debe empezar con "El presente informe…"; reducir los 15 objetivos específicos a 5–6, con al menos uno de electrónica, mecánica e interfaz (bien planteados: 1, 8, 10, 11, 13).
- Tarea del Prof. Gaona: de la "evaluación detallada", rescatar puntos a mejorar y los que quedan fuera de alcance, justificando.

### Nuevo sensor: radar Luda / POLIVIR FM02B (part number LDFM04_B)

- Nivel por radar 80 GHz: 0,1–40 m, ±3 mm, resolución 1 mm, haz 14°. Velocidad superficial por radar 24 GHz: 0,05–20 m/s, ±0,01 m/s.
- 9–24 V (típ. 12 V), 130–160 mA; **RS485 a 115200 baudios** (el ultrasónico de P3 era 9600). Peso 0,8 kg; incluye soporte orientable, placa para poste, 3 abrazaderas inoxidables y 5 m de cable. USD 690,41 + 64,80 envío.
- La ficha comercial tiene inconsistencias (rango de velocidad 0,05 vs 0,1 m/s; corriente 87–140 vs 130–160 mA) y **no trae el mapa de registros RS485**: hace falta el manual antes de reescribir `sensor_rs485`.
- Fotos y capturas de la ficha: `documentos/Reportes/P4/sensor_nuevo/`.
- **H** (Nivel = H − d) depende del lugar de instalación y el equipo ya sabe cómo obtenerlo; no incluirlo como contenido de informes.

### Decisiones de alcance (acordadas con el usuario)

- Foco de P4: **validación** (resultados medibles) y **objetivos**. La mecánica se reduce al **montaje del sensor** con su kit comercial; no hay gabinete/soporte nuevo que diseñar ni cálculo por viento.
- **Fuera de alcance:** montaje en el arroyo (depende del Depto. de Mecánica y Energía y del grupo de investigación Mburicaó → se demuestra en un **banco de laboratorio** que simula la instalación), medir caudal, calibrar en todo el rango 0,1–40 m, usuarios/permisos en la web, validar la predicción con IA (es del proyecto de investigación), panel solar. No mencionar certificación IP65.
- Pruebas de P3 (videos de sensor, SD, RTC, módem, sistema integrado, Store and Forward; ~9 meses de datos en Grafana) se usan como línea de base; lo que falta es cuantificar: error en mm, perdidos/duplicados en ≥ 10 cortes, tiempo de recuperación, % de disponibilidad, consumo y autonomía.
- Objetivos reformulados (5): Electrónica (integración + consumo/autonomía), Firmware (Store and Forward con ID, 0 perdidos/duplicados en ≥ 10 cortes), Montaje del sensor (banco de laboratorio, antena nivelada), Interfaz (históricos, antigüedad del dato, estado del sensor, alarma), Validación (error ≤ ±10 mm vs cinta métrica en banco). Los umbrales son propuestas del equipo.

### Informe de puntos de mejora

- Entregable: `documentos/Reportes/P4/Informe_puntos_de_mejora_P4.pdf`. **La fuente es LaTeX**: `documentos/Reportes/P4/latex_puntos_mejora/main.tex` (+ `diagramas/diagrama_mejoras.tex` en TikZ, `figuras/`). El documento de claude.ai que se usó como borrador ya no es la fuente.
- Compilar (TeX Live instalado): `cd documentos/Reportes/P4/latex_puntos_mejora && pdflatex main.tex && pdflatex main.tex`, luego copiar `main.pdf` a `../Informe_puntos_de_mejora_P4.pdf` y regenerar `../latex_puntos_mejora_overleaf.zip`. Revisar las páginas renderizadas (`pdftoppm`) antes de entregar.
- Detalles LaTeX: `babel` en español rompe las flechas de TikZ → se usa `\usetikzlibrary{babel}`; figuras/tablas con `[H]` (paquete `float`) y `\needspace` antes de las secciones para que el título no quede separado de su tabla.
- Estilo pedido por el usuario: corto, visual (imágenes/diagramas), lenguaje técnico pero simple, fuente LaTeX (Latin Modern), portada con logo FIUNA (`Hardware/V3_Estacion_Nivel/imagenes/Logo-fiuna.png`). **No escribir "Grupo 10"** (era de P3).

### Datos privados

`documentos/Reportes/P4/` también contiene, solo en local y en `.gitignore`, la planilla `Evaluacion_Proyecto_4_Mecatronica_FIUNA.xlsx` (notas de los 16 equipos), `tarea.jpeg` (teléfono del profesor) y la captura del comentario del profesor. **Nunca subirlos**: el repo es público.

## Arquitectura

```
Mini UPS ─12V─> Sensor de nivel RS485                         ── UART1 GPIO1/2, DE/RE GPIO4
                 (P3: ultrasónico Modbus RTU 9600; P4: radar FM02B 115200, ver abajo)
         ─12V─> Módem DTU GPRS USR-IOT (RS485, 115200)        ── UART2 GPIO5/6, RTS GPIO7 (half-duplex HW)
         ─5V──> PCB ESP32-S3-WROOM-1 (N16R8) ─ AMS1117 3.3V
                   ├─ RTC DS3231M  I2C0 SDA8/SCL9 (SQW GPIO16 sin uso)
                   ├─ microSD SPI  CS10 MOSI11 CLK12 MISO13, DET15 (activo bajo)
                   └─ LED azul GPIO21 (lectura válida)
```

Trama al servidor: `d=<epoch>,<valor %.3f>\r\n`; éxito si la respuesta contiene `OK`. El servidor/BD y Grafana no están en este repo.

## Estructura

- `Software/Firmware_Final/` — **firmware de producción** (PlatformIO + ESP-IDF, env `freenove_esp32_s3_wroom`). `src/main.c` + componentes en `components/{reloj,sdcard,sensor_rs485,modem_dtu,led}`.
- `Software/Firmware_Pruebas/` — pruebas aisladas por módulo (se comenta/descomenta en `main/main.c`). API distinta a la final; no reutilizar sin revisar.
- `Hardware/V3_Estacion_Nivel/` — KiCad 9 (2 capas), Gerbers en `fabrication_files_G10/`, BOM `Estacion_Nivel_V3.csv`, STEP, planos PDF. `Librerias Kicad/` son zips de terceros.
- `documentos/Diseño Mecánico/` — SolidWorks (`Caja/`, `sensor/`) + PDFs de planos.
- `documentos/Reportes/` — informes de P3 (`1f_*`, `2p_*`, `P3_final.pptx`). `documentos/Reportes/P4/` — todo lo de P4 (ver abajo).
- `documentos/Evidencias_Pruebas/` — capturas; videos de las pruebas de P3 enlazados a Google Drive.

## Flujo del firmware final

1. `app_main`: init RTC → SD → sensor (hace una lectura de descarte) → DTU (espera 3 s) → LED; crea `dtu_mutex` y `task_pendientes` (prio 4).
2. Loop: lee RTC, espera al próximo slot (**cada 2 min**, ver abajo), lee sensor hasta 3 intentos (si falla, valor `-1.0`), `timestamp = epoch(RTC) + 3*3600` (RTC en hora local PY UTC-3 → se convierte a UTC), guarda siempre en `/sdcard/datalog.csv`, envía; si falla, agrega a `/sdcard/temp.csv`.
3. `task_pendientes`: cada 5 s, si faltan >35 s al próximo slot, toma el primer pendiente FIFO, lo envía (mutex con 1 s de espera) y si hay `OK` reescribe `temp.csv` sin esa línea.

## Particularidades que hay que conocer

- `reloj_segundos_hasta_proximo_slot_5min()` usa `intervalo = 2 * 60`: el nombre dice 5 min pero mide cada 2 min.
- El sensor entrega **distancia** `d` (registro 0x0000, mm → m). El informe define `Nivel = H − d`, pero el firmware **envía y guarda `d` crudo**; H no está implementado.
- `CONFIG_FREERTOS_HZ=100` (tick 10 ms): los `pdMS_TO_TICKS` pequeños se redondean (p. ej. `uart_wait_tx_done(..., 15 ms)` = 1 tick).
- El acceso a la SD **no** está protegido por mutex (sólo el DTU); main y la tarea de pendientes tocan `temp.csv`.
- `dtu_send_level` usa `uart_read_bytes` con buffer de 1023 bytes y timeout de 12 s: cada envío espera ~12 s completos.
- FATFS sin LFN (`CONFIG_FATFS_LFN_NONE`): nombres de archivo en formato 8.3.
- I2C usa el driver legacy `driver/i2c.h` (deprecado en IDF 5.x).
- `sdkconfig` del final declara flash de 2 MB aunque el módulo es N16R8; `Firmware_Pruebas/sdkconfig.defaults` habla de N8R8.
- El README raíz describe una estructura de `components/` ligeramente distinta a la real (nombres `led_blink`, `dtu`, `rtc`, `sensor` en Pruebas).
- `Hardware/.../DRC.rpt` es de una versión vieja (`Estacion_lluviaV2`); `ERC.rpt` tiene 4 errores de tipo `power_pin_not_driven` (faltan PWR_FLAG).

## Comandos

PlatformIO está en `~/.platformio/penv/bin/pio` (no está en el PATH).

```bash
cd Software/Firmware_Final        # o Software/Firmware_Pruebas
~/.platformio/penv/bin/pio run                       # compilar
~/.platformio/penv/bin/pio run -t upload             # cargar
~/.platformio/penv/bin/pio device monitor -b 115200  # monitor serie (USB Serial/JTAG)
```

- La plataforma está fijada en `espressif32@6.13.0` (**ESP-IDF 5.5.3**, la versión validada en P3). No quitar el pin: sin él PlatformIO baja ESP-IDF 6.x, donde el componente `driver` ya no exporta `driver/gpio.h` y el build falla.
- Ambos proyectos compilan en Linux (verificado 2026-10-06). Primera compilación ~15 min (descarga del framework); después ~2 min.
- No hay tests automatizados ni CI; "compila" no significa "probado en la placa".

## Convenciones

- C estilo ESP-IDF, `ESP_LOGx` con `TAG` por módulo, banners `// ====` entre secciones, llaves en línea propia. Funciones en español con prefijo de módulo (`reloj_*`, `sdcard_*`, `dtu_*`, `sensor_rs485_*`, `led_*`).
- Cada componente: `.c`, `.h` y `CMakeLists.txt` con `idf_component_register(... REQUIRES ...)`.
- Pines definidos como `#define` en el `.h`/`.c` de cada componente; si se cambia un pin, actualizar README y el cuadro de pines del informe.
- No subir `.pio/`, binarios ni archivos grandes nuevos (los videos van a Google Drive).

## Git

- `origin`: `https://github.com/HecVelaz/Sistema-de-adquisicion-y-monitoreo-de-nivel-P4` (rama `main`). **Aquí van todos los cambios de P4.**
- `original`: el repo de Proyecto 3 (`…-monitoreo-de-nivel`). Sólo lectura; el push está deshabilitado a propósito. No modificarlo.
- Commits en español, cortos. Hacer commit/push sólo cuando el usuario lo pida.
- Identidad local del repo: `hvelazquez@fiuna.edu.py` (el `user.email` global de la máquina es un placeholder).
- Nombres de carpeta sin espacios al final (Windows no puede clonarlos).
