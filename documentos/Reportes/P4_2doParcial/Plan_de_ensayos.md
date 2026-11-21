# Plan de ensayos — Proyecto 4 (2.º parcial)

Pasos y tablas para completar durante cada prueba del banco de laboratorio. Los umbrales son los que el equipo propuso en la respuesta a la evaluación del 1.er parcial (`P4_1erParcial/Respuesta_evaluacion_1P_P4.pdf`). Los resultados ya obtenidos se anotan también en `docs/ESTADO.md`.

| # | Ensayo | Criterio de aceptación | Estado |
| --- | --- | --- | --- |
| 1 | Exactitud del nivel | Error ≤ ±10 mm contra cinta métrica, en ≥ 4 alturas × 10 lecturas | Pendiente (1 altura en seco) |
| 2 | Cortes de red | ≥ 10 cortes: 0 perdidos, 0 duplicados; tiempo de recuperación medido | 1 de 10 |
| 3 | Cortes de energía | ≥ 10 cortes (incluso durante la escritura en la SD): id continuo, cola intacta | Pendiente |
| 4 | Disponibilidad | ≥ 95 % de lecturas recibidas en 7 días a 5 min | Pendiente |
| 5 | Consumo y autonomía | Consumo por módulo; autonomía estimada y medida con la UPS | Radar medido |
| 6 | Montaje | Antena nivelada (burbuja), haz dentro del balde | Pendiente |
| 7 | Interfaz | Página: antigüedad del dato, estado de la estación y del sensor, alarma | Pendiente de verificar |

**Antes de empezar cualquier ensayo**

- Anotar fecha, hora, versión del firmware (último commit) y red WiFi usada.
- Tener abierto el receptor en la PC (`python3 Software/receptor_pc/receptor_udp.py`): es la forma más rápida de ver cada lectura.
- Fuentes de verdad para contar datos: **SD** (`datos.csv`, tiene todo), **BD** (tabla `Masi`), **página** (`datos.json` de la rama `datos`) y **PC** (`lecturas_pc.csv`). Cada medición tiene un **id correlativo** y un epoch alineado a 5 min: un salto de id o un epoch faltante = dato perdido; el mismo epoch dos veces = duplicado.

---

## 1. Exactitud del nivel (banco con balde)

**Objetivo:** error de la distancia del radar ≤ ±10 mm contra la cinta métrica.

**Preparación**

1. Radar en el soporte, antena hacia abajo, **nivelada con burbuja en los dos ejes**. Leer y anotar roll/pitch (`0x0009`/`0x000A`) como referencia (ver ensayo 6).
2. Balde de ~30 cm de diámetro con agua, centrado bajo la antena. Distancia antena–agua **entre 0,1 m y ~1,2 m** (más lejos, el haz de 14° toca las paredes).
3. Sin metal, ventiladores ni aire acondicionado cerca del haz.
4. Firmware en **modo banco** (`PLATFORMIO_BUILD_FLAGS=-DMODO_BANCO pio run -t upload`): una lectura cada 2 s en el receptor de la PC.
5. Referencia de la cinta: medir desde la **cara inferior de la antena** hasta la superficie del agua, en el centro del balde. Anotar desde dónde se midió (el radar mide desde su propio plano de referencia y tiene una calibración de −46 mm).

**Procedimiento (por cada altura)**

1. Fijar el nivel de agua y esperar a que la superficie quede quieta (el radar filtra 128 muestras: esperar al menos 1 min después de cada cambio).
2. Medir con la cinta 3 veces y promediar → **d_ref**.
3. Tomar **10 lecturas consecutivas** del radar (20 s en modo banco) → anotar los ids del rango.
4. Anotar la **señal** (dB). Si es < 30 dB, anotarlo; si la señal es baja en todas las alturas, subir la potencia de nivel (7 → 3 → 0, registro `0x07D7`) y **repetir todas las alturas** con la nueva potencia.

**Tabla de resultados**

| Altura | d_ref cinta (mm) | Ids | Promedio radar (mm) | Desvío estándar (mm) | Error = radar − cinta (mm) | Máx. \|error\| (mm) | Señal (dB) | ¿≤ ±10 mm? |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 (~0,3 m) | | | | | | | | |
| 2 (~0,5 m) | | | | | | | | |
| 3 (~0,8 m) | | | | | | | | |
| 4 (~1,1 m) | | | | | | | | |

Potencia de nivel usada: ____ · Calibración del radar: −46 mm · Sensibilidad: 5

**Análisis**

- Si el error es **constante** en las 4 alturas (corrimiento), corregir la calibración (`0x07D8`, pasos de 1 mm), anotar el valor anterior y el nuevo y **repetir** el ensayo.
- Si el error **cambia con la altura**, revisar la nivelación y que el haz no toque el balde.
- El desvío estándar da la **repetibilidad**; el error promedio da la **exactitud**.

---

## 2. Cortes de red (Store and Forward)

**Objetivo:** ≥ 10 cortes de distintos tipos con 0 perdidos y 0 duplicados en la BD, y medir cuánto tarda en vaciarse la cola.

**Preparación:** firmware de producción con intervalo de 1 min para acelerar (`PLATFORMIO_BUILD_FLAGS=-DINTERVALO_MEDICION_MIN=1 pio run -t upload`); placa con su fuente; monitor serie abierto si se puede.

**Tipos de corte a cubrir** (el módem y el WiFi son caminos independientes hacia el PHP):

| Tipo | Cómo se provoca | Qué se espera |
| --- | --- | --- |
| A. WiFi caído, módem activo | Apagar el router / desconectar la red | El PHP sigue llegando por el **módem**; la PC y la página se cortan; nada a `temp.csv` |
| B. Módem caído, WiFi activo | Desconectar la antena o los 12 V del módem | `Modem sin OK. Reintentando por WiFi`; el PHP llega por WiFi |
| C. Los dos caídos, corto (< 1 min) | A + B a la vez | Datos a `temp.csv`; al volver, reenvío en orden |
| D. Los dos caídos, largo (> 10 min) | A + B a la vez | Cola de ≥ 10 datos; reenvío completo FIFO |
| E. Sin internet con WiFi conectado | Desenchufar el cable de internet del router | Igual que A (el WiFi no da `OK`) |

**Tabla de resultados**

| N.º | Tipo | Inicio | Fin | Duración | Datos en la cola | Reenviados | Perdidos | Duplicados en BD | Tiempo hasta cola vacía |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | WiFi (antes del módem) | 22:15:29 | 22:22:31 | 7 min | 7 | 7 | 0 | 0 | 1 min 36 s |
| 2 | | | | | | | | | |
| 3 | | | | | | | | | |
| 4 | | | | | | | | | |
| 5 | | | | | | | | | |
| 6 | | | | | | | | | |
| 7 | | | | | | | | | |
| 8 | | | | | | | | | |
| 9 | | | | | | | | | |
| 10 | | | | | | | | | |

**Cómo contar:** para el rango de horas del corte, comparar los epochs de `datos.csv` (SD) con los de la tabla `Masi`. Perdido = está en la SD y no en la BD; duplicado = el mismo epoch más de una vez en la BD. La columna `estado` marca `RECUPERADO` las filas que llegaron más de 120 s tarde.

---

## 3. Cortes de energía

**Objetivo:** ≥ 10 cortes de alimentación sin perder datos ni la cola, con el id continuo.

**Momentos a cubrir:** placa esperando el próximo intervalo · durante la lectura del radar · **durante la escritura en la SD** (justo en el segundo del intervalo) · durante el envío por el módem · con datos en `temp.csv` (combinar con un corte de red).

| N.º | Momento del corte | Hora | Id antes | Id después | ¿Id continuo? | Pendientes antes | Pendientes después | `datos.csv` legible | Observaciones |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | | | | | | | | | |
| 2 | | | | | | | | | |
| 3 | | | | | | | | | |
| 4 | | | | | | | | | |
| 5 | | | | | | | | | |
| 6 | | | | | | | | | |
| 7 | | | | | | | | | |
| 8 | | | | | | | | | |
| 9 | | | | | | | | | |
| 10 | | | | | | | | | |

Cada reinicio cuesta el intervalo en curso (la placa tarda en arrancar y sincronizar la hora): **no es una pérdida del Store and Forward**, pero hay que anotarlo.

---

## 4. Disponibilidad de 7 días

**Objetivo:** ≥ 95 % de las lecturas esperadas recibidas en la BD, a 5 min, sin tocar la placa.

- Lecturas esperadas: 7 días × 288 = **2016**.
- Disponibilidad = recibidas (epochs únicos en la BD dentro del período) / 2016 × 100.
- Empezar recién cuando no haya más cambios de firmware ni cortes provocados. Placa con su fuente (no el USB de la PC).

| Inicio | Fin | Esperadas | Recibidas en BD | Disponibilidad | Huecos (cantidad) | Hueco más largo | Lecturas con falla del sensor (`-1`) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| | | 2016 | | | | | |

Comparar también con la SD (¿cuántas se midieron?) y con la página (¿cuántas llegaron por WiFi?). Para la línea de base, repetir el mismo cálculo con los 9 meses de P3.

---

## 5. Consumo y autonomía

| Módulo | Tensión | Corriente medida | Potencia | Instrumento | Estado |
| --- | --- | --- | --- | --- | --- |
| Radar FM02B | 12 V | 126 mA | ~1,5 W | Fuente de laboratorio | Medido (sin reposo: es constante) |
| Módem DTU GPRS (reposo / transmitiendo) | 12 V | | | Fuente de laboratorio | Pendiente |
| ESP32-S3 + WiFi (promedio / pico) | 5 V | | | Medidor USB | Pendiente |
| **Total** | | | | | |

- Autonomía estimada con la UPS Marsriva KP1 Ultra (16 000 mAh): energía útil de la UPS / potencia total (considerar las pérdidas de conversión de cada salida).
- Autonomía medida: UPS cargada, red apagada, anotar la hora de la última lectura recibida.

| Inicio (UPS al 100 %) | Última lectura | Autonomía medida | Autonomía estimada |
| --- | --- | --- | --- |
| | | | |

---

## 6. Montaje del sensor

| Verificación | Valor / resultado |
| --- | --- |
| Nivel de burbuja en la cara superior, eje X | |
| Nivel de burbuja en la cara superior, eje Y | |
| Roll (`0x0009`, 0,1°) con el radar nivelado | |
| Pitch (`0x000A`, 0,1°) con el radar nivelado | |
| Ángulo de la app (≈ 55° nivelado, a confirmar) | |
| Altura antena–agua máxima usada (≤ 1,2 m con balde de 30 cm) | |
| Cable con bucle en "U" y sin peso sobre el conector | |
| Fotos del montaje (guardar en `P4_2doParcial/Fotos/bancada_de_prueba/`) | |

---

## 7. Interfaz (página de monitoreo)

| Verificación | Cómo | Resultado |
| --- | --- | --- |
| Muestra la última lectura (distancia, velocidad, señal) | Comparar con el receptor de la PC | |
| Antigüedad del dato | Debe crecer entre lecturas y volver a 0 con cada una | |
| Estación **CAÍDA** tras > 15 min sin datos | Desconectar la placa 20 min | |
| Falla del sensor | Desconectar el radar: aviso en rojo y estado `SIN RESPUESTA` | |
| Alarma por nivel | Fijar `ALARMA_DISTANCIA_M` y acercar el agua al radar | |
| Históricos 24 h / 7 días / 30 días con cortes en los huecos | Mirar después de un corte | |
| Retraso medición → página | Hora de la medición vs. hora en que aparece | |
