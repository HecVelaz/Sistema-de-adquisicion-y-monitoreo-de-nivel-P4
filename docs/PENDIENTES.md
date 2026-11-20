# Pendientes

Tareas y próximos pasos, por prioridad. Lo terminado se borra de aquí y queda en `docs/BITACORA.md`.

## Ya

1. **Confirmar la fecha de entrega del 2.º parcial**: la registrada (16/11/2026) ya pasó. Define si entran los 7 días de disponibilidad.
2. **Revisar el día de datos** que dejó la placa sola (vista `Masi_estado_estacion`, huecos y duplicados).
3. **Cortes de energía** con el firmware nuevo (desenchufar la placa), incluso justo durante la escritura en la SD y el envío: al volver, el id sigue y no se pierden pendientes. Con el próximo corte de router, comprobar que la tarea de fondo ya no reintenta sin WiFi.
4. **Exactitud sobre agua** con el modo banco: ≥ 4 alturas × 10 lecturas contra cinta (≤ ±10 mm). Mirar la señal (> 30 dB); si sigue baja, subir la potencia de nivel (7 → 3 → 0).
5. **Disponibilidad 7 días** a 5 min (≥ 95 %), sin tocar la placa. Empezar cuando terminen los cortes y los cambios de firmware.

## Banco de laboratorio (validación)

6. Cortes de red hasta llegar a ≥ 10 (falta 9): sin internet con WiFi (cable del router), cortos (< 1 min) y largos (> 10 min).
7. Balde a ≤ 1,2 m, antena nivelada con burbuja; anotar roll/pitch (`0x0009`/`0x000A`) como referencia.
8. Si el error con agua es constante, corregir la calibración (hoy −46 mm, registro `0x07D8`) y anotar ambos valores.
9. Definir un umbral de señal mínima para marcar la lectura como dudosa.
10. Consumo del ESP32 con medidor USB; autonomía cuando llegue la UPS.

## Servidor, base de datos y documentación del sensor

11. Preguntar quién lee la tabla de la estación (¿la predicción?). Decidir si se borran los datos de banco (19/11 desde las 19:55 y los `-1` del año 2000) o si las pruebas van a una tabla aparte.
12. Pedir al administrador del servidor un script nuevo para la trama `d=<epoch>,<id>,<distancia>,<velocidad>,<estado>` (sin tocar el actual) y sugerirle mejoras de seguridad. Alternativa: servidor propio con la web.
13. Pedir al vendedor la hoja de datos de la versión exacta (serie y firmware V25). Leer `0xEA60–63` desde la placa.
14. Si la red bloquea NTP: tomar la hora del encabezado `Date` de la respuesta del servidor.

## Interfaz

15. Web propia: históricos, antigüedad del dato, estado (ya están la columna `estado` y la vista), alarma. Antes, resolver cómo lee la BD (acceso remoto o Grafana).
16. Disponibilidad de la línea de base de P3 (9 meses) por SQL: esperados vs recibidos por mes, cantidad y duración de huecos.

## Prolijidad

17. Guía del radar: agregar sin reposo, `0xEA60`, `0x07D8`, `0x07D7`, `0x07D9`.
18. Regenerar desde KiCad el esquemático PDF de `planos/` y el BOM `.csv`.
19. `README.md` raíz: módem → WiFi, estructura de `components/`, pinout de conectores.
20. `ERC.rpt` (faltan PWR_FLAG) y `DRC.rpt` viejo.
21. `sdkconfig` declara flash de 2 MB y el módulo tiene más; `Firmware_Pruebas/sdkconfig.defaults` habla de N8R8.
22. Verificar la pila BT1 (CR2032) del DS3231.
