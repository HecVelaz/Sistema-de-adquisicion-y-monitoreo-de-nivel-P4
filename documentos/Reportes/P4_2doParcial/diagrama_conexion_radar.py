# Genera diagrama_conexion_radar.jpg sobre la foto de la placa.
# Uso: python3 diagrama_conexion_radar.py Fotos salida.png  (requiere Pillow y fuentes DejaVu)
import sys
from PIL import Image, ImageDraw, ImageFont, ImageFilter

FOTOS = sys.argv[1]
SALIDA = sys.argv[2]

F = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FB = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
def fnt(sz, b=False): return ImageFont.truetype(FB if b else F, sz)

ROJO, VERDE, AMAR, NEGRO = (214, 40, 40), (22, 150, 90), (240, 190, 0), (25, 25, 25)
GRIS, TXT, AZUL, NARANJA = (120, 120, 120), (30, 30, 30), (25, 90, 200), (210, 110, 0)

# ---------------- lienzo y foto de la placa ----------------
W, H = 2110, 1720
img = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(img)

placa = Image.open(f"{FOTOS}/placa.jpeg")
CX0, CY0, CX1, CY1 = 0, 225, 1201, 1395
S = 0.80
OX, OY = 420, 110
foto = placa.crop((CX0, CY0, CX1, CY1))
foto = foto.resize((int(foto.width * S), int(foto.height * S)), Image.LANCZOS)
img.paste(foto, (OX, OY))

def P(x, y):  # coordenadas de la foto original -> lienzo
    return (OX + (x - CX0) * S, OY + (y - CY0) * S)

d.text((W / 2, 40), "Conexión del radar FM02B a la PCB V3 (vista superior)",
       font=fnt(44, True), fill=TXT, anchor="mm")

def flecha(p0, p1, color, w=5):
    d.line([p0, p1], fill=color, width=w)
    import math
    ang = math.atan2(p1[1] - p0[1], p1[0] - p0[0])
    L = 22
    a1 = (p1[0] - L * math.cos(ang - 0.45), p1[1] - L * math.sin(ang - 0.45))
    a2 = (p1[0] - L * math.cos(ang + 0.45), p1[1] - L * math.sin(ang + 0.45))
    d.polygon([p1, a1, a2], fill=color)

def caja(x, y, lineas, color, ancho=None, sz=27):
    f1, f2 = fnt(sz, True), fnt(sz - 4)
    alto = 14 + len(lineas) * (sz + 8)
    ancho = ancho or max(d.textlength(t, font=(f1 if i == 0 else f2)) for i, t in enumerate(lineas)) + 30
    d.rounded_rectangle([x, y, x + ancho, y + alto], 12, fill="white", outline=color, width=4)
    for i, t in enumerate(lineas):
        d.text((x + 15, y + 10 + i * (sz + 8)), t, font=(f1 if i == 0 else f2), fill=(color if i == 0 else TXT))
    return (x, y, x + ancho, y + alto)

def marco(cx, cy, w, h, color):
    d.rounded_rectangle([cx - w / 2, cy - h / 2, cx + w / 2, cy + h / 2], 10, outline=color, width=5)

# ---------------- J2: 12 V ----------------
j2 = P(110, 430)
marco(j2[0] + 5, j2[1], 150, 120, ROJO)
b = caja(20, 300, ["J2 · entrada 12 V", "Salida 12 V del UPS", "(plug DC; + al centro,", " verificar antes)"], ROJO)
flecha((b[2], (b[1] + b[3]) / 2), (j2[0] - 70, j2[1]), ROJO)

# ---------------- J9 / J4: 5 V ----------------
j9 = P(1062, 840)
marco(j9[0], j9[1], 70, 130, AZUL)
j4 = P(1045, 1110)
marco(j4[0], j4[1], 170, 150, AZUL)
b = caja(1440, 560, ["J9 (USB-C) o J4 · 5 V", "Salida USB del UPS", "Alimenta ESP32 + WiFi", "Usar UNO de los dos"], AZUL)
flecha((b[0], (b[1] + b[3]) / 2 - 20), (j9[0] + 40, j9[1]), AZUL)
flecha((b[0], (b[1] + b[3]) / 2 + 40), (j4[0] + 90, j4[1] - 30), AZUL)

# ---------------- antena WiFi ----------------
ant = P(955, 285)
marco(ant[0], ant[1], 170, 70, NARANJA)
b = caja(1440, 150, ["Antena WiFi (ESP32-S3)", "No taparla con metal", "Solo redes de 2,4 GHz"], NARANJA)
flecha((b[0], (b[1] + b[3]) / 2), (ant[0] + 90, ant[1]), NARANJA)

# ---------------- J8: no usar ----------------
j8c = P(290, 1290)
marco(j8c[0], j8c[1], 245, 150, GRIS)
d.line([(j8c[0] - 115, j8c[1] - 68), (j8c[0] + 115, j8c[1] + 68)], fill=GRIS, width=5)
d.line([(j8c[0] - 115, j8c[1] + 68), (j8c[0] + 115, j8c[1] - 68)], fill=GRIS, width=5)
b = caja(20, 790, ["J8 · NO USAR", "Canal RS485 del módem", "(sin uso con WiFi)"], GRIS)
flecha((b[2], (b[1] + b[3]) / 2), (j8c[0] - 128, j8c[1] - 30), GRIS)

# ---------------- J3: sensor ----------------
j3c = P(586, 1290)
marco(j3c[0], j3c[1], 250, 150, VERDE)
b = caja(1030, 1052, ["J3 · RADAR (canal RS485 #1)"], VERDE, sz=24)
flecha((b[0], (b[1] + b[3]) / 2), (j3c[0] + 128, j3c[1] + 40), VERDE)

tornillos = [492, 555, 618, 680]
colores = [ROJO, VERDE, AMAR, NEGRO]
nombres = ["1  +12 V", "2  A (485A)", "3  B (485B)", "4  GND"]
hilos = ["rojo", "verde", "amarillo", "negro"]
PIN_AV = ["1", "3", "4", "2"]  # manual: 1 rojo, 2 negro, 3 verde, 4 amarillo
COLOR_PIN = {1: ROJO, 2: NEGRO, 3: VERDE, 4: AMAR}
y_borde = P(0, 1395)[1] + 6
x_cable = 1240
pts_fin = []
for k, (tx, c) in enumerate(zip(tornillos, colores)):
    x, y = P(tx, 1335)
    yv = y_borde + 70 + (3 - k) * 52  # giro escalonado: no se cruzan
    camino = [(x, y), (x, yv), (x_cable, yv)]
    d.line(camino, fill=(0, 0, 0), width=17, joint="curve")
    d.line(camino, fill=c, width=11, joint="curve")
    d.ellipse([x - 13, y - 13, x + 13, y + 13], fill=c, outline="black", width=3)
    pts_fin.append((x_cable, yv))
    # rotulo del pin (debajo de la foto, al lado del hilo)
for k, (tx, c) in enumerate(zip(tornillos, colores)):
    x, _ = P(tx, 1335)
y_tab = y_borde + 70 + 4 * 52 + 20

# tabla de pines bajo los hilos
tx0 = 430
d.text((tx0, y_tab), "Borne J3", font=fnt(26, True), fill=TXT)
d.text((tx0 + 230, y_tab), "Hilo del radar", font=fnt(26, True), fill=TXT)
d.text((tx0 + 480, y_tab), "Pin aviación", font=fnt(26, True), fill=TXT)
for k in range(4):
    yy = y_tab + 42 + k * 40
    d.rectangle([tx0 + 230, yy + 4, tx0 + 262, yy + 30], fill=colores[k], outline="black", width=2)
    d.text((tx0, yy), nombres[k], font=fnt(26), fill=TXT)
    d.text((tx0 + 275, yy), hilos[k], font=fnt(26), fill=TXT)
    d.text((tx0 + 480, yy), PIN_AV[k], font=fnt(26), fill=TXT)

# ---------------- cable y radar ----------------
ys = [p[1] for p in pts_fin]
ymid = (min(ys) + max(ys)) / 2
d.rounded_rectangle([x_cable, min(ys) - 22, x_cable + 70, max(ys) + 22], 14, fill=(45, 45, 45))
d.line([(x_cable + 70, ymid), (1470, ymid)], fill=(45, 45, 45), width=34)
d.text(((x_cable + 1470) / 2 + 35, ymid - 40), "cable 5 m", font=fnt(24), fill=TXT, anchor="mm")

# ficha aviacion: vista del lado de los contactos
cx, cy, r = 1560, ymid, 82
d.ellipse([cx - r - 12, cy - r - 12, cx + r + 12, cy + r + 12], fill=(200, 200, 205), outline="black", width=3)
d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(40, 40, 40))
pos = {1: (-1, -1), 2: (1, -1), 3: (1, 1), 4: (-1, 1)}
for n, (sx, sy) in pos.items():
    px, py = cx + sx * 36, cy + sy * 36
    d.ellipse([px - 19, py - 19, px + 19, py + 19], fill=COLOR_PIN[n], outline="white", width=3)
    d.text((px, py), str(n), font=fnt(22, True), fill=("black" if n in (3, 4) else "white"), anchor="mm")
d.text((cx, cy + r + 40), "Ficha aviación", font=fnt(24, True), fill=TXT, anchor="mm")
d.text((cx, cy + r + 68), "pines según el manual", font=fnt(22), fill=TXT, anchor="mm")
d.text((cx, cy + r + 94), "alinear los puntos rojos", font=fnt(22), fill=TXT, anchor="mm")

# foto del radar (panel con el conector)
rad = Image.open(f"{FOTOS}/conexion_radar.jpeg").crop((40, 700, 1090, 1450))
rad.thumbnail((400, 400), Image.LANCZOS)
rx, ry = 1680, int(ymid - rad.height / 2)
img.paste(rad, (rx, ry))
d.rectangle([rx, ry, rx + rad.width, ry + rad.height], outline=GRIS, width=3)
d.line([(cx + r + 12, cy), (rx + rad.width * 0.48, cy)], fill=(45, 45, 45), width=10)
d.text((rx + rad.width / 2, ry - 30), "Radar FM02B (9–24 V, ~140 mA)", font=fnt(24, True), fill=TXT, anchor="mm")

# nota inferior
d.text((W / 2, H - 40),
       "Cablear con todo desenchufado · J3-1 y J3-4 ya traen +12 V y GND desde J2 · si el radar no responde, invertir verde y amarillo",
       font=fnt(23), fill=TXT, anchor="mm")

img.save(SALIDA, quality=92)
print(img.size)
