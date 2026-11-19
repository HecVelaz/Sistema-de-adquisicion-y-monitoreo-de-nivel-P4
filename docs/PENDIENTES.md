# Pendientes

Tareas y próximos pasos, por prioridad. Lo terminado se borra de aquí y queda en `docs/BITACORA.md`.

## Ya

1. **Confirmar la fecha de entrega del 2.º parcial**: la registrada (16/11/2026) ya pasó.
2. **Borrar de la base los registros de prueba** con fecha del año 2000 y valor `-1` (epoch < 2025-01-01).
3. **Pila BT1** (CR2032) del DS3231: verificar o poner una nueva.
4. **Conectar el radar a J3** según la guía y ver en el monitor `Distancia: … | senal: …`.

## Banco de laboratorio (validación)

5. Balde a ≤ 1,2 m, antena nivelada con burbuja; confirmar si el ángulo ≈ 55° sirve como referencia de nivelación.
6. Señal de nivel sobre el agua: si sigue baja, subir la potencia de emisión de nivel (7 → 3 → 0).
7. Exactitud vs cinta métrica a 3–4 alturas: objetivo ±10 mm. Si el error es constante, corregir la calibración (hoy −46 mm) y anotar ambos valores.
8. Definir un umbral de señal mínima para marcar la lectura como inválida.

## Firmware y servidor

9. Trama nueva `d=<epoch>,<id>,<distancia>,<velocidad>,<estado>` (ID para detectar perdidos/duplicados; estado en vez de `-1`) y adaptar el PHP.
10. Ensayos: ≥ 10 cortes (perdidos/duplicados, tiempo de recuperación), disponibilidad 7 días (≥ 95 %), consumo y autonomía con el UPS.
11. Si la red bloquea NTP: tomar la hora del encabezado `Date` de la respuesta del servidor.
12. Sugerir al dueño del servidor que `insertarMasi.php` pida una clave (hoy inserta sin autenticación).

## Interfaz

13. Web propia del equipo: históricos, antigüedad del dato, estado del sensor, alarma; probar con la BD de P3 y luego con el radar.

## Prolijidad

14. Regenerar desde KiCad el esquemático PDF de `planos/` y el BOM `.csv` (desactualizados respecto de la placa).
15. `README.md` raíz: módem → WiFi, estructura de `components/`, pinout de conectores.
16. `ERC.rpt` (faltan PWR_FLAG) y `DRC.rpt` viejo.
17. `sdkconfig` declara flash de 2 MB y el módulo tiene más (el bootloader ve 8 MB); `Firmware_Pruebas/sdkconfig.defaults` habla de N8R8.
