# Sistema de adquisición y monitoreo de nivel de agua

## Estación de medición de nivel — Arroyo Mburicaó

Estación de adquisición y monitoreo del nivel de agua del arroyo Mburicaó (Asunción, Paraguay), instalada en el Colegio San Ignacio de Loyola. Proyecto académico de Ingeniería Mecatrónica, FIUNA.

- **Proyecto 3 (2026-1C):** PCB propia con ESP32-S3, sensor ultrasónico RS485, microSD, RTC y envío por módem GPRS. Terminado; unos 9 meses de datos.
- **Proyecto 4 (2026-2C, actual):** radar **Luda/POLIVIR FM02B** (nivel y velocidad superficial), montaje del sensor en un banco de laboratorio, envío por **módem GPRS + WiFi** y una **página web propia** de monitoreo.

**Página de monitoreo:** https://hecvelaz.github.io/Sistema-de-adquisicion-y-monitoreo-de-nivel-P4/

---

## Datos obtenidos (Proyecto 3)

Valores registrados por el prototipo de P3 durante unos 9 meses seguidos en la cuenca del arroyo Mburicaó.

![Gráfico de Grafana con los datos de P3](documentos/Evidencias_Pruebas/capturas/grafico_completo.png)

## Imágenes del hardware

| Render 3D / PCB | PCB diseñada | Placa montada |
| --- | --- | --- |
| ![Render 3D](Hardware/V3_Estacion_Nivel/imagenes/placa_frontal.jpg) | ![PCB diseñada](Hardware/V3_Estacion_Nivel/imagenes/diseño.jpg) | ![Placa montada](Hardware/V3_Estacion_Nivel/imagenes/Partes.png) |

---

## Características principales

- **ESP32-S3** (WROOM-1 N16R8) en PCB propia (V3, KiCad 9), firmware en C con **ESP-IDF 5.5.3** sobre PlatformIO.
- **Radar FM02B** por **RS485 / Modbus RTU** (115200, dirección 1): distancia al agua (80 GHz), intensidad de señal y velocidad superficial (24 GHz).
- **Módem DTU GPRS USR-GPRS232-730** (RS485, modo HTTP client) para enviar cada medición al servidor del grupo de investigación.
- **WiFi** del ESP32-S3: respaldo del módem, envío de la lectura completa a la PC del equipo (UDP) y a la página de monitoreo (GitHub Pages). Lista de redes en orden de preferencia.
- **microSD:** registro completo de cada medición (`datos.csv`) y cola de pendientes (`temp.csv`).
- **RTC DS3231M** sincronizado por NTP al arrancar; sin hora válida no se mide.
- **Id correlativo** por medición guardado en la NVS (sobrevive a reinicios): un salto en el id es un dato perdido.
- **Store and Forward:** lo que no llega al servidor queda en la SD y se reenvía solo, en orden.
- Medición cada **5 min**; **modo banco** con una lectura cada 2 s para las pruebas.
- Alimentación con **mini UPS**: 12 V para el radar y el módem, 5 V para el ESP32-S3, 3,3 V internos.
- LED azul en **GPIO21**: lectura válida.

---

## Arquitectura

```text
                         Mini UPS
           ┌───────────────┼──────────────────┐
          12 V            12 V               5 V
           │               │                  │
           ▼               ▼                  ▼
     Radar FM02B      Módem DTU GPRS       ESP32-S3 (PCB V3)
     (J3, RS485)      (J8, RS485)            ├── RTC DS3231M   ── I2C  (GPIO8/9)
           │               │                 ├── microSD       ── SPI  (GPIO10–13, DET 15)
           │  UART1        │  UART2          ├── LED           ── GPIO21
           └──────────────►│◄────────────────┤
                           │                 └── WiFi
                           ▼                       │
                  Servidor PHP + BD          ┌─────┴──────────────┐
                  (d=<epoch>,<distancia>)    ▼                    ▼
                                         PC del equipo      GitHub Pages
                                         (UDP, línea        (HTTPS, línea
                                          completa)          completa)
```

### A dónde va cada medición

| Destino | Por dónde | Qué va | Si falla |
| --- | --- | --- | --- |
| microSD `datos.csv` | — | `epoch,id,distancia_m,velocidad_ms,senal_db,estado` | — |
| Servidor PHP | **Módem GPRS** | `d=<epoch>,<distancia>` (`-1` si el radar falló) | reintento por **WiFi**; si también falla, `temp.csv` |
| PC del equipo | WiFi, UDP broadcast :5005 | la línea de `datos.csv` | queda en la SD |
| Página de monitoreo | WiFi, HTTPS (`repository_dispatch`) | la línea de `datos.csv` | queda en la SD y en la PC |

El script del servidor acepta solo los dos campos de `d=`: la velocidad y la señal van a la SD, a la PC y a la página.

---

## Alimentación

| Módulo | Tensión | Notas |
| --- | ---: | --- |
| Radar FM02B | 12 V | 126 mA medidos (~1,5 W); no tiene modo de reposo |
| Módem DTU GPRS | 12 V | Comunicación con el servidor |
| ESP32-S3 | 5 V | Por USB-C (J9) o jack (J4), nunca los dos |
| RTC, microSD, LED | 3,3 V | AMS1117 en la placa |

## Conectores de la placa V3

| Conector | Qué es | Uso |
| --- | --- | --- |
| **J2** | Jack 12 V | Alimenta J3 y J8 (no al ESP32) |
| **J9** / **J4** | USB-C / jack 5 V | ESP32-S3 (usar uno solo) |
| **J3** | Bornera RS485 #1: 1 +12 V, 2 A, 3 B, 4 GND | Radar (rojo, verde, amarillo, negro) |
| **J8** | Bornera RS485 #2: 1 +12 V, 2 A, 3 B, 4 GND | Módem DTU (A, B, GND) |

Guía completa de armado y cableado: [`documentos/Reportes/P4_2doParcial/Guia_conexion_radar_FM02B.md`](documentos/Reportes/P4_2doParcial/Guia_conexion_radar_FM02B.md) ([PDF](documentos/Reportes/P4_2doParcial/Guia_conexion_radar_FM02B.pdf)).

## Pinout principal

| Módulo | Señal | GPIO ESP32-S3 | Interfaz | Función |
| --- | --- | ---: | --- | --- |
| Radar RS485 (J3) | TX / DI | GPIO1 | UART1 TX | Comando Modbus |
| Radar RS485 (J3) | RX / RO | GPIO2 | UART1 RX | Respuesta |
| Radar RS485 (J3) | DE/RE | GPIO4 | RTS por hardware | Dirección RS485 |
| Módem DTU (J8) | TX / DI | GPIO5 | UART2 TX | Trama al módem |
| Módem DTU (J8) | RX / RO | GPIO6 | UART2 RX | Respuesta del servidor |
| Módem DTU (J8) | DE/RE | GPIO7 | RTS por hardware | Dirección RS485 |
| RTC DS3231M | SDA / SCL | GPIO8 / GPIO9 | I2C0 | Fecha y hora |
| RTC DS3231M | SQW/INT | GPIO16 | Entrada | Sin uso |
| microSD | CS / MOSI / CLK / MISO | GPIO10 / 11 / 12 / 13 | SPI | Tarjeta |
| microSD | DET | GPIO15 | Entrada (activo bajo) | Detección de tarjeta |
| LED azul | — | GPIO21 | Salida | Lectura válida |

---

## Software

### Firmware (`Software/Firmware_Final`)

| Módulo | Descripción |
| --- | --- |
| `src/main.c` | Ciclo principal: hora, id, lectura, SD, envíos; tarea de pendientes; modo banco |
| `components/sensor_rs485` | Radar FM02B por Modbus RTU: distancia, señal, velocidad y estado de la lectura |
| `components/modem_dtu` | Envío de `d=<epoch>,<distancia>` por el módem en modo HTTP client |
| `components/wifi_envio` | WiFi (lista de redes), HTTP al PHP (respaldo), UDP a la PC, HTTPS a GitHub, hora por NTP |
| `components/sdcard` | `datos.csv` y la cola `temp.csv` (mutex, `fsync`, recuperación tras un corte) |
| `components/reloj` | RTC DS3231M e intervalo de medición |
| `components/led` | LED de lectura válida |

Las claves WiFi, la URL del servidor y el token de GitHub van en `components/wifi_envio/credenciales.h`, que **no se sube** al repo; la plantilla es `credenciales_ejemplo.h`. Detalle técnico del firmware: [`Software/CLAUDE.md`](Software/CLAUDE.md).

### Otros

| Carpeta | Qué hay |
| --- | --- |
| `Software/Firmware_Pruebas` | Pruebas aisladas por módulo (de P3) |
| `Software/receptor_pc` | `receptor_udp.py`: recibe las lecturas en la PC y las guarda en `lecturas_pc.csv` |
| `Software/Moden` | Manuales y programa de configuración del módem USR-GPRS232-730 |
| `web/` | Página de monitoreo (GitHub Pages) |
| `.github/workflows` | `lectura.yml` (agrega cada lectura a `datos.json` en la rama `datos`) y `pagina.yml` (publica `web/`) |

---

## Ciclo de operación

```text
Inicio: RTC → SD → radar → módem → WiFi → hora por NTP → tarea de pendientes
  ↓
Esperar el próximo intervalo (cada 5 min, alineado al reloj)
  ↓
Tomar el id siguiente (NVS)
  ↓
Leer el radar (hasta 3 intentos) ──► si falla: distancia = -1 y estado ≠ 0
  ↓
Guardar la línea completa en datos.csv
  ↓
Enviar la línea completa a la PC (UDP)
  ↓
Enviar d=<epoch>,<distancia> al PHP: módem ──► sin OK: WiFi ──► sin OK: temp.csv
  ↓
Enviar la línea completa a la página (GitHub)
  ↓
Volver a esperar
```

En paralelo, la **tarea de pendientes** toma el primer dato de `temp.csv` (lejos del próximo intervalo), lo reenvía por módem o WiFi y, si recibe `OK`, lo saca de la cola. Los pendientes con fecha anterior a 2025 se descartan (RTC sin hora).

---

## Formato de datos

| Archivo / trama | Formato | Ejemplo |
| --- | --- | --- |
| `datos.csv` (SD), línea a la PC y a la página | `epoch,id,distancia_m,velocidad_ms,senal_db,estado` | `1795144800,2,0.864,0.000,18,0` |
| `temp.csv` (SD, pendientes) | `epoch,nivel` | `1795144800,0.864` |
| Trama al servidor | `d=<epoch>,<distancia>` | `d=1795144800,0.864` |
| Respuesta del servidor | `OK` | |

- `epoch` en UTC; campo vacío = sin dato.
- Estado: 0 OK, 1 sin respuesta del radar, 2 respuesta inválida (CRC o excepción Modbus), 3 fuera de rango (< 100 mm).
- La distancia es la del radar a la superficie del agua; el nivel se obtiene como `H − d` (H depende de la instalación).

---

## Estructura del repositorio

```text
├── docs/                      Estado, bitácora, decisiones y pendientes del proyecto
├── documentos/
│   ├── Diseño Mecánico/       SolidWorks (caja y soporte del sensor) y planos
│   ├── Evidencias_Pruebas/    Capturas (los videos de P3 están en Google Drive)
│   └── Reportes/              Informes de P3, P4_1erParcial/ y P4_2doParcial/ (manuales del radar, guía de conexión)
├── Hardware/V3_Estacion_Nivel/ KiCad 9, Gerbers, BOM, STEP, planos e imágenes
├── Software/
│   ├── Firmware_Final/        Firmware de producción
│   ├── Firmware_Pruebas/      Pruebas por módulo
│   ├── receptor_pc/           Receptor UDP para la PC
│   └── Moden/                 Manuales y software del módem
├── web/                       Página de monitoreo
└── .github/workflows/         Publicación de la página y recepción de lecturas
```

---

## Compilación y carga

PlatformIO está en `~/.platformio/penv/bin/pio` (plataforma fijada en `espressif32@6.13.0`, ESP-IDF 5.5.3).

```bash
cd Software/Firmware_Final
pio run                                                  # compilar
pio run -t upload                                        # cargar (producción, cada 5 min)
PLATFORMIO_BUILD_FLAGS=-DMODO_BANCO pio run -t upload    # modo banco: radar cada 2 s, solo a la PC
pio device monitor -b 115200                             # monitor serie
```

Si después de cargar el monitor muestra `waiting for download`, apretar el botón **RESET** de la placa.

Receptor en la PC (opcional; la estación no depende de él):

```bash
sudo ufw allow 5005/udp                       # Linux, una sola vez
python3 Software/receptor_pc/receptor_udp.py  # desde la raíz del repo
```

---

## Pruebas

### Proyecto 3 (línea de base)

| Prueba | Descripción | Estado |
| --- | --- | --- |
| Sensor RS485 | Lectura de nivel por Modbus RTU | Correcto |
| RTC DS3231M | Fecha y hora por I2C | Correcto |
| microSD | Detección, montaje, escritura y lectura | Correcto |
| Módem DTU | Envío de `d=timestamp,nivel` al servidor | Correcto |
| Sistema integrado | Lectura, almacenamiento y transmisión | Correcto |
| Store and Forward | Guardado y reenvío de pendientes | Correcto |

### Proyecto 4 (en curso)

Radar, módem + WiFi, envío a la PC y página probados en la placa. Pendientes: exactitud sobre agua (≥ 4 alturas × 10 lecturas, error ≤ ±10 mm), ≥ 10 cortes de red y de energía (0 perdidos / 0 duplicados), disponibilidad ≥ 95 % en 7 días, consumo y autonomía. Estado al día: [`docs/ESTADO.md`](docs/ESTADO.md); plan de ensayos: [`documentos/Reportes/P4_2doParcial/Plan_de_ensayos.md`](documentos/Reportes/P4_2doParcial/Plan_de_ensayos.md).

---

## Costos

Lista de materiales: `Hardware/V3_Estacion_Nivel/Estacion_Nivel_V3.csv` (desactualizada respecto a la placa fabricada; el `.kicad_pcb` es la referencia). Archivos de fabricación: `Hardware/V3_Estacion_Nivel/fabrication_files_G10/`.

---

## Equipo

| Integrante | Contacto |
| --- | --- |
| Héctor Dejesús Velázquez Ojeda | [hvelazquez@fiuna.edu.py](mailto:hvelazquez@fiuna.edu.py) |
| Mathias Ramón Aguilar DelValle | [maguilar@fiuna.edu.py](mailto:maguilar@fiuna.edu.py) |
| Mauricio Iván Tullo Estigarribia | [mauritullo@fiuna.edu.py](mailto:mtullo@fiuna.edu.py) |

Institución: Universidad Nacional de Asunción — Facultad de Ingeniería
Carrera: Ingeniería Mecatrónica
Cátedras: Proyecto 3 (2026-1C) y Proyecto 4 (2026-2C). Profesores de Proyecto 4: Prof. Ing. Federico Gaona, MSc. y Prof. Ing. Esteban Fretes.

---

## Licencia

- Firmware: MIT
- Documentación: MIT
