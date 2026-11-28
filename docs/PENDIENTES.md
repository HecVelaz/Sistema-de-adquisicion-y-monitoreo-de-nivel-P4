# Pendientes

Tareas y próximos pasos, por prioridad. Lo terminado se borra de aquí y queda en `docs/BITACORA.md`.

Calendario día por día hasta el 14/12: `documentos/Reportes/P4_Final/Pendientes_hasta_14_12.pdf`. Procedimientos de los ensayos: `documentos/Reportes/P4_2doParcial/Plan_de_ensayos.md` y, para la velocidad, el anexo A de `documentos/Reportes/P4_Final/Estructura_presentacion_final.md`.

## Antes del 7/12 (desde casa; 30/11–4/12 es la semana del robot)

1. **Preguntar a los profesores**: formato y duración de la presentación, si el informe final se entrega el 14/12, si se permite demo en vivo, y pedir la devolución del 2.º parcial.
2. **Averiguar qué pasó el 27/11 de 15:05 a 21:35** (distancia ~1,04 m).
3. Mirar la página una vez por día: que la estación siga enviando.
4. Agregar a `leer_radar_pc.py` la velocidad superficial `0x000F` (float LE).
5. Scripts de análisis: conciliación microSD ↔ BD ↔ página, disponibilidad real, gráficos de exactitud.
6. Elegir el lugar del ensayo de velocidad y juntar materiales (flotadores, cinta, marcas, celular a 60 fps, USB–RS485, alargue, fuente 12 V).
7. Adelantar informe final (correcciones de la devolución, secciones de diseño, mecánica, interfaz) y diapositivas de los bloques 1–3.

## Ensayos en la FIUNA (7/12–10/12)

8. **7/12**: copiar `datos.csv` de la microSD (y comprobar que se vuelve a montar); disponibilidad real de los 7 días y del total continuo. **Exactitud sobre agua**: 4 alturas × 10 lecturas contra cinta (≤ ±10 mm), nivelado con burbuja, anotar roll/pitch (`0x0009`/`0x000A`). Señal: probar potencia de nivel 7 → 3 → 0.
9. **8/12**: 9 cortes de red (cortos, largos, WiFi sin internet, módem sin señal, WiFi caído con el módem enviando) y 9 de energía (incluso durante la escritura en la SD y el envío). Consumo del ESP32 y del módem. A la noche: baterías nuevas y autonomía de la UPS.
10. **9/12**: **ensayo de velocidad** (flotador entre dos marcas alrededor de 0,7·H, ≥ 10 pasadas, lectura cada 0,2 s). Fotos y video del montaje, la página y el ensayo.
11. **10/12**: repetir lo que falló, video de respaldo de la demo, umbral de alarma de la página (`ALARMA_DISTANCIA_M`). Dejar la estación funcionando hasta el 14/12.
12. Si el error con agua es constante, corregir la calibración (hoy −46 mm, registro `0x07D8`) y anotar ambos valores. Definir un umbral de señal mínima para marcar la lectura como dudosa.

## Informe final y presentación (11/12–13/12)

13. Tabla ensayo / umbral / resultado / cumple y gráficos (radar vs. cinta, radar vs. flotador, serie continua con huecos, señal). Comparar con la línea de base de P3.
14. Informe final: Resultados (con velocidad), Conclusiones, resumen, abstract, cronograma real, anexos; revisión cruzada.
15. Presentación (~16 diapositivas + respaldo), dos ensayos cronometrados, respuestas a preguntas probables, video de respaldo en pendrive y Drive.
16. Cierre del repo: README, ESTADO, bitácora (incluida la sesión del 23/11, que no tiene entrada); revisar que no haya credenciales ni datos privados en el diff.

## Si sobra tiempo

17. Regenerar desde KiCad el esquemático PDF de `planos/` y el BOM `.csv`; `ERC.rpt` (faltan PWR_FLAG) y `DRC.rpt` viejo.
18. Disponibilidad de la línea de base de P3 (9 meses) por SQL: esperados vs recibidos por mes, cantidad y duración de huecos.
19. `sdkconfig` declara flash de 2 MB y el módulo tiene más; `Firmware_Pruebas/sdkconfig.defaults` habla de N8R8.
20. Verificar la pila BT1 (CR2032) del DS3231.
21. Revisar si el README raíz describe bien `components/` (en `Firmware_Pruebas` se llaman `led_blink`, `dtu`, `rtc`, `sensor`).

## Después del 14/12

22. **Renovar el token de GitHub antes del 2027-01-19** (nuevo token en `credenciales.h` + recargar la placa).
23. Preguntar quién lee la tabla de la estación (¿la predicción?) y decidir si se borran los datos que no son del arroyo o si las pruebas van a una tabla aparte.
24. Pedir al administrador del servidor un script nuevo para la trama `d=<epoch>,<id>,<distancia>,<velocidad>,<estado>` (sin tocar el actual) y sugerirle mejoras de seguridad.
25. Pedir al vendedor la hoja de datos de la versión exacta (serie y firmware V25). Leer `0xEA60–63` desde la placa.
26. Si la red bloquea NTP: tomar la hora del encabezado `Date` de la respuesta del servidor.
