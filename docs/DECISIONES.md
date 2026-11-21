# Decisiones

Por qué se eligió algo y qué se descartó. No proponer revertirlas sin hablarlo con el equipo.

## Movido desde CLAUDE.md (2026-10-22)

### Alcance de Proyecto 4 (acordado con el usuario)

- Foco de P4: **validación** (resultados medibles) y **objetivos**. La mecánica se reduce al **montaje del sensor** con su kit comercial; no hay gabinete/soporte nuevo que diseñar ni cálculo por viento.
- **Fuera de alcance:** montaje en el arroyo (depende del Depto. de Mecánica y Energía y del grupo de investigación Mburicaó → se demuestra en un **banco de laboratorio** que simula la instalación), medir caudal, calibrar en todo el rango 0,1–40 m, usuarios/permisos en la web, validar la predicción con IA (es del proyecto de investigación), panel solar. No mencionar certificación IP65.
- La web "MburicaoCastAI" es del grupo de investigación y el modelo de predicción no cuenta para este proyecto académico: ambos solo se citan. La interfaz de P4 es una **web propia** del equipo (se prueba con la BD histórica de P3 y luego con los datos del radar).
- Las pruebas de P3 se usan como línea de base; lo que falta es cuantificar: error en mm, perdidos/duplicados en ≥ 10 cortes, tiempo de recuperación, % de disponibilidad, consumo y autonomía.
- **H** (Nivel = H − d) depende del lugar de instalación y el equipo ya sabe cómo obtenerlo; no se incluye como contenido de informes.

### Objetivos reformulados (5)

Electrónica (integración + consumo/autonomía), Firmware (Store and Forward con ID, 0 perdidos/duplicados en ≥ 10 cortes), Montaje del sensor (banco de laboratorio, antena nivelada), Interfaz (históricos, antigüedad del dato, estado del sensor, alarma), Validación (error ≤ ±10 mm vs cinta métrica en banco). Los umbrales son propuestas del equipo.

### Decisiones de la respuesta a la evaluación detallada

- Trama nueva `d=<epoch>,<id>,<distancia>,<velocidad>,<estado>` (el servidor es modificable).
- RTC +3 h se mantiene (Paraguay UTC−3 fijo) y se documenta.
- Disponibilidad objetivo ≥ 95 % en 7 días.
- Alimentación de respaldo: Mini UPS Marsriva KP1 Ultra (16 000 mAh, 30 W).
- Banco con un balde (haz de 14° → con balde de 30 cm, hasta ~1 m); pruebas solo en laboratorio.
- El plan de datos del módem lo paga la Facultad.
- Los profesores de hidrología tienen otros sistemas de nivel para comparar.
- Como el radar llega en noviembre, cortes, disponibilidad (7 días) y autonomía se ensayan antes con el sensor ultrasónico de P3.

### Informes

- La fuente del informe de puntos de mejora es LaTeX; el documento de claude.ai que se usó como borrador ya no es la fuente.

### Firmware

- La plataforma está fijada en `espressif32@6.13.0` (ESP-IDF 5.5.3, la versión validada en P3): sin el pin PlatformIO baja ESP-IDF 6.x, donde el componente `driver` ya no exporta `driver/gpio.h` y el build falla.

## 2026-11-19

### Comunicación y hora

- ~~**Envío por WiFi del ESP32-S3 en lugar del módem DTU** (decisión del equipo "por ahora"). `modem_dtu` queda en el árbol sin usar; J8 y UART2 libres.~~ Reemplazada el 2026-11-20 (tarde): el PHP vuelve a ir por el módem, con respaldo WiFi.
- Se mantiene el mismo script del servidor (`insertarMasi.php`) y la trama `d=<epoch>,<valor>`. Como no se conoce el PHP, el firmware hace un POST con `d=` en la URL **y** en el cuerpo (sirve con `$_GET`, `$_POST` o `$_REQUEST`, una sola inserción).
- **Hora por NTP** al arrancar y escrita en el DS3231 (hora PY, se conserva el +3 h al enviar). **Sin hora válida (año < 2025) no se mide**, y los pendientes con fecha < 2025-01-01 se descartan en vez de enviarse. Motivo: el RTC estaba en el año 2000 y se enviaron datos con esa fecha.

### Radar

- Se lee solo la distancia `d` (`0x0001`); la señal (`0x0004`) solo se registra en el log hasta definir un umbral en el banco.
- No se cambian parámetros del radar (calibración −46 mm, potencias) hasta medir en el banco con cinta métrica.

## 2026-11-20

### Trama, servidor y base de datos

- **El script del servidor no se puede editar** (solo hay acceso a la BD) y acepta exactamente 2 campos. Por eso **la trama sigue siendo `d=<epoch>,<valor>`** y queda en suspenso la trama de 5 campos de 2026-10 hasta que el administrador suba un script nuevo.
- Mientras tanto: **id, velocidad, señal y estado se guardan en la SD** (`datos.csv`). Perdidos y duplicados se verifican por la hora (los intervalos son fijos) y cruzando la SD con la BD.
- **Estado en la BD con una columna generada** (no hay permiso para disparadores) y **caída de la estación con una vista** (> 15 min sin datos = 3 lecturas, como se prometió para la web).
- **Sin índices `UNIQUE` en la tabla**: ante un repetido el script no respondería `OK` y la placa reintentaría el dato para siempre, bloqueando la cola.

### Firmware

- El intervalo de producción sigue en **5 min**; 1 min solo para pruebas, con la opción de compilación.
- No se agrega OTA por ahora (requiere cambiar la tabla de particiones).

## 2026-11-20 (tarde)

### Comunicación (decisiones del usuario)

- **PHP por el módem GPRS y, si no hay `OK`, por WiFi**; si fallan los dos, cola en la SD. Motivo: que el dato llegue al servidor aunque no haya WiFi. Riesgo aceptado: si el módem insertó pero su `OK` se perdió, el WiFi duplica (se detecta por la hora).
- **WiFi para la PC y la página**: el módem solo hace HTTP a un servidor y GitHub exige HTTPS.
- **Lectura completa a la PC por UDP broadcast** (sin cola ni reintento: la SD tiene todo). Se descartó Tailscale (no corre en la ESP32) y, por ahora, MQTT.
- **Lista de redes WiFi** en orden de preferencia (FIUNA primero, para estar en la misma red que la PC); no un portal de configuración.
- **Página de monitoreo en GitHub Pages, en este repo**, alimentada **directo por la ESP32** (`repository_dispatch` → Action → `datos.json` en la rama `datos`), no con la PC como puente, para que no dependa de la PC. Se descartó leer la BD con Actions porque la velocidad no está en la BD. La rama `datos` se reescribe con `--force` (un solo commit) para que el repo no crezca.
- **Token de GitHub *fine-grained***, solo este repo, Contents: Read and write; solo en `credenciales.h`. Vence el 2027-01-19.

## Movido desde CLAUDE.md (2026-11-21)

Desde `Software/CLAUDE.md`.

- Hora válida obligatoria: el 19/11/2026 el RTC estaba en el año 2000 (¿pila BT1?) y se enviaron `-1` con esas fechas a la base.
- Main task stack de 3584 a 8192 por el cliente HTTP.
