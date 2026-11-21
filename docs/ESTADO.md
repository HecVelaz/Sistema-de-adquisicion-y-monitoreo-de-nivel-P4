# Estado del proyecto — 2026-11-20 (noche)

Foto del estado actual. Se reescribe al cerrar cada sesión (`/cerrar`); la historia está en `docs/BITACORA.md`.

## Contexto

- P3 terminado (línea de base). P4 1.er parcial: 46/50; lo más flojo fue validación y objetivos.
- Informes del 1.er parcial y su fuente LaTeX: `documentos/Reportes/P4_1erParcial/` (incluye `Respuesta_evaluacion_1P_P4.pdf`, con lo prometido para el 2.º parcial). Material del 2.º parcial: `documentos/Reportes/P4_2doParcial/`.
- La fecha de entrega registrada (16/11/2026) ya pasó: falta confirmar la nueva.

## Radar FM02B

- Funciona con el firmware en J3 (115200, dirección 1). `0x0001` = distancia en mm, confirmado con cinta a una altura (0,197 m contra ~19,7 cm). **Hoy quedó desconectado**: todas las lecturas de la tarde son `-1` / estado 1.
- Señal 15–18 dB sobre superficies (el manual pide > 30); **todavía no se midió sobre agua**.
- Velocidad (`0x01F4`) se lee: 0,000 m/s en el banco.
- **Consumo: 126 mA a 12 V (~1,5 W)**. No tiene modo de reposo.
- Configuración (Bluetooth): calibración −46 mm, potencia de nivel 7 (mínima), sensibilidad 5. Sin cambios hasta medir con agua.
- No hay datasheet oficial publicado; los tres PDFs de `P4_2doParcial/` son la referencia.

## Placa V3

- Conectores: **J2** jack 12 V (solo radar) · **J9** USB-C o **J4** jack 5 V (ESP32) · **J3** radar (1 +12 V, 2 A, 3 B, 4 GND) · **J8** módem DTU (A/B; el módem toma 12 V aparte).
- Conectada al USB de la PC, tras cargar firmware a veces queda en modo descarga: **botón RESET**.
- El esquemático PDF de `Hardware/V3_Estacion_Nivel/planos/` y el BOM `.csv` están **desactualizados**; el `.kicad_pcb` y los `.kicad_sch` son los correctos.
- Pila BT1 del DS3231 sin verificar (el NTP corrige la hora al arrancar).

## Comunicación (firmware `Software/Firmware_Final`, commit `938929f`)

| Destino | Por dónde | Si falla |
| --- | --- | --- |
| PHP del servidor (`d=<epoch>,<distancia>`) | **Módem GPRS** (HTTPD) | WiFi; si no, cola en la SD |
| PC (línea completa, con velocidad) | WiFi, UDP broadcast :5005 | solo SD |
| Página GitHub Pages (línea completa) | WiFi, HTTPS (`repository_dispatch`) | solo SD y PC |
| SD `datos.csv` (`epoch,id,distancia,velocidad,señal,estado`) | — | — |

- **Probado en la placa** a las 21:05 (id 241): SD + PC + `Respuesta DTU: OK` (3,9 s) + página. Detalle técnico: `Software/CLAUDE.md`.
- WiFi: lista `WIFI_REDES` (FIUNA primero, `LabEquiposMedicos` de respaldo). Credenciales, URL, IP de la PC y token: solo en `credenciales.h`.
- Módem USR-GPRS232-730 configurado por AT: `HTTPD`, POST a `/level/insertarMasi.php`, timeout 10 s, APN `internet.tigo.py`, señal 25–27/31.
- Página: https://hecvelaz.github.io/Sistema-de-adquisicion-y-monitoreo-de-nivel-P4/ — datos en `datos.json` de la rama `datos` (30 días). **Token de GitHub vence el 2027-01-19**; al vencer la página deja de recibir (lo demás sigue).
- Receptor en la PC: `python3 Software/receptor_pc/receptor_udp.py` (opcional; nada depende de él).
- Modo banco (`-DMODO_BANCO`): cada 2 s a la PC, sin SD, PHP ni página. Intervalo de producción 5 min.
- Pendiente de ensayar: corte durante la escritura en la SD, corte de WiFi con el módem enviando.

## Ensayos

| Ensayo | Resultado | Falta |
| --- | --- | --- |
| Cortes de red | 1 de ≥ 10: 7/7 recuperados, 0 perdidos, 0 duplicados, 1 min 36 s (solo WiFi, antes del módem) | 9 más, incluido WiFi caído con módem activo |
| Cortes de energía | — | ≥ 10, incluso durante la escritura en la SD |
| Exactitud | 1 altura en seco | 4 alturas × 10 lecturas sobre agua |
| Consumo | Radar 126 mA | ESP32 (medidor USB), módem |
| Disponibilidad 7 días | — | Empezar después de los cortes |
| Autonomía | — | Falta la UPS |

## Base de datos

- Columna generada **`estado`** (OK / FALLA_SENSOR / RECUPERADO) y vista **`Masi_estado_estacion`** (EN LINEA / CAIDA, > 15 min sin datos). Sin permiso para disparadores.
- **Datos que no son del arroyo** en la tabla de la estación: banco desde el 19/11 19:55, `-1` con fecha del año 2000, `-1` de toda la tarde del 20/11 (radar desconectado) y la trama de prueba del módem (20/11 20:45:43). No se borraron: antes hay que saber si la predicción lee esa tabla.
- Respaldo local de la tabla del 19/11 (fuera del repo).

## Documentación

- Guía de armado: `P4_2doParcial/Guia_conexion_radar_FM02B.md` y `.pdf` (9 págs.), al día: radar, módem en J8, envíos, registros nuevos y fallas.
- **Plan de ensayos** con procedimientos y tablas para completar: `P4_2doParcial/Plan_de_ensayos.md`.
- `README.md` raíz: al día con P4 (arquitectura módem + WiFi + página, conectores, pinout, formatos).
- Manuales y programa del módem en `Software/Moden/` (subidos; `Gm3Cfg.ini` no, tiene la IP del servidor).
- En la respuesta a la evaluación se escribió "tenemos acceso al servidor para adaptarlo": no es así (solo a la BD). Explicarlo en el 2.º parcial.
