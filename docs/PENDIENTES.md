# Pendientes

Tareas y próximos pasos, por prioridad. Lo terminado se borra de aquí y queda en `docs/BITACORA.md`.

## Ya

Procedimientos y tablas de los ensayos: `documentos/Reportes/P4_2doParcial/Plan_de_ensayos.md`.

1. **Confirmar la fecha de entrega del 2.º parcial**: la registrada (16/11/2026) ya pasó. Define si entran los 7 días de disponibilidad.
2. **Conectar el radar** (estuvo desconectado toda la tarde del 20/11) y revisar el día de datos que dejó la placa sola el 20/11 hasta ~18:00 (vista `Masi_estado_estacion`, huecos y duplicados).
3. **Cortes de energía** con el firmware nuevo (desenchufar la placa), incluso justo durante la escritura en la SD y el envío: al volver, el id sigue y no se pierden pendientes.
4. **WiFi caído con el módem activo**: el PHP tiene que seguir llegando por GPRS (y la página/PC se cortan). Y al revés: módem sin señal o apagado → el PHP sale por WiFi. Contar duplicados.
5. **Exactitud sobre agua** con el modo banco (lecturas cada 2 s en el receptor de la PC): ≥ 4 alturas × 10 lecturas contra cinta (≤ ±10 mm). Mirar la señal (> 30 dB); si sigue baja, subir la potencia de nivel (7 → 3 → 0).
6. **Disponibilidad 7 días** a 5 min (≥ 95 %), sin tocar la placa ni el USB (con su fuente). Empezar cuando terminen los cortes y los cambios de firmware.

## Banco de laboratorio (validación)

7. Cortes de red hasta llegar a ≥ 10 (falta 9): sin internet con WiFi (cable del router), cortos (< 1 min) y largos (> 10 min).
8. Balde a ≤ 1,2 m, antena nivelada con burbuja; anotar roll/pitch (`0x0009`/`0x000A`) como referencia.
9. Si el error con agua es constante, corregir la calibración (hoy −46 mm, registro `0x07D8`) y anotar ambos valores.
10. Definir un umbral de señal mínima para marcar la lectura como dudosa.
11. Consumo del ESP32 con medidor USB; autonomía cuando llegue la UPS.

## Servidor, base de datos y documentación del sensor

12. Preguntar quién lee la tabla de la estación (¿la predicción?). Decidir si se borran los datos que no son del arroyo (banco del 19/11 desde las 19:55, `-1` del año 2000, `-1` de la tarde del 20/11 con el radar desconectado y la trama de prueba del módem del 20/11 20:45:43) o si las pruebas van a una tabla aparte.
13. Pedir al administrador del servidor un script nuevo para la trama `d=<epoch>,<id>,<distancia>,<velocidad>,<estado>` (sin tocar el actual) y sugerirle mejoras de seguridad. Alternativa: servidor propio con la web.
14. Pedir al vendedor la hoja de datos de la versión exacta (serie y firmware V25). Leer `0xEA60–63` desde la placa.
15. Si la red bloquea NTP: tomar la hora del encabezado `Date` de la respuesta del servidor.

## Interfaz

16. Página (GitHub Pages) ya hecha: definir el **umbral de alarma** por nivel (`ALARMA_DISTANCIA_M` en `web/index.html`, hoy desactivado) y mirarla con el radar conectado. **Renovar el token de GitHub antes del 2027-01-19** (nuevo token en `credenciales.h` + recargar la placa).
17. Disponibilidad de la línea de base de P3 (9 meses) por SQL: esperados vs recibidos por mes, cantidad y duración de huecos.

## Prolijidad

18. Regenerar desde KiCad el esquemático PDF de `planos/` y el BOM `.csv`.
19. `ERC.rpt` (faltan PWR_FLAG) y `DRC.rpt` viejo.
20. `sdkconfig` declara flash de 2 MB y el módulo tiene más; `Firmware_Pruebas/sdkconfig.defaults` habla de N8R8.
21. Verificar la pila BT1 (CR2032) del DS3231.
