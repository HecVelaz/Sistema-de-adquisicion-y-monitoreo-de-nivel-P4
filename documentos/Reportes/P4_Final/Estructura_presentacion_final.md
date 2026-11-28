# Presentación final — Proyecto 4 (14/12/2026)

Estructura propuesta: **~16 diapositivas, 15–20 min + preguntas**. Base: el informe del 2.º parcial (`../P4_2doParcial/latex_informe_2P/`). Regla: una idea por diapositiva, poco texto, figuras y números grandes.

Reparto sugerido (cada uno presenta lo que hizo):
- **A**: introducción, problema, objetivos y arquitectura (1–5).
- **B**: radar, firmware y comunicación (6–9).
- **C**: mecánica e interfaz (10–11).
- **Todos**: ensayos, resultados y demo (12–15). Cierre: quien abrió.

## Bloque 1 — Contexto (≈ 3 min)

1. **Portada.** Título, integrantes, profesores, FIUNA, fecha.
2. **Problema.** Arroyo Mburicaó: crecidas en Asunción y por qué hace falta medir el nivel de forma continua. Foto del sitio (Colegio San Ignacio de Loyola).
3. **De P3 a P4.** P3: PCB + firmware + sensor ultrasónico, con 9 meses de datos (línea de base). P4 suma el **radar FM02B**, la **mecánica** (montaje) y la **interfaz** (web propia).
4. **Objetivos y alcance.** El objetivo general, los específicos medibles y lo que queda fuera (caudal, predicción: ver `docs/DECISIONES.md`).

## Bloque 2 — Diseño (≈ 6 min)

5. **Arquitectura.** El diagrama en bloques: UPS → radar / módem DTU / PCB ESP32-S3; RTC, microSD y LED. Los tres caminos del dato: servidor (GPRS/WiFi), página y PC.
6. **Por qué el radar.** La comparación de alternativas (ultrasónico vs radar): exactitud ±3 mm, sin contacto, sin influencia de la temperatura ni del viento. Datos técnicos: 80 GHz para nivel y 24 GHz para velocidad.
7. **Conexión y protocolo.** RS485/Modbus RTU a 115200, los registros que se leen (distancia, señal, velocidad) y la configuración leída por Bluetooth.
8. **Firmware.** El flujo del programa (diagrama), la trama `d=<epoch>,<valor>`, el respaldo en la microSD y el reenvío de pendientes. Las protecciones agregadas.
9. **Comunicación redundante.** GPRS y WiFi: cuál sale cuando el otro se cae. Una línea de cada modo.

## Bloque 3 — Mecánica e interfaz (≈ 3 min)

10. **Montaje del sensor.** El render y los planos acotados, la nivelación (antena de nivel perpendicular al agua y haz de velocidad a 55°) y una foto del montaje en el banco o instalado.
11. **Página de monitoreo.** Captura de la web: última lectura, antigüedad del dato, estado CAÍDA/SIN RESPUESTA e históricos. La ESP32 la alimenta sin la PC como intermediaria.

## Bloque 4 — Ensayos y resultados (≈ 5 min) — lo que más pesa

12. **Plan de ensayos y criterios.** Una tabla con cada ensayo, su umbral y el resultado (✔/✘). Sale de `../P4_2doParcial/Plan_de_ensayos.md`.
13. **Exactitud.**
    - Nivel: 4 alturas × 10 lecturas contra cinta métrica. Gráfico radar vs. cinta con la recta 1:1 y el error medio ± desviación. Umbral: ±10 mm.
    - **Velocidad**: radar vs. objeto cronometrado entre dos marcas (anexo A). Gráfico radar vs. referencia. Error en m/s y en %.
14. **Confiabilidad.** La disponibilidad de 7 días (%), los cortes de red y de energía (perdidos/duplicados = 0), la conciliación microSD ↔ base de datos ↔ página, el consumo y la autonomía de la UPS. Comparación con la línea de base de P3 (9 meses).
15. **Demo en vivo** (o video de respaldo si falla la red): el radar mide → la trama llega → la página se actualiza. Mover una tabla bajo el radar para que la distancia cambie en vivo.

## Bloque 5 — Cierre (≈ 1 min)

16. **Conclusiones y trabajo futuro.** Qué objetivos se cumplieron (con números), los problemas encontrados y cómo se resolvieron, y el trabajo futuro: caudal con la sección del arroyo, predicción (`Prediccion-de-nivel-de-rios`), script nuevo del servidor y alarma por nivel.

**Diapositivas de respaldo** (después del cierre, para las preguntas): pinout, mapa de registros Modbus, esquemático de la PCB, BOM y costo, y el cronograma real.

## Preguntas probables

- ¿Por qué el radar y no el ultrasónico de P3? ¿Cuánto mejoró?
- ¿Qué pasa si se cae la conexión? ¿Y si se corta la energía durante una escritura?
- ¿Cuánto aguanta la UPS? El radar no tiene modo de reposo (≈ 1,5 W continuos).
- ¿Cómo se sabe que el radar está nivelado y que no se movió? (roll/pitch `0x0009`/`0x000A`)
- ¿Cómo validaron la velocidad? ¿Por qué no calculan el caudal?
- ¿Qué pasa con la página cuando se venza el token de GitHub (19/01/2027)?

---

## Anexo A — Ensayo de velocidad (radar vs. objeto entre dos marcas)

**Idea:** un objeto recorre una distancia conocida *L* entre dos marcas, se cronometra el tiempo *t* y la referencia es *v = L / t*. Se compara con la velocidad del radar en el mismo momento.

**Cosas del radar a tener en cuenta**
- El haz de velocidad **no mira hacia abajo**: sale hacia adelante a ~55° de la horizontal y llega al agua a **L ≈ 0,7·H** delante del radar. Las dos marcas tienen que encerrar esa zona, no la vertical del radar.
- Mide la componente **a lo largo del eje del haz**: el objeto tiene que moverse en esa dirección (hacia el radar o alejándose), no de costado.
- Rango 0,05–20 m/s, exactitud ±0,01 m/s. Conviene probar 3 velocidades, por ejemplo ~0,2, ~0,5 y ~1 m/s.
- El firmware lee cada 5 min, así que no sirve para esto. Usar `Software/receptor_pc/leer_radar_pc.py --intervalo 0.2` con el adaptador USB–RS485, y sincronizar la hora de la PC con el video.
- El registro `0x01F4` está **filtrado** (promediado): con un paso corto puede no llegar al valor real. Si es así, agregar al script la lectura de la velocidad superficial sin filtrar (`0x000F`, float little-endian) y comparar las dos.

**Variante 1 — Banco (sin corriente):** arrastrar con un hilo, a velocidad lo más constante posible, una placa plana (mejor de metal o forrada en aluminio, que refleja más) a la altura del "agua", sobre unos 3–4 m. El radar ve el objeto en movimiento. Prueba que la medición Doppler y la corrección del ángulo son correctas. **Lo que no prueba:** que mida bien sobre el agua real.

**Variante 2 — Agua en movimiento (método del flotador), la más convincente:** en el arroyo o en un canal, soltar un flotador aguas arriba y cronometrar su paso entre dos marcas separadas 2–5 m, con el radar midiendo la misma zona.
- Usar un flotador **casi sumergido** (por ejemplo, una botella con agua hasta el ~80 %, de color visible) para que el viento no lo arrastre.
- Al menos **10 pasadas**. Informar la media y la desviación de cada método.
- El flotador da la velocidad **superficial**, que es la misma magnitud que mide el radar.

**Cronometraje:** grabar con el celular a 60 fps y contar cuadros (resolución de ~0,017 s), mejor que con un cronómetro a mano. Con *L* = 2 m y *v* = 0,5 m/s (*t* = 4 s), el error de tiempo es ~0,4 % y el de ±1 cm en *L*, ~0,5 %.

**Criterio sugerido:** |v_radar − v_ref| ≤ 0,05 m/s o ≤ 10 %, lo que sea mayor. El método del flotador tiene su propia incertidumbre, del orden del 10 %.

**Tabla de registro**

| Pasada | L (m) | t (s) | v_ref (m/s) | v_radar filtrada (m/s) | v_radar superficial (m/s) | Señal (dB) | Error (m/s) | Error (%) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | | | | | | | | |
