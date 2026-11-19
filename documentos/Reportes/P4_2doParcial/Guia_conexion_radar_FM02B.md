# Guía de conexión e instalación del radar FM02B (P4)

Guía práctica para armar la estación con el radar Luda/POLIVIR FM02B y la PCB V3, **sin módem DTU**: los datos salen por el **WiFi del ESP32-S3**.

Fuentes: los tres PDFs de esta carpeta (ficha del LDFM02_B, protocolo Modbus V3 e instrucciones de montaje), el PCB `Hardware/V3_Estacion_Nivel/Estacion_Nivel_V3.kicad_pcb` y las fotos de `Fotos/`.

> **Ojo:** el PDF `Hardware/V3_Estacion_Nivel/planos/Estacion_Nivel_V3.pdf` y el BOM `Estacion_Nivel_V3.csv` están **desactualizados**: muestran borneras J4/J5 de 2 polos que la placa fabricada no tiene. Esta guía sigue el `.kicad_pcb` y el esquemático fuente (`vcc_to_3v3.kicad_sch`), que coinciden con la placa real.

> Los PDFs llaman al equipo LDFM01, LDFM02_B o LDFM04_B según el documento. Para la conexión no cambia nada: los 4 hilos y el protocolo son los mismos.

---

## 1. Qué se conecta con qué

![Conexión del radar a la PCB V3 sobre la foto de la placa.](diagrama_conexion_radar.jpg)

### Conectores de la placa (vista superior, como en la foto)

| Conector | Dónde está | Qué es | Uso con el radar y WiFi |
| -------- | ---------------- | ------------------------------ | ----------------------------- |
| **J2** | Jack DC arriba a la izquierda | Entrada **12 V** | Salida 12 V del UPS |
| **J9** | USB-C a la derecha | Entrada **5 V** (pasa por el diodo D8) | Salida USB del UPS (y programación) |
| **J4** | Jack DC abajo a la derecha | Entrada **5 V** directa (sin diodo) | Alternativa a J9: usar **uno solo** |
| **J3** | Bornera de 4, la de la **derecha** | Canal RS485 #1 (U1 → UART1) + 12 V | **Radar** |
| **J8** | Bornera de 4, la de la izquierda | Canal RS485 #2 (U5 → UART2) + 12 V | **No usar** (era el módem) |

Las borneras se numeran **de izquierda a derecha** (borne 1 a la izquierda) mirando la placa como en la foto:

| Borne | J3 (radar) | J8 (módem, sin uso) |
| --- | --- | --- |
| 1 | +12 V | +12 V |
| 2 | A1 | A2 |
| 3 | B1 | B2 |
| 4 | GND | GND |

### Tabla de cableado del radar

| Hilo del radar | Pin aviación | Señal | Borne de la PCB | Notas |
| -------- | ------- | ---------- | -------------- | ----------------------------- |
| Rojo | 1 | Vcc 9–24 V | **J3-1** | +12 V que entran por J2 |
| Verde | 3 | 485A | **J3-2 (A1)** | Va al pin A del SP3485 (U1) |
| Amarillo | 4 | 485B | **J3-3 (B1)** | Va al pin B del SP3485 (U1) |
| Negro | 2 | GND | **J3-4** | Une la masa del radar con la de la PCB: **obligatorio** para el RS485 |

Lo que ya trae la PCB en el canal de J3 (no hay que agregar nada):

- Transceptor **SP3485EN a 3,3 V** (U1) → UART1 del ESP32: TX = GPIO1, RX = GPIO2, DE/RE = GPIO4.
- Resistencia de terminación de **120 Ω** (R3) entre A1 y B1.
- Polarización: 4,7 kΩ de A1 a 3,3 V (R1) y 4,7 kΩ de B1 a GND (R2).
- Protección TVS **SM712** (D1) en A1/B1.

**J8 queda libre.** No hace falta desoldar nada; simplemente no se conecta.

### Alimentación

- Radar: 9–24 V, típico **12 V a 130–160 mA** (~1,9 W). Los manuales piden que en los bornes del radar haya **al menos 7 V**. Con los 5 m de cable que trae no hay problema, pero si se alarga el cable hay que medirlo.
- Los **12 V entran por el jack J2** y llegan directo a J3-1 (y J8-1). **No alimentan al ESP32.**
- El ESP32 se alimenta con **5 V por J9 (USB-C) o por J4**, que van al AMS1117 → 3,3 V. Por eso hay que conectar **las dos** salidas del UPS: 12 V a J2 y 5 V a J9 o J4.
- **Polaridad de J2:** antes de enchufar, medir que el **centro del plug** del UPS sea el **+**. Con la placa sin alimentar, el pin central del jack J2 tiene que dar continuidad con J3-1.
- El WiFi del ESP32 tiene picos de ~350–500 mA en 3,3 V: usar una salida USB del UPS de al menos 1 A y un cable corto y bueno.

---

## 2. Antes de armar: probar el radar solo (recomendado)

Así se descartan problemas del sensor antes de sumar la PCB.

### Opción A: Bluetooth (sin cables de datos)

1. Alimentar el radar con 12 V (rojo +, negro −).
2. En WeChat, buscar el mini-programa **"JET Radar Debugger"** → `BR2262e-s` → **Flowmeter** → *Device Information* / *Parameter Configuration*.
3. Comprobar que muestra distancia y anotar la **dirección (debe ser 1)** y el **baud rate (debe ser 115200)**.

> **Hecho (19/11/2026):** con el Bluetooth se leyó **dirección 1** y **115200 baudios**. Coincide con la configuración del firmware, así que no hay que cambiar nada. La opción B ya no hace falta salvo para diagnosticar fallas.

#### Configuración leída con la app (19/11/2026)

Capturas en `Fotos/Aplicacion/`. Equipo: *flowmeter*, n.º de serie `1a91000400000019`, versión V25.

| Grupo | Parámetro | Valor | Comentario |
| ---------- | ---------------------- | ------------ | ----------------------------------------- |
| Básico | Dirección / baud | 1 / 115200 | Coincide con el firmware. **No tocar.** |
| Nivel | Rango (*Distance*) | 40 m | El máximo. Bien. |
| Nivel | Altura de instalación | 500 cm | Solo sirve para el "nivel" que calcula el radar (5,00 − d). El firmware usa `d`, así que no influye. |
| Nivel | **Calibración** | **−46 mm** | Corrimiento que el radar resta a todas las distancias. No cambiarlo a ciegas: ver abajo. |
| Nivel | Sensibilidad | 5 | El manual dice 3 de fábrica (menor = más sensible). |
| Nivel | Potencia de emisión | 7 | **La mínima** (0 = máxima). |
| Velocidad | Filtro de sentido, algoritmo espectral, límite | bidireccional, apagado, 5 m/s | El caudal está fuera de alcance: dejarlo así. |
| Lectura | Distancia (*Empty height*) | 0,849 m | Nivel = 5,000 − 0,849 = 4,151 m (coincide). |
| Lectura | **Señal de nivel** | **4** | Muy baja: el manual pide más de 30 dB. |
| Lectura | Ángulo | 56,1° | Cercano a los 55° nominales del haz de velocidad. |

Qué hacer con estos valores:

1. **Señal de nivel = 4.** Una lectura con tan poca señal puede no ser confiable. Primero hay que ver cuánto da apuntando al agua del balde, a ~1 m, sin objetos cerca (la captura puede haber sido apuntando a la mesa o al piso). Si sigue baja, subir la **potencia de emisión de nivel** (7 → 3, o hasta 0) y volver a mirar la señal. La señal también aparece en el monitor serie del firmware.
2. **Calibración −46 mm.** No se sabe si viene de fábrica o la cambió alguien. En el banco, comparar la distancia del radar con la cinta métrica a 3–4 alturas de agua:
   - si la diferencia es **constante**, corregirla en este parámetro y anotar el valor anterior y el nuevo;
   - si la diferencia **varía** con la altura, no es un problema de corrimiento: revisar la nivelación y el haz.
3. **Ángulo 56,1°.** Según la figura del manual, con el radar nivelado el haz de velocidad queda a 55°. Es probable que esta lectura sirva para verificar la nivelación (≈ 55° = nivelado), pero hay que **confirmarlo una vez** con el nivel de burbuja antes de usarlo.
4. **Altura de instalación.** Opcional: ponerla igual a la altura real del banco (en cm), así la app muestra el nivel del agua directamente y se puede comparar con la cinta.

### Opción B: adaptador USB–RS485 en la PC

Conectar verde → A, amarillo → B, negro → GND del adaptador (y 12 V aparte). Configuración: **115200 baudios, 8N1, sin paridad, dirección 1**.

Script mínimo (Linux, `pip install pyserial`); las tramas son las del manual y tienen el CRC verificado:

```python
import serial, time
s = serial.Serial("/dev/ttyUSB0", 115200, timeout=0.5)
def q(hexstr):
    s.reset_input_buffer(); s.write(bytes.fromhex(hexstr)); time.sleep(0.1)
    r = s.read(64); print(hexstr, "->", r.hex(" ").upper()); return r
r = q("01 03 00 01 00 01 D5 CA")             # distancia, mm
if len(r) >= 5: print("d =", int.from_bytes(r[3:5], "big"), "mm")
r = q("01 03 00 04 00 01 C5 CB")             # intensidad de señal, dB (>30 = buena)
if len(r) >= 5: print("señal =", int.from_bytes(r[3:5], "big"), "dB")
```

Si no responde nada: invertir verde y amarillo (no rompe nada) y revisar que el GND esté unido.

---

## 3. Armado paso a paso (con todo desenchufado)

1. **Desconectar el UPS.** Todo el cableado se hace sin tensión.
2. **Radar a J3** (la bornera de 4 de la derecha), de izquierda a derecha:
   - J3-1 ← **rojo**, J3-2 ← **verde**, J3-3 ← **amarillo**, J3-4 ← **negro**.
   - Usar punteras (terminales tubulares) en los hilos y ajustar bien los tornillos de la bornera de 5 mm.
   - La bornera es enchufable: se puede sacar de la placa para cablearla más cómodo y después volver a enchufarla.
3. **Conector de aviación** (radar ↔ cable): alinear los **puntos rojos** de las dos partes, sostener por la zona "1" del dibujo del manual y empujar hasta oír el **"clic"**. Para sacarlo, tirar sujetando por la zona "2" (no tirar del cable).
4. **Revisión con el multímetro, todavía sin tensión:**
   - no hay continuidad entre J3-1 (+12 V) y J3-4 (GND);
   - el pin central de J2 da continuidad con J3-1, y la carcasa de J2 con J3-4;
   - entre J3-2 y J3-3 se miden ~120 Ω (la terminación de la placa; con el radar conectado puede dar algo menos).
5. **12 V a la placa:** enchufar el plug de 12 V del UPS en **J2**, con la polaridad ya verificada (+ al centro).
6. **5 V a la placa:** cable USB del UPS a **J9 (USB-C)**. Si se usa J4 en su lugar, no conectar los dos al mismo tiempo.
7. **Encender** y medir **≥ 7 V entre rojo y negro del lado del radar** (debería dar ~12 V).
8. Abrir el monitor serie (`pio device monitor -b 115200`) y comprobar que el firmware lee la distancia y la señal (ver la sección 6).

---

## 4. Montaje mecánico (banco de laboratorio)

### Kit que trae el radar

- **Soporte "pico de pato"** (duckbill), orientable, en acero inoxidable.
- **Placa adaptadora** de 110 × 93 mm con ranuras para abrazaderas y agujeros de Ø 7 mm.
- **3 abrazaderas** inoxidables, para fijarlo a un poste o caño.
- El cuerpo del radar mide ~159,5 × 95 × 99 mm. La **cara de arriba es plana** y tiene los agujeros de fijación al soporte. La **antena de nivel (80 GHz) mira hacia abajo**. La **cara inclinada a 35°** es la de la antena de velocidad (24 GHz).

### Reglas del manual que aplican al banco

1. **Antena de nivel perpendicular al agua (90°).** El manual marca como **incorrecto** montarlo inclinado. Para nivelarlo, apoyar un **nivel de burbuja sobre la cara superior plana** y comprobarlo en los dos ejes; después, ajustar el soporte orientable.
   - Ayuda extra: los registros `0x0009` (roll) y `0x000A` (pitch), en 0,1°, salen del giróscopo interno. Con el radar ya nivelado con burbuja, leerlos y **anotarlos como referencia**: sirven para comprobar más tarde que no se movió.
2. **Huella del haz de nivel** (elipse sobre el agua, con H = altura de la antena en metros):
   - ancho C ≈ **0,25·H** (haz de 14°) × D ≈ **0,17·H** (haz de 10°).
   - Para que el haz **no toque las paredes del balde**, C tiene que entrar dentro del diámetro. Con un balde de 30 cm: 0,25·H ≤ 0,30 → **H ≤ 1,2 m**, así que conviene quedarse en ~1 m.
   - Rango mínimo: **0,1 m**. No acercar el agua a menos de 10 cm de la antena.
3. **Haz de velocidad:** sale hacia adelante, a 55° de la horizontal, e incide a L ≈ 0,7·H. En el banco no hay corriente, así que la velocidad va a dar ~0 o ruido. Eso es normal y no afecta la medición de nivel.
4. **Cable:** dejar un **bucle en "U"** por debajo del conector (gotero), para que el agua no escurra hacia la ficha. El manual recomienda **caño corrugado** para el cable. No dejar el peso del cable colgando del conector.

### Pruebas en interior (avisos del manual)

Los tubos fluorescentes, los ventiladores, la salida del aire acondicionado, las fuentes de potencia y los objetos metálicos cerca del haz generan **lecturas espurias** por reflexión múltiple, sobre todo en la velocidad. Para el banco:

- sacar objetos metálicos del cono de la antena (soportes, reglas, mesas);
- si aparecen saltos raros, apagar el ventilador o el aire, o hacer la prueba junto a una ventana abierta;
- controlar la **intensidad de señal** (registro `0x0004`): por encima de **30 dB** es buena.

---

## 5. Registros Modbus útiles (resumen del protocolo V3)

Formato: **115200 8N1, dirección 1, función 03 (leer) / 06 (escribir 1 registro), CRC16 Modbus (polinomio A001)**. Los enteros vienen en big-endian. Los `float` de 4 bytes vienen en **little-endian (DCBA)**: hay que invertir el orden de los bytes antes de convertir.

| Registro | Qué es | Trama de lectura | Formato |
| ----------- | ------------------- | ------------------------------- | ------------------- |
| `0x0001` | **Distancia radar → agua** (filtrada) | `01 03 00 01 00 01 D5 CA` | uint16, **mm** |
| `0x0003` | Nivel de agua = alt. instalación − distancia | `01 03 00 03 00 01 74 0A` | uint16, mm |
| `0x0004` | Intensidad de señal (nivel) | `01 03 00 04 00 01 C5 CB` | dB, > 30 = buena |
| `0x0005` | Altura de instalación (R/W) | `01 03 00 05 00 01 94 0B` | uint16, **cm** (fábrica: 10 000 cm) |
| `0x0009` / `0x000A` | Roll / pitch | `01 03 00 09 00 01 54 08` / `01 03 00 0A 00 01 A4 08` | int16, 0,1° |
| `0x01F4–F5` | Velocidad filtrada | `01 03 01 F4 00 02 84 05` | int32 BE con signo, **× 0,001 m/s** |
| `0x000F–10` | Velocidad superficial | `01 03 00 0F 00 02 F4 08` | float LE, m/s |
| `0x006E` | Dirección del equipo | `01 03 00 6E 00 01 E5 D7` | fábrica 1 |
| `0x006F` | Baud rate / 100 | `01 03 00 6F 00 01 B4 17` | en nuestro radar: 115200 |

Notas:

- **Para el proyecto alcanza con `0x0001`** (la distancia `d`). El firmware envía `d` crudo: `Nivel = H − d` no se calcula en la placa. Los registros `0x0003`/`0x0005` son opcionales.
- Si alguna vez se quiere usar `0x0003`: primero escribir la altura de instalación en cm. Ejemplo para 100 cm: `01 06 00 05 00 64 98 20`.
- La tabla del manual dice que el baud de fábrica es `0x60` (9600), pero **nuestro radar está a 115200** (confirmado por Bluetooth el 19/11/2026), igual que dice la ficha.
- La respuesta a una lectura de 1 registro tiene 7 bytes: `01 03 02 HH LL CRC CRC`.
- **No escribir** registros de flujo, sección o algoritmo (`0x0061–0x0066`, `0x07E2`…): no hacen falta para medir nivel.

---

## 6. Firmware adaptado (`Software/Firmware_Final`)

El firmware ya está adaptado al radar y al WiFi. **Compila, pero todavía no se probó en la placa.**

### Sensor (`components/sensor_rs485/sensor_rs485.c`)

| Antes (ultrasónico P3) | Ahora (radar FM02B) |
| --- | --- |
| 9600 baudios | **115200** (`SENSOR_BAUD_RATE`) |
| Registro 0x0000 | **Registro 0x0001** (distancia, mm → m) |
| DE/RE (GPIO4) manejado a mano con retardos | DE/RE manejado **por hardware** (modo RS485 half-duplex del UART, igual que el canal del módem en P3) |
| Sin verificar el CRC | **Se verifica el CRC**, la dirección, la función y la cantidad de bytes; detecta excepciones Modbus |
| — | Descarta lecturas < 100 mm (fuera de rango) y muestra la **señal en dB** en el monitor serie |

Los pines **no cambian** (GPIO1/2/4, UART1, bornera J3). La dirección (1) y el baud (115200) se confirmaron por Bluetooth.

### Envío: WiFi en lugar del módem (`components/wifi_envio/`)

- WiFi en modo estación con **reconexión automática**. Si al arrancar no hay red, espera 15 s y sigue: mide y guarda en la SD igual.
- Cada dato se manda por **HTTP** al mismo script PHP que usaba el módem (`insertarMasi.php`), con la misma trama `d=<epoch>,<valor>` (ejemplo: `d=3242432,2.2`). Éxito = respuesta HTTP 200 que contiene `OK`. El *Store and Forward* (`temp.csv` + tarea de pendientes) no cambió.
- No tenemos el código del PHP, así que no sabemos si lee `$_GET` o `$_POST`. Por eso el firmware hace un **POST a `...script.php?d=<epoch>,<valor>` con el mismo `d=...` en el cuerpo**: funciona en los dos casos (y con `$_REQUEST`), y es una sola petición, así que inserta una sola vez.
- `modem_dtu` queda en el código pero ya no se usa; la bornera J8 y la UART2 (GPIO5/6/7) quedan libres.
- Partición de la app ampliada a 1,5 MB (`platformio.ini`): con WiFi no entraba en 1 MB.

### Lo que hay que completar antes de cargar

Editar **`Software/Firmware_Final/components/wifi_envio/credenciales.h`**. Este archivo **no se sube al repo** (está en `.gitignore`); la plantilla es `credenciales_ejemplo.h`.

```c
#define WIFI_SSID          "nombre_de_la_red"
#define WIFI_CLAVE         "clave_de_la_red"
#define SERVIDOR_URL       "http://<IP del servidor>/level/insertarMasi.php"   // sin "?d=..." 
```

- En la copia local de `credenciales.h` **la URL ya está cargada**. Faltan solo el SSID y la clave del WiFi.
- **La IP del servidor no se escribe en el repo ni en esta guía.** Es un script que inserta en la base de datos sin contraseña, y el repo es público. Se pasa por un canal privado del equipo.
- Después: `pio run -t upload` y `pio device monitor -b 115200`. En el monitor tienen que verse `Conectado. IP ... | RSSI ... dBm`, `Distancia: x.xxx m | senal: NN dB` y `HTTP 200 | respuesta: ...OK`.

### Cosas del WiFi a tener en cuenta en el lugar

- El ESP32-S3 solo usa **2,4 GHz**.
- Las redes institucionales con **portal cautivo** (página de login) no funcionan con el ESP32. Hace falta una red con WPA2 normal; para el banco, alcanza con el hotspot de un celular.
- El módulo WROOM-1 tiene **antena impresa**: una **caja metálica la bloquea**. Mirar el RSSI que muestra el monitor serie (mejor que −70 dBm) en el lugar definitivo.

---

## 7. Lista de verificación rápida

- [ ] Radar probado solo (Bluetooth o USB–RS485): responde en la dirección 1; baud confirmado.
- [ ] UPS desconectado durante el cableado.
- [ ] J3 (bornera derecha), de izquierda a derecha: rojo, verde, amarillo, negro. J8 sin conectar.
- [ ] 12 V del UPS → jack J2 (+ al centro, verificado); 5 V → USB-C J9 (o J4, nunca los dos).
- [ ] Sin corto entre J3-1 y J3-4; entre J3-2 y J3-3 hay ~120 Ω.
- [ ] Conector de aviación con los puntos rojos alineados y el "clic".
- [ ] ≥ 7 V en los bornes del radar.
- [ ] Antena nivelada con burbuja (los dos ejes); roll/pitch anotados.
- [ ] Haz dentro del balde (H ≤ ~1,2 m con balde de 30 cm; agua a más de 10 cm).
- [ ] Sin metal ni ventiladores en el cono; señal `0x0004` > 30 dB.
- [ ] Cable con bucle en "U" y sin peso sobre el conector.
- [ ] `credenciales.h` completado (SSID y clave; la URL ya está) y **no subido** al repo.
- [ ] Firmware cargado; el monitor muestra la distancia, la señal y `HTTP 200 ... OK`.

## 8. Si algo falla

| Síntoma | Causa probable | Qué hacer |
| ------------------ | ------------------- | ----------------------- |
| No hay ninguna respuesta Modbus | A/B invertidos, GND sin unir, baud mal | Invertir verde/amarillo; revisar negro → J3-4 |
| Responde con CRC malo o basura | Ruido o mal contacto | Revisar las borneras y el GND; acortar el cable |
| Distancia que salta o es absurda | Haz tocando la pared del balde, metal o ventilador cerca, antena inclinada | Bajar H, centrar, nivelar, revisar la señal `0x0004` |
| El radar se reinicia o no prende | Tensión < 7 V en sus bornes | Medir en el lado del radar; revisar la salida de 12 V del UPS |
| La placa se reinicia al transmitir por WiFi | USB flojo o débil | Cable USB-C corto; salida ≥ 1 A |
| No conecta al WiFi | Portal cautivo, 5 GHz o caja metálica | Usar una red WPA2 de 2,4 GHz; mejorar la ubicación de la antena |
| `HTTP 404/405` o sin `OK` | URL mal escrita, o el servidor no responde `OK` | Revisar `SERVIDOR_URL`; ver en el monitor la respuesta que devuelve el servidor |
