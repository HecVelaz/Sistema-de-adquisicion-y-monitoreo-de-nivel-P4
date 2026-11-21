#!/usr/bin/env python3
"""Tabla de exactitud del radar a partir de lecturas_pc.csv (ensayo 1 del plan).

Para cada altura de agua se indica la distancia de referencia medida con la
cinta (mm) y qué lecturas del radar le corresponden. Calcula promedio, desvío
estándar, error (radar - cinta) y error máximo, y si cumple <= +-10 mm.
Solo se usan las lecturas con estado 0 (OK); las demás se cuentan aparte.

Formas de elegir las lecturas de cada altura:

  --medida NOMBRE REF_MM DESDE HASTA
      DESDE/HASTA son ids (p. ej. 31 40) o horas de la PC (p. ej. 22:31:10 22:31:30).
      Con ids se toma el bloque más reciente con esos ids (en modo banco el id
      vuelve a 1 en cada arranque). Con horas se usa la fecha de --fecha o, si
      no se da, la última fecha del archivo.

  --ultimas N --ref REF_MM [--nombre NOMBRE]
      Las últimas N lecturas OK del archivo. Útil durante la medición.

Ejemplos (desde la raíz del repo):
  python3 Software/receptor_pc/exactitud.py --ultimas 10 --ref 873
  python3 Software/receptor_pc/exactitud.py \\
      --medida "1 (~0,3 m)" 302 11 20 --medida "2 (~0,5 m)" 498 22:40:00 22:40:25
"""

import argparse
import csv
import math
import re
import sys
from datetime import datetime

TOLERANCIA_MM = 10.0
SENAL_MIN_DB = 30


def leer_csv(ruta):
    filas = []
    with open(ruta, newline="", encoding="utf-8") as f:
        for r in csv.DictReader(f):
            try:
                filas.append({
                    "hora": datetime.fromisoformat(r["hora_pc"]),
                    "id": int(r["id"]),
                    "dist": float(r["distancia_m"]) * 1000 if r["distancia_m"] else None,
                    "senal": int(r["senal_db"]) if r["senal_db"] else None,
                    "estado": int(r["estado"]),
                })
            except (KeyError, ValueError):
                continue   # línea mal formada: se ignora
    return filas


def por_ids(filas, desde, hasta):
    """Bloque más reciente de filas con id en [desde, hasta].

    Se recorre hacia atrás: el bloque termina al salir del rango o cuando el
    id deja de bajar (la placa se reinició y los ids volvieron a empezar).
    """
    bloque = []
    for f in reversed(filas):
        dentro = desde <= f["id"] <= hasta
        if dentro and (not bloque or f["id"] < bloque[-1]["id"]):
            bloque.append(f)
        elif bloque:
            break
    return list(reversed(bloque))


def por_horas(filas, fecha, desde, hasta):
    a = datetime.combine(fecha, desde)
    b = datetime.combine(fecha, hasta)
    return [f for f in filas if a <= f["hora"] <= b]


def hora(txt):
    for fmt in ("%H:%M:%S", "%H:%M"):
        try:
            return datetime.strptime(txt, fmt).time()
        except ValueError:
            pass
    return None


def calcular(nombre, ref_mm, sel):
    ok = [f for f in sel if f["estado"] == 0 and f["dist"] is not None]
    fila = {"nombre": nombre, "ref": ref_mm, "n": len(ok), "total": len(sel)}
    if ok:
        d = [f["dist"] for f in ok]
        prom = sum(d) / len(d)
        desv = math.sqrt(sum((x - prom) ** 2 for x in d) / (len(d) - 1)) if len(d) > 1 else 0.0
        senales = [f["senal"] for f in ok if f["senal"] is not None]
        fila.update(
            ids=f"{ok[0]['id']}–{ok[-1]['id']}",
            prom=prom, desv=desv, error=prom - ref_mm,
            maxerr=max(abs(x - ref_mm) for x in d),
            senal=sum(senales) / len(senales) if senales else None,
        )
    return fila


def num(x, dec=1):
    return f"{x:.{dec}f}".replace(".", ",")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0],
                                 epilog=__doc__.split("\n\n", 1)[1],
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--archivo", default="lecturas_pc.csv")
    ap.add_argument("--medida", nargs=4, action="append", default=[],
                    metavar=("NOMBRE", "REF_MM", "DESDE", "HASTA"))
    ap.add_argument("--ultimas", type=int)
    ap.add_argument("--ref", type=float, help="referencia de la cinta (mm) para --ultimas")
    ap.add_argument("--nombre", default="—")
    ap.add_argument("--fecha", help="AAAA-MM-DD para los rangos por hora (por defecto, la última del archivo)")
    args = ap.parse_args()

    filas = leer_csv(args.archivo)
    if not filas:
        sys.exit(f"Sin lecturas en {args.archivo}")
    fecha = datetime.fromisoformat(args.fecha).date() if args.fecha else filas[-1]["hora"].date()

    resultados = []
    if args.ultimas:
        if args.ref is None:
            sys.exit("--ultimas necesita --ref (distancia de la cinta en mm)")
        ok = [f for f in filas if f["estado"] == 0 and f["dist"] is not None]
        resultados.append(calcular(args.nombre, args.ref, ok[-args.ultimas:]))

    for nombre, ref, desde, hasta in args.medida:
        ref = float(ref.replace(",", "."))
        if re.fullmatch(r"\d+", desde) and re.fullmatch(r"\d+", hasta):
            sel = por_ids(filas, int(desde), int(hasta))
        elif hora(desde) and hora(hasta):
            sel = por_horas(filas, fecha, hora(desde), hora(hasta))
        else:
            sys.exit(f"Rango no válido en '{nombre}': use dos ids o dos horas (HH:MM[:SS])")
        resultados.append(calcular(nombre, ref, sel))

    if not resultados:
        ap.print_help()
        sys.exit(1)

    print("| Altura | d_ref cinta (mm) | Ids | N OK | Promedio radar (mm) | Desvío estándar (mm) "
          "| Error = radar − cinta (mm) | Máx. \\|error\\| (mm) | Señal (dB) | ¿≤ ±10 mm? |")
    print("| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |")
    avisos = []
    for r in resultados:
        if r["n"] == 0:
            print(f"| {r['nombre']} | {num(r['ref'])} | — | 0 de {r['total']} | — | — | — | — | — | — |")
            avisos.append(f"{r['nombre']}: no hay lecturas OK en el rango elegido.")
            continue
        cumple = "Sí" if r["maxerr"] <= TOLERANCIA_MM else "No"
        senal = num(r["senal"], 0) if r["senal"] is not None else "—"
        print(f"| {r['nombre']} | {num(r['ref'])} | {r['ids']} | {r['n']} de {r['total']} | {num(r['prom'])} "
              f"| {num(r['desv'])} | {num(r['error'])} | {num(r['maxerr'])} | {senal} | {cumple} |")
        if r["n"] < 10:
            avisos.append(f"{r['nombre']}: solo {r['n']} lecturas OK (el plan pide 10).")
        if r["total"] > r["n"]:
            avisos.append(f"{r['nombre']}: {r['total'] - r['n']} lecturas con falla en el rango (no se usaron).")
        if r["senal"] is not None and r["senal"] < SENAL_MIN_DB:
            avisos.append(f"{r['nombre']}: señal promedio {num(r['senal'], 0)} dB (< {SENAL_MIN_DB} dB).")

    con_datos = [r for r in resultados if r["n"]]
    if len(con_datos) > 1:
        errores = [r["error"] for r in con_datos]
        print(f"\nError promedio entre alturas: {num(sum(errores) / len(errores))} mm · "
              f"rango de errores: {num(min(errores))} a {num(max(errores))} mm "
              f"(si es casi constante, es un corrimiento: corregir la calibración 0x07D8).")
    for a in avisos:
        print("Aviso:", a)


if __name__ == "__main__":
    main()
