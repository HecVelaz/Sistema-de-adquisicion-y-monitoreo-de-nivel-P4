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
