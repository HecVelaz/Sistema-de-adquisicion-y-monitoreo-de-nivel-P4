# CLAUDE.md

Contexto para Claude Code en este repositorio. Idioma de trabajo: **español** (código, comentarios, commits y documentación ya están en español; mantenerlo).

## Proyecto

Estación de adquisición y monitoreo de nivel de agua del **arroyo Mburicaó** (Asunción, Paraguay), instalada en el Colegio San Ignacio de Loyola. Proyecto académico de Ingeniería Mecatrónica, FIUNA (en P3 eran el grupo 10; en P4 no usar ese número): Héctor Velázquez, Mathias Aguilar, Mauricio Tullo.

- **Proyecto 3 (2026-1C)**: sólo software/electrónica. PCB propia + firmware. Terminado.
- **Proyecto 4 (actual, 2026-2C)**: suma **mecánica** (montaje del sensor) e **interfaz** (web propia del equipo). Sensor nuevo: radar Luda/POLIVIR FM02B. Profesores: Prof. Ing. Federico Gaona, MSc. y Prof. Ing. Esteban Fretes. Entrega del 2.º parcial: **16/11/2026**.
- Repos relacionados (no están aquí): `HecVelaz/Proyecto-Mburica-` (repo central, IA, docs), `HecVelaz/Prediccion-de-nivel-de-rios`.

### Arquitectura

```
Mini UPS ─12V─> Sensor de nivel RS485                         ── UART1 GPIO1/2, DE/RE GPIO4
                 (P3: ultrasónico Modbus RTU 9600; P4: radar FM02B 115200)
         ─12V─> Módem DTU GPRS USR-IOT (RS485, 115200)        ── UART2 GPIO5/6, RTS GPIO7 (half-duplex HW)
         ─5V──> PCB ESP32-S3-WROOM-1 (N16R8) ─ AMS1117 3.3V
                   ├─ RTC DS3231M  I2C0 SDA8/SCL9 (SQW GPIO16 sin uso)
                   ├─ microSD SPI  CS10 MOSI11 CLK12 MISO13, DET15 (activo bajo)
                   └─ LED azul GPIO21 (lectura válida)
```

Trama al servidor: `d=<epoch>,<valor %.3f>\r\n`; éxito si la respuesta contiene `OK`. El servidor/BD y Grafana no están en este repo.

## Reglas

- El estado va en `docs/ESTADO.md`, nunca en `CLAUDE.md`. Este archivo solo cambia si cambia una regla.
- Al cerrar una sesión: `/cerrar`; al empezar: `/retomar`.
- Las decisiones de alcance de `docs/DECISIONES.md` están acordadas con el usuario: no proponer revertirlas.
- **Datos privados**: `documentos/Reportes/P4/` también contiene, solo en local y en `.gitignore`, la planilla `Evaluacion_Proyecto_4_Mecatronica_FIUNA.xlsx` (notas de los 16 equipos), `tarea.jpeg` (teléfono del profesor) y la captura del comentario del profesor. **Nunca subirlos**: el repo es público.
- No subir `.pio/`, binarios ni archivos grandes nuevos (los videos van a Google Drive).
- Nombres de carpeta sin espacios al final (Windows no puede clonarlos).

### Git

- `origin`: `https://github.com/HecVelaz/Sistema-de-adquisicion-y-monitoreo-de-nivel-P4` (rama `main`). **Aquí van todos los cambios de P4.**
- `original`: el repo de Proyecto 3 (`…-monitoreo-de-nivel`). Sólo lectura; el push está deshabilitado a propósito. No modificarlo.
- Commits en español, cortos. Hacer commit/push sólo cuando el usuario lo pida.
- Identidad local del repo: `hvelazquez@fiuna.edu.py` (el `user.email` global de la máquina es un placeholder).

## Qué leer para cada cosa

| Para… | Leer |
| --- | --- |
| Estado actual, notas, entregables hechos | `docs/ESTADO.md` |
| Por qué se decidió algo, alcance, objetivos | `docs/DECISIONES.md` |
| Qué falta hacer | `docs/PENDIENTES.md` |
| Firmware: compilar, flujo, particularidades, convenciones C | `Software/CLAUDE.md` |
| Informes LaTeX de P4: compilar, estilo | `documentos/Reportes/P4/CLAUDE.md` |
| Descripción general y pinout | `README.md` |
| Reglas del revisor Codex | `AGENTS.md` |

## Estructura

- `Software/Firmware_Final/` — firmware de producción; `Software/Firmware_Pruebas/` — pruebas por módulo.
- `Hardware/V3_Estacion_Nivel/` — KiCad 9 (2 capas), Gerbers en `fabrication_files_G10/`, BOM `Estacion_Nivel_V3.csv`, STEP, planos PDF. `Librerias Kicad/` son zips de terceros.
- `documentos/Diseño Mecánico/` — SolidWorks (`Caja/`, `sensor/`) + PDFs de planos.
- `documentos/Reportes/` — informes de P3 (`1f_*`, `2p_*`, `P3_final.pptx`). `documentos/Reportes/P4/` — todo lo de P4.
- `documentos/Evidencias_Pruebas/` — capturas; videos de las pruebas de P3 enlazados a Google Drive.
- `docs/` — estado, decisiones y pendientes.
