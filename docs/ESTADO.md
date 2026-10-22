# Estado del proyecto

Lo que está hecho, a medias y mediciones. Se actualiza al cerrar cada sesión (`/cerrar`).

## Movido desde CLAUDE.md (2026-10-22)

### Proyecto 3 (2026-1C)

- Terminado: PCB propia + firmware. Informes `documentos/Reportes/1f_*`, `2p_*`, `P3_final.pptx`.
- Pruebas de P3 (videos de sensor, SD, RTC, módem, sistema integrado, Store and Forward; ~9 meses de datos en Grafana) se usan como línea de base en P4.

### Evaluación de la primera entrega de P4 (20/09/2026)

- Nota oficial **46/50** (informe 21/25, diseño mecánico 25/25). En la planilla del evaluador: **71/100** en la rúbrica (= Σ peso × puntaje/5); 85,7 % es la valoración *relativa* al mejor equipo (82,8), no un puntaje propio.
- Criterios más bajos: **Validación y seguridad 2,5/5** y **Objetivos y alcance 3/5**. Mejora prioritaria según la planilla: "cerrar la metrología del nivel y demostrar recuperación de datos ante cortes".
- Comentario del Prof. Fretes: el resumen no debe empezar con "El presente informe…"; reducir los 15 objetivos específicos a 5–6, con al menos uno de electrónica, mecánica e interfaz (bien planteados: 1, 8, 10, 11, 13).
- Tarea del Prof. Gaona: de la "evaluación detallada", rescatar puntos a mejorar y los que quedan fuera de alcance, justificando. Se respondió con `documentos/Reportes/P4/Respuesta_evaluacion_1P_P4.pdf` (ver abajo).

### Nuevo sensor: radar Luda / POLIVIR FM02B (part number LDFM04_B)

- Nivel por radar 80 GHz: 0,1–40 m, ±3 mm, resolución 1 mm, haz 14°. Velocidad superficial por radar 24 GHz: 0,05–20 m/s, ±0,01 m/s.
- 9–24 V (típ. 12 V), 130–160 mA; **RS485 a 115200 baudios** (el ultrasónico de P3 era 9600). Peso 0,8 kg; incluye soporte orientable, placa para poste, 3 abrazaderas inoxidables y 5 m de cable. USD 690,41 + 64,80 envío.
- La ficha comercial tiene inconsistencias (rango de velocidad 0,05 vs 0,1 m/s; corriente 87–140 vs 130–160 mA) y **no trae el mapa de registros RS485**.
- Fotos y capturas de la ficha: `documentos/Reportes/P4/sensor_nuevo/`.
- El firmware todavía no está adaptado al radar.

### Respuesta a la evaluación detallada

- Entregable: `documentos/Reportes/P4/Respuesta_evaluacion_1P_P4.pdf`; fuente LaTeX en `documentos/Reportes/P4/latex_respuesta_evaluacion/` (zip `documentos/Reportes/P4/latex_respuesta_evaluacion_overleaf.zip`). Es para la entrega del 2.º parcial (16/11/2026).
- Cada una de las 10 debilidades de la planilla (PDF 1) se marca como REALIZADO / 2.º PARCIAL / FUERA DE ALCANCE. Las decisiones que tomó están en `docs/DECISIONES.md`.

### Informe de puntos de mejora

- Entregable: `documentos/Reportes/P4/Informe_puntos_de_mejora_P4.pdf`; fuente LaTeX `documentos/Reportes/P4/latex_puntos_mejora/main.tex` (+ `diagramas/diagrama_mejoras.tex` en TikZ, `figuras/`).

### Firmware

- Ambos proyectos (`Software/Firmware_Final/`, `Software/Firmware_Pruebas/`) compilan en Linux (verificado 2026-10-06) con `espressif32@6.13.0` (ESP-IDF 5.5.3).
- Intervalo de medición: 5 min (288 lecturas/día). En P3 era `2 * 60` para pruebas.
- El firmware **envía y guarda `d` crudo**; `Nivel = H − d` no está implementado.
- Trama actual: `d=<epoch>,<valor %.3f>\r\n` (la nueva trama acordada aún no está implementada, ver `docs/DECISIONES.md`).

### Hardware y documentación

- `Hardware/V3_Estacion_Nivel/DRC.rpt` es de una versión vieja (`Estacion_lluviaV2`); `ERC.rpt` tiene 4 errores de tipo `power_pin_not_driven` (faltan PWR_FLAG).
- El `README.md` raíz describe una estructura de `components/` ligeramente distinta a la real (nombres `led_blink`, `dtu`, `rtc`, `sensor` en Pruebas).
