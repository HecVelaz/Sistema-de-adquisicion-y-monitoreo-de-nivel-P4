# Estado del proyecto — 2026-11-19

Foto del estado actual. Se reescribe al cerrar cada sesión (`/cerrar`); la historia está en `docs/BITACORA.md`.

## Contexto

- P3 terminado (línea de base). P4 1.er parcial: 46/50; lo más flojo fue validación y objetivos (detalle en la bitácora).
- Informes del 1.er parcial y su fuente LaTeX: `documentos/Reportes/P4_1erParcial/`. Material del 2.º parcial: `documentos/Reportes/P4_2doParcial/`.

## Radar FM02B (ya está en el laboratorio)

- Manuales con mapa de registros en `P4_2doParcial/`. Distancia `d` = registro `0x0001` (mm); señal `0x0004`.
- Configuración leída por Bluetooth: dirección 1, **115200** (confirmado), calibración **−46 mm** (origen desconocido), potencia de nivel **7 = mínima**.
- Lectura de prueba con la app: señal de nivel **4** (el manual pide > 30). No se sabe a qué apuntaba.
- **Todavía no se conectó a la PCB ni se probó con el firmware.**

## Placa V3

- Conectores reales: **J2** jack 12 V · **J9** USB-C o **J4** jack 5 V (uno solo) · **J3** radar (1 +12 V, 2 A, 3 B, 4 GND) · **J8** canal del módem, sin uso.
- El esquemático PDF de `Hardware/V3_Estacion_Nivel/planos/` y el BOM `.csv` están **desactualizados** (muestran J4/J5 de 2 polos); el `.kicad_pcb` y los `.kicad_sch` son los correctos.
- El DS3231 estaba en el año 2000: **sospecha de pila BT1 ausente o agotada** (sin verificar).
- `DRC.rpt` es de una versión vieja; `ERC.rpt` tiene 4 `power_pin_not_driven`.

## Firmware (`Software/Firmware_Final`)

- Compila (`espressif32@6.13.0`, ESP-IDF 5.5.3). App 1,03 MB en partición de 1,5 MB.
- **Radar**: driver nuevo (115200, `0x0001`, CRC, DE/RE por hardware). Compila; **sin probar con el radar**.
- **WiFi + HTTP**: probado en la placa con la red `FIUNA` (WPA2, RSSI −63 dBm): `HTTP 200 OK` del servidor. El módem DTU ya no se usa.
- **Hora**: NTP al arrancar → DS3231 en hora PY. Sin hora válida no mide. El usuario vio un dato llegar a las 19:55 con hora correcta.
- Credenciales WiFi y URL del servidor: solo en `components/wifi_envio/credenciales.h` (local, en `.gitignore`).
- Intervalo de medición 5 min. Se envía y guarda `d` crudo (`Nivel = H − d` no está en la placa).
- Trama vigente `d=<epoch>,<valor %.3f>`; si el sensor falla se envía `-1`. La trama nueva acordada no está implementada.
- Detalle técnico y particularidades: `Software/CLAUDE.md`.

## Base de datos

- Quedaron registros de prueba `-1.000` con fecha del año 2000 (epoch ~946 696 200), enviados antes del arreglo del reloj. **No se borraron todavía.**

## Documentación

- Guía de armado: `P4_2doParcial/Guia_conexion_radar_FM02B.md` y `.pdf`, con diagrama sobre la foto de la placa.
- `README.md` raíz: describe el módem DTU y una estructura de `components/` que ya no coincide.
