#!/usr/bin/env python3
"""Disponibilidad de datos de una estación (lecturas cada 5 min).

Lee un CSV exportado de phpMyAdmin con columnas "timeStamp","Nivel"
(hora local, "AAAA-MM-DD HH:MM:SS") y calcula:
  - lecturas esperadas, recibidas y disponibilidad (total y por mes),
  - huecos (cantidad, duración, los más largos) agrupados por duración,
  - registros repetidos en el mismo turno.

Cada registro se asigna al turno de 5 min más cercano (algunos llegaron con
segundos, p. ej. 08:34:46). Un turno con más de un registro cuenta una vez
como recibido y el resto como repetido.

Uso:
    python3 disponibilidad.py <datos.csv> <carpeta_figuras> [nombre]

Genera <nombre>_mensual.pdf y <nombre>_serie.pdf en la carpeta de figuras e
imprime las tablas en LaTeX. El CSV no se sube al repo.
"""
import csv
import sys
import datetime as dt
from collections import Counter, defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

PASO = 300  # s
MESES = ["ene", "feb", "mar", "abr", "may", "jun", "jul", "ago", "sep", "oct", "nov", "dic"]
RANGOS = [  # (hasta en min, etiqueta) para agrupar los huecos por duración
    (10, "1 lectura (5--10 min)"),
    (60, "hasta 1 h"),
    (6 * 60, "1--6 h"),
    (24 * 60, "6--24 h"),
    (float("inf"), "más de 24 h"),
]


def leer(ruta):
    with open(ruta, newline="") as f:
        filas = list(csv.reader(f))[1:]
    datos = []
    for t, v in filas:
        ts = dt.datetime.fromisoformat(t)
        datos.append((ts, float(v)))
    datos.sort()
    return datos


def a_turno(ts, origen):
    s = (ts - origen).total_seconds()
    return int(round(s / PASO))


def num(x, d=1):
    return f"{x:,.{d}f}".replace(",", "X").replace(".", ",").replace("X", ".")


def ent(x):
    return f"{x:,d}".replace(",", ".")


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    ruta, carpeta = sys.argv[1], sys.argv[2]
    nombre = sys.argv[3] if len(sys.argv) > 3 else "disp_p3"
    datos = leer(ruta)

    origen = datos[0][0].replace(second=0)
    origen -= dt.timedelta(minutes=origen.minute % 5)
    turnos = Counter(a_turno(ts, origen) for ts, _ in datos)
    n_turnos = max(turnos) + 1
    recibidos = len(turnos)
    repetidos = sum(c - 1 for c in turnos.values())
    fuera_grilla = sum(1 for ts, _ in datos if ts.second or ts.minute % 5)
    fin = origen + dt.timedelta(seconds=(n_turnos - 1) * PASO)

    # Por mes
    esp_mes, rec_mes = defaultdict(int), defaultdict(int)
    for k in range(n_turnos):
        t = origen + dt.timedelta(seconds=k * PASO)
        clave = (t.year, t.month)
        esp_mes[clave] += 1
        if k in turnos:
            rec_mes[clave] += 1

    # Huecos: secuencias de turnos sin dato
    presentes = sorted(turnos)
    huecos = []  # (inicio del primer turno faltante, cantidad de turnos)
    for a, b in zip(presentes, presentes[1:]):
        if b - a > 1:
            huecos.append((origen + dt.timedelta(seconds=(a + 1) * PASO), b - a - 1))

    grupos = [[0, 0] for _ in RANGOS]  # cantidad, turnos faltantes
    for _, n in huecos:
        dur = (n + 1) * 5  # min entre la última lectura y la siguiente
        for i, (lim, _) in enumerate(RANGOS):
            if dur <= lim:
                grupos[i][0] += 1
                grupos[i][1] += n
                break
    faltantes = n_turnos - recibidos

    # ---------------- Salida en texto / LaTeX ----------------
    print(f"% Período: {origen:%d/%m/%Y %H:%M} -> {fin:%d/%m/%Y %H:%M} "
          f"({num((fin - origen).total_seconds() / 86400, 0)} días)")
    print(f"% Esperadas {n_turnos}, recibidas {recibidos}, faltantes {faltantes}, "
          f"disponibilidad {100 * recibidos / n_turnos:.2f} %")
    print(f"% Registros {len(datos)}, repetidos {repetidos}, fuera de grilla {fuera_grilla}")
    print(f"% Huecos {len(huecos)}; valores min {min(v for _, v in datos)} max {max(v for _, v in datos)}")
    print()
    print("% --- Resumen ---")
    print(f"Período & {origen:%d/%m/%Y} -- {fin:%d/%m/%Y} "
          f"({num((fin - origen).total_seconds() / 86400, 0)} días) \\\\")
    print(f"Lecturas esperadas (1 cada 5 min) & {ent(n_turnos)} \\\\")
    print(f"Lecturas recibidas & {ent(recibidos)} \\\\")
    print(f"Lecturas faltantes & {ent(faltantes)} "
          f"($\\approx$ {num(faltantes * 5 / 1440, 1)} días) \\\\")
    print(f"\\textbf{{Disponibilidad}} & \\textbf{{{num(100 * recibidos / n_turnos, 1)}\\,\\%}} \\\\")
    print(f"Huecos (al menos 1 lectura faltante) & {ent(len(huecos))} \\\\")
    print(f"Registros repetidos en el mismo turno & {ent(repetidos)} \\\\")
    print()
    print("% --- Huecos por duración ---")
    for (lim, etq), (c, n) in zip(RANGOS, grupos):
        print(f"{etq} & {ent(c)} & {ent(n)} & {num(100 * n / faltantes, 1)}\\,\\% \\\\")
    print()
    print("% --- Huecos más largos ---")
    for ini, n in sorted(huecos, key=lambda h: -h[1])[:6]:
        fin_h = ini + dt.timedelta(seconds=n * PASO)
        print(f"{ini:%d/%m/%Y %H:%M} & {fin_h:%d/%m/%Y %H:%M} & {num(n * 5 / 60, 1)} h \\\\")
    print()
    print("% --- Por mes ---")
    for clave in sorted(esp_mes):
        e, r = esp_mes[clave], rec_mes[clave]
        print(f"% {MESES[clave[1] - 1]} {clave[0]}: {r}/{e} = {100 * r / e:.1f} %")

    # ---------------- Figuras ----------------
    plt.rcParams.update({"font.size": 9, "font.family": "serif"})
    azul, rojo, gris = "#1f5f99", "#c0392b", "#7f8c8d"

    # 1) Disponibilidad por mes
    claves = sorted(esp_mes)
    etiquetas = [f"{MESES[m - 1]}\n{a % 100:02d}" for a, m in claves]
    pct = [100 * rec_mes[c] / esp_mes[c] for c in claves]
    fig, ax = plt.subplots(figsize=(6.3, 2.6))
    barras = ax.bar(range(len(claves)), pct,
                    color=[azul if p >= 95 else rojo for p in pct], width=0.7)
    ax.axhline(95, color=gris, ls="--", lw=1)
    ax.text(len(claves) - 0.35, 95, "meta\n95 %", ha="left", va="center", color=gris, fontsize=8)
    ax.set_xlim(-0.6, len(claves) + 0.4)
    for b, p in zip(barras, pct):
        ax.text(b.get_x() + b.get_width() / 2, p + 1, f"{p:.0f}", ha="center", va="bottom", fontsize=8)
    # meses incompletos (primero y último) en tono claro
    for i in (0, len(claves) - 1):
        barras[i].set_alpha(0.45)
    ax.set_xticks(range(len(claves)), etiquetas)
    ax.set_ylim(0, 112)
    ax.set_ylabel("Disponibilidad (%)")
    ax.spines[["top", "right"]].set_visible(False)
    fig.tight_layout()
    fig.savefig(f"{carpeta}/{nombre}_mensual.pdf")
    plt.close(fig)

    # 2) Serie completa con los huecos de más de 1 h sombreados
    fig, ax = plt.subplots(figsize=(6.3, 2.6))
    xs, ys = [], []
    prev = None
    for ts, v in datos:
        if prev is not None and (ts - prev).total_seconds() > 3600:
            xs.append(prev + dt.timedelta(minutes=5)); ys.append(float("nan"))
        xs.append(ts); ys.append(v)
        prev = ts
    ax.plot(xs, ys, color=azul, lw=0.6)
    for ini, n in huecos:
        if n * 5 > 60:
            ax.axvspan(ini, ini + dt.timedelta(seconds=n * PASO), color=rojo, alpha=0.25, lw=0)
    ax.invert_yaxis()
    ax.set_ylabel("Distancia sensor–agua (m)\n(arriba = más agua)")
    ax.xaxis.set_major_locator(mdates.MonthLocator())
    ax.xaxis.set_major_formatter(plt.FuncFormatter(
        lambda x, _: f"{MESES[mdates.num2date(x).month - 1]}"))
    ax.spines[["top", "right"]].set_visible(False)
    ax.margins(x=0.01)
    fig.tight_layout()
    fig.savefig(f"{carpeta}/{nombre}_serie.pdf")
    plt.close(fig)


if __name__ == "__main__":
    main()
