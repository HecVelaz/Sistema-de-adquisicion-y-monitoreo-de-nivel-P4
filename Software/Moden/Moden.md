## Propósito
Aquí se guarda el software del moden utilizado , también 2 pdfs que pueden ayudar a la programación , se utilizó comdanos AT para la configuración del moden 

## Contenido
- [[USR-GM3-Setup-Software_V1.2.1.101.exe]]
- [[Gm3Cfg.ini]]
- [[Gm3Lang.txt]]
- ![[USR-GPRS232-730-Guide-Line_V1.0.13.01.pdf]]
- ![[USR-GPRS232-730&734-AT commands V1.0.0.pdf]]


## Configuración actual (movido desde `Software/CLAUDE.md`, 2026-11-21)

- Módem USR-GPRS232-730, firmware V3.2.2, 115200 8N1 (UART2 GPIO5/6, RTS GPIO7).
- Configurado por AT el 20/11/2026: modo `HTTPD` (`AT+WKMOD`), `POST`, URL `/level/insertarMasi.php` (antes `insertarFecha.php`), servidor = el de `credenciales.h` puerto 80, header `Content-Type: application/x-www-form-urlencoded[0D][0A]`, timeout 10 s, filtro de cabeceras `on` (al serial llega solo el cuerpo, `OK`), APN `internet.tigo.py`.
- Modo comando: `+++` → `a` → `a` → `+ok`; `AT+S` guarda y reinicia.
- `Gm3Cfg.ini` y `Gm3Lang.txt` no se suben (están en `.gitignore`): `Gm3Cfg.ini` guarda la IP del servidor.
