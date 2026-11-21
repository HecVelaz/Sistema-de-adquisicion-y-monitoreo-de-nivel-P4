#!/usr/bin/env python3
"""Lee el radar FM02B directo desde la PC con un adaptador USB-RS485.

Sirve para el ensayo de exactitud sin la placa (no hace falta cargar el
modo banco). Lee por Modbus RTU la distancia (0x0001), la señal (0x0004) y
la velocidad filtrada (0x01F4-F5) cada INTERVALO segundos y guarda cada
lectura con el mismo formato que lecturas_pc.csv, así exactitud.py funciona
igual (con --archivo).

Conexión: verde -> A, amarillo -> B, negro -> GND del adaptador; rojo a +12 V
de la fuente (negro también a su GND). Radar: 115200 8N1, dirección 1.
Si no responde: invertir verde y amarillo y revisar el GND.

Uso (desde la raíz del repo; Ctrl+C para terminar):
  python3 Software/receptor_pc/leer_radar_pc.py                 # cada 2 s
  python3 Software/receptor_pc/leer_radar_pc.py --prueba        # una lectura, con las tramas en hex
  python3 Software/receptor_pc/exactitud.py --archivo lecturas_radar_pc.csv --ultimas 10 --ref 873
"""

import argparse
import csv
import os
import sys
import time
from datetime import datetime

import serial

REG_DISTANCIA = 0x0001   # mm, filtrada
REG_SENAL = 0x0004       # dB (> 30 = buena)
REG_VELOCIDAD = 0x01F4   # int32 BE con signo, x 0,001 m/s (2 registros)
DISTANCIA_MIN_MM = 100   # fuera de rango por debajo (igual que el firmware)

# Mismos códigos que el firmware (sensor_estado_t)
OK, SIN_RESPUESTA, RESPUESTA_INVALIDA, FUERA_DE_RANGO = 0, 1, 2, 3
ESTADOS = {0: "OK", 1: "SIN_RESPUESTA", 2: "RESPUESTA_INVALIDA", 3: "FUERA_DE_RANGO"}
CABECERA = ["hora_pc", "ip_placa", "epoch", "id", "distancia_m",
            "velocidad_ms", "senal_db", "estado"]


def crc16(datos):
    crc = 0xFFFF
    for b in datos:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def leer_registros(puerto, direccion, reg, cant, ver_hex=False):
    """Función 03. Devuelve (estado, lista de valores de 16 bits)."""
    trama = bytes([direccion, 0x03, reg >> 8, reg & 0xFF, 0x00, cant])
    c = crc16(trama)
    trama += bytes([c & 0xFF, c >> 8])

    puerto.reset_input_buffer()
    puerto.write(trama)   # 8 bytes: < 1 ms a 115200; la espera de la respuesta es el timeout
    esperado = 5 + 2 * cant
    resp = puerto.read(esperado)

    if ver_hex:
        print(f"  TX {trama.hex(' ').upper()}  ->  RX {resp.hex(' ').upper() or '(nada)'}")
    if not resp:
        return SIN_RESPUESTA, None
    if len(resp) >= 5 and resp[1] == 0x83:
        return RESPUESTA_INVALIDA, None          # excepción Modbus
    if len(resp) != esperado or resp[0] != direccion or resp[1] != 0x03 or resp[2] != 2 * cant:
        return RESPUESTA_INVALIDA, None
    if crc16(resp[:-2]) != (resp[-2] | (resp[-1] << 8)):
        return RESPUESTA_INVALIDA, None
    return OK, [(resp[3 + 2 * i] << 8) | resp[4 + 2 * i] for i in range(cant)]


def leer_radar(puerto, direccion, ver_hex=False):
    """Como sensor_rs485_leer() del firmware: el estado depende solo de la distancia."""
    lectura = {"dist": None, "vel": None, "senal": None}

    estado, v = leer_registros(puerto, direccion, REG_DISTANCIA, 1, ver_hex)
    if estado != OK:
        return estado, lectura
    if v[0] < DISTANCIA_MIN_MM:
        return FUERA_DE_RANGO, lectura
    lectura["dist"] = v[0] / 1000.0

    # Señal y velocidad: informativas, no invalidan la lectura
    estado_s, v = leer_registros(puerto, direccion, REG_SENAL, 1, ver_hex)
    if estado_s == OK:
        lectura["senal"] = v[0]
    estado_v, v = leer_registros(puerto, direccion, REG_VELOCIDAD, 2, ver_hex)
    if estado_v == OK:
        crudo = (v[0] << 16) | v[1]
        if crudo & 0x80000000:
            crudo -= 1 << 32
        lectura["vel"] = crudo / 1000.0
    return OK, lectura


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0],
                                 epilog=__doc__.split("\n\n", 1)[1],
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--puerto", default="/dev/ttyUSB0")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--direccion", type=int, default=1)
    ap.add_argument("--intervalo", type=float, default=2.0, help="segundos entre lecturas")
    ap.add_argument("--archivo", default="lecturas_radar_pc.csv")
    ap.add_argument("--prueba", action="store_true", help="una lectura mostrando las tramas, sin guardar")
    args = ap.parse_args()

    try:
        puerto = serial.Serial(args.puerto, args.baud, bytesize=8, parity="N",
                               stopbits=1, timeout=0.5)
    except serial.SerialException as e:
        sys.exit(f"No se pudo abrir {args.puerto}: {e}\n"
                 "¿Está conectado el adaptador? Ver con: ls /dev/ttyUSB*")

    if args.prueba:
        estado, l = leer_radar(puerto, args.direccion, ver_hex=True)
        print(f"Estado: {ESTADOS[estado]} | distancia {l['dist']} m | "
              f"señal {l['senal']} dB | velocidad {l['vel']} m/s")
        if estado == SIN_RESPUESTA:
            print("Sin respuesta: invertir verde/amarillo (A/B), revisar GND, 12 V y el puerto.")
        return

    nuevo = not os.path.exists(args.archivo) or os.path.getsize(args.archivo) == 0
    print(f"Leyendo el radar en {args.puerto} cada {args.intervalo:g} s, guardando en {args.archivo}",
          flush=True)

    with open(args.archivo, "a", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        if nuevo:
            w.writerow(CABECERA)
            f.flush()
        n = 0
        proxima = time.monotonic()
        try:
            while True:
                n += 1
                estado, l = leer_radar(puerto, args.direccion)
                hora = datetime.now().isoformat(timespec="seconds")
                fila = [hora, "usb-rs485", int(time.time()), n,
                        f"{l['dist']:.3f}" if l["dist"] is not None else "",
                        f"{l['vel']:.3f}" if l["vel"] is not None else "",
                        l["senal"] if l["senal"] is not None else "",
                        estado]
                w.writerow(fila)
                f.flush()
                print(f"{hora}  id {n:>5}  distancia {fila[4] or '-':>7} m  "
                      f"velocidad {fila[5] or '-':>7} m/s  senal {fila[6] if fila[6] != '' else '-':>3} dB  "
                      f"{ESTADOS[estado]}", flush=True)

                proxima += args.intervalo
                time.sleep(max(0.0, proxima - time.monotonic()))
        except KeyboardInterrupt:
            print("\nFin.")
        except serial.SerialException as e:
            print(f"\nSe perdió el puerto: {e}")


if __name__ == "__main__":
    main()
