# Estado del proyecto — 2026-11-28

Foto del estado actual. Se reescribe al cerrar cada sesión (`/cerrar`); la historia está en `docs/BITACORA.md`.

## Contexto

- P3 terminado (línea de base). P4 1.er parcial: 46/50. **2.º parcial entregado; promedio de proceso 74 % → derecho a final.**
- **Final: lunes 14/12/2026** — presentación + informe final (confirmar con los profesores formato, duración, si el informe se entrega ese día y si se permite demo en vivo).
- **Del 30/11 al 4/12 el equipo está con el robot de Robótica 1 y 2**: los ensayos van del **7/12 al 10/12**; informe y presentación, del 11/12 al 13/12.
- Plan día por día: `documentos/Reportes/P4_Final/Pendientes_hasta_14_12.pdf`. Estructura de la presentación y ensayo de velocidad: `P4_Final/Estructura_presentacion_final.md`.
- Informe del 2.º parcial: `documentos/Reportes/P4_2doParcial/Informe_2doParcial_P4.pdf` (fuente `latex_informe_2P/`). Material del 1.er parcial: `P4_1erParcial/`.

## Estación en el banco (FIUNA)

- **Funcionando sola desde el 20/11** con el firmware de producción (5 min). Al 28/11 12:50: id 2434. Dejarla así hasta el 7/12.
- **Prueba de 7 días (20/11 22:00 → 27/11 22:00): 98,0 % según la página** (1975 de 2016; umbral 95 %). La cifra real sale de la microSD y la BD (copiar `datos.csv` el 7/12).
- 24 huecos de 10–60 min en la página y 31 ids que no llegaron; 0 duplicados. Falta ver si están en la SD.
- Distancia 0,871–0,881 m, salvo el **27/11 15:05–21:35 (~1,04 m)**: averiguar si alguien tocó el balde o el radar.
- Desde casa no se puede leer la microSD; sí la copia de la página (`datos.json` de la rama `datos`, comando en la bitácora del 28/11).

## Radar FM02B

- Firmware en J3 (115200, dirección 1). `0x0001` = distancia en mm; velocidad `0x01F4` (filtrada, × 0,001 m/s).
- **Señal 15–23 dB** (el manual pide > 30). Todavía no se midió la exactitud **sobre agua**.
- Velocidad: 0,000 en el banco salvo picos aislados de ±0,16–0,22 m/s. **Sin validar**: falta el ensayo con agua en movimiento.
- Consumo 126 mA a 12 V (~1,5 W), sin modo de reposo.
- Configuración (Bluetooth): calibración −46 mm, potencia de nivel 7 (mínima), sensibilidad 5.
- No hay datasheet oficial de la versión exacta; los tres PDFs de `P4_2doParcial/` son la referencia.

## Placa V3

- Conectores: **J2** jack 12 V (solo radar) · **J9** USB-C o **J4** jack 5 V (ESP32) · **J3** radar (1 +12 V, 2 A, 3 B, 4 GND) · **J8** módem DTU (A/B; el módem toma 12 V aparte).
- Tras cargar firmware a veces queda en modo descarga: **botón RESET**.
- Esquemático PDF de `Hardware/V3_Estacion_Nivel/planos/` y BOM `.csv` **desactualizados**; el `.kicad_pcb` y los `.kicad_sch` son los correctos.
- Pila BT1 del DS3231 sin verificar (el NTP corrige la hora al arrancar).

## Comunicación (firmware `Software/Firmware_Final`)

| Destino | Por dónde | Si falla |
| --- | --- | --- |
| PHP del servidor (`d=<epoch>,<distancia>`) | **Módem GPRS** (HTTPD) | WiFi; si no, cola en la SD |
| PC (línea completa, con velocidad) | WiFi, UDP broadcast :5005 | solo SD |
| Página GitHub Pages (línea completa) | WiFi, HTTPS (`repository_dispatch`) | solo SD y PC |
| SD `datos.csv` (`epoch,id,distancia,velocidad,señal,estado`) | — | — |

- Credenciales, URL, IP de la PC y token: solo en `credenciales.h`. Módem: `Software/Moden/Moden.md`.
- Página: https://hecvelaz.github.io/Sistema-de-adquisicion-y-monitoreo-de-nivel-P4/ (30 días). **Token de GitHub vence el 2027-01-19.**
- `leer_radar_pc.py` lee el radar con un adaptador USB–RS485 (exactitud y velocidad; `--intervalo 0.2`). Modo banco (`-DMODO_BANCO`): cada 2 s a la PC.

## Ensayos

| Ensayo | Resultado | Falta (fecha prevista) |
| --- | --- | --- |
| Disponibilidad 7 días | 98,0 % (página) | Confirmar con SD y BD (7/12) |
| Exactitud | 1 altura en seco | 4 alturas × 10 lecturas sobre agua (7/12) |
| Cortes de red | 1 de ≥ 10, 0 perdidos, 0 duplicados | 9 más, incluido WiFi caído con módem activo (8/12) |
| Cortes de energía | — | ≥ 10, incluso durante la escritura en la SD (8/12) |
| Consumo | Radar 126 mA | ESP32 y módem (8/12) |
| Autonomía | — | Baterías nuevas en la UPS (noche del 8/12) |
| Velocidad | — | Flotador entre dos marcas vs. radar (9/12) |

## Base de datos

- Columna generada `estado` y vista `Masi_estado_estacion` (EN LINEA / CAIDA, > 15 min sin datos).
- Hay **datos que no son del arroyo** en la tabla (banco desde el 19/11, `-1` del año 2000 y del 20/11, trama de prueba del módem). No se borran hasta saber si la predicción lee esa tabla.

## Documentación

- Guía de armado: `P4_2doParcial/Guia_conexion_radar_FM02B.md/.pdf`. Plan de ensayos: `P4_2doParcial/Plan_de_ensayos.md` (todavía sin el de velocidad; está en el anexo A de la estructura).
- Datos privados fuera del repo (en `.gitignore`): planillas de notas, `prom.png`, capturas con datos de terceros.
