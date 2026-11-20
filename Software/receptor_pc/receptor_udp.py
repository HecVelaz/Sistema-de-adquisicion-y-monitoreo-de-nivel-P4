#!/usr/bin/env python3
"""Recibe por UDP las lecturas completas de la estacion y las guarda en CSV.

La ESP32 (Firmware_Final, funcion wifi_send_pc) manda una linea por medicion:
    epoch,id,distancia_m,velocidad_ms,senal_db,estado
(epoch 0 en modo banco; campo vacio si no hubo dato; estado 0 = OK,
1 = sin respuesta, 2 = respuesta invalida, 3 = fuera de rango).

Uso:  python3 receptor_udp.py [--puerto 5005] [--archivo lecturas_pc.csv]
La PC tiene que estar en la misma red WiFi que la placa y su IP debe ser
la que figura como PC_IP en credenciales.h. Ctrl+C para salir.
"""

import argparse
import csv
import os
import socket
from datetime import datetime

ESTADOS = {"0": "OK", "1": "SIN_RESPUESTA", "2": "RESPUESTA_INVALIDA", "3": "FUERA_DE_RANGO"}
CABECERA = ["hora_pc", "ip_placa", "epoch", "id", "distancia_m",
            "velocidad_ms", "senal_db", "estado"]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--puerto", type=int, default=5005)
    ap.add_argument("--archivo", default="lecturas_pc.csv")
    args = ap.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", args.puerto))

    nuevo = not os.path.exists(args.archivo) or os.path.getsize(args.archivo) == 0
    print(f"Escuchando UDP en el puerto {args.puerto}, guardando en {args.archivo}", flush=True)

    with open(args.archivo, "a", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        if nuevo:
            w.writerow(CABECERA)
            f.flush()
        try:
            while True:
                datos, (ip, _) = sock.recvfrom(512)
                linea = datos.decode("utf-8", errors="replace").strip()
                campos = linea.split(",")
                if len(campos) != 6:
                    print(f"[{ip}] trama ignorada: {linea!r}", flush=True)
                    continue

                hora = datetime.now().isoformat(timespec="seconds")
                w.writerow([hora, ip] + campos)
                f.flush()

                epoch, id_, dist, vel, senal, est = campos
                print(f"{hora}  id {id_:>5}  distancia {dist or '-':>7} m  "
                      f"velocidad {vel or '-':>7} m/s  senal {senal or '-':>3} dB  "
                      f"{ESTADOS.get(est, est)}", flush=True)
        except KeyboardInterrupt:
            print("\nFin.")


if __name__ == "__main__":
    main()
