# CLAUDE.md — Informes de Proyecto 4

Reglas para los informes LaTeX de `documentos/Reportes/P4/`. Las reglas generales están en el `CLAUDE.md` raíz; qué entregables existen, en `docs/ESTADO.md`.

## Fuentes

- `latex_puntos_mejora/main.tex` (+ `diagramas/diagrama_mejoras.tex` en TikZ, `figuras/`) → `Informe_puntos_de_mejora_P4.pdf`. **La fuente es LaTeX**, no el borrador de claude.ai.
- `latex_respuesta_evaluacion/` → `Respuesta_evaluacion_1P_P4.pdf` (mismo flujo de compilación).

## Compilar (TeX Live instalado)

```bash
cd documentos/Reportes/P4/latex_puntos_mejora && pdflatex main.tex && pdflatex main.tex
```

Luego copiar `main.pdf` a `../Informe_puntos_de_mejora_P4.pdf` y regenerar `../latex_puntos_mejora_overleaf.zip` (en la respuesta a la evaluación: `../Respuesta_evaluacion_1P_P4.pdf` y `../latex_respuesta_evaluacion_overleaf.zip`). Revisar las páginas renderizadas (`pdftoppm`) antes de entregar.

## Detalles LaTeX

- `babel` en español rompe las flechas de TikZ → se usa `\usetikzlibrary{babel}`.
- Figuras/tablas con `[H]` (paquete `float`) y `\needspace` antes de las secciones para que el título no quede separado de su tabla.

## Estilo y contenido

- Estilo pedido por el usuario: corto, visual (imágenes/diagramas), lenguaje técnico pero simple, fuente LaTeX (Latin Modern), portada con logo FIUNA (`Hardware/V3_Estacion_Nivel/imagenes/Logo-fiuna.png`).
- **No escribir "Grupo 10"** (era de P3).
- No incluir **H** (Nivel = H − d) como contenido de informes; no mencionar certificación IP65; no mencionar el intervalo de 2 min de P3.
- El resumen no debe empezar con "El presente informe…" (comentario del Prof. Fretes).

## Datos privados

Esta carpeta tiene archivos privados en `.gitignore`: ver "Datos privados" en el `CLAUDE.md` raíz.
