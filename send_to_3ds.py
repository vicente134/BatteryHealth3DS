#!/usr/bin/env python3
"""
send_to_3ds.py - Enviador Wi-Fi directo (NetLoader / 3dslink protocol) para BatteryHealth3DS
Creado por ElVichoPo (@vicente134)

Uso:
  python3 send_to_3ds.py <IP_DE_LA_3DS> [ruta_al_archivo.3dsx]

No requiere instalar devkitPro ni nada externo: usa solo Python estandar (socket + zlib)
y si no tienes el archivo .3dsx en la carpeta, lo descarga automaticamente del GitHub oficial.
"""

import os
import sys
import struct
import socket
import zlib
import urllib.request

PORT = 17491
CHUNK_SIZE = 16 * 1024
GITHUB_3DSX_URL = "https://github.com/vicente134/BatteryHealth3DS/releases/latest/download/BatteryHealth3DS.3dsx"

def ensure_3dsx_file(path):
    if os.path.isfile(path):
        return path
    if os.path.isfile("3ds-battery-health.3dsx"):
        return "3ds-battery-health.3dsx"
    print(f"[*] Descargando {path} desde el GitHub oficial de ElVichoPo...")
    urllib.request.urlretrieve(GITHUB_3DSX_URL, path)
    print(f"[+] Descarga completada ({os.path.getsize(path)} bytes).")
    return path

def send_3dsx(ip, filepath):
    filename = "BatteryHealth3DS.3dsx"
    name_bytes = filename.encode("utf-8") + b"\x00"
    filesize = os.path.getsize(filepath)

    print(f"[*] Conectando a la consola 3DS/2DS en {ip}:{PORT}...")
    with socket.create_connection((ip, PORT), timeout=10) as sock:
        # 1. Enviar longitud del nombre, nombre y tamaño del archivo (little-endian int32)
        sock.sendall(struct.pack("<I", len(name_bytes)))
        sock.sendall(name_bytes)
        sock.sendall(struct.pack("<I", filesize))

        # 2. Leer respuesta de la 3DS (int32 == 0 significa listo)
        resp = struct.unpack("<i", sock.recv(4))[0]
        if resp != 0:
            raise RuntimeError(f"El Homebrew Launcher rechazo la transferencia (codigo {resp})")

        # 3. Comprimir y enviar en bloques zlib igual que 3dslink oficial
        compressor = zlib.compressobj(level=6)
        sent_raw = 0
        with open(filepath, "rb") as f:
            while True:
                chunk = f.read(CHUNK_SIZE)
                if not chunk:
                    break
                comp = compressor.compress(chunk)
                if comp:
                    sock.sendall(struct.pack("<I", len(comp)))
                    sock.sendall(comp)
                sent_raw += len(chunk)

            tail = compressor.flush()
            if tail:
                sock.sendall(struct.pack("<I", len(tail)))
                sock.sendall(tail)

        # 4. Enviar longitud de argumentos de linea de comandos (0)
        cmd_arg = b"sdmc:/3ds/" + name_bytes
        sock.sendall(struct.pack("<I", len(cmd_arg)))
        sock.sendall(cmd_arg)

    print(f"[+] ¡BatteryHealth3DS enviado con exito a {ip}! Ya se guardo en tu SD y se esta abriendo.")

def main():
    print("==========================================================")
    print("  Enviador Wi-Fi 3dslink - BatteryHealth3DS (ElVichoPo)   ")
    print("==========================================================")
    if len(sys.argv) >= 2:
        ip = sys.argv[1].strip()
    else:
        ip = input("Ingresa la IP que muestra tu 3DS/2DS al presionar [Y] en Homebrew Launcher: ").strip()

    if not ip:
        print("[!] Debes ingresar una direccion IP valida (ej: 192.168.1.40).")
        sys.exit(1)

    target_file = sys.argv[2] if len(sys.argv) >= 3 else "BatteryHealth3DS.3dsx"
    filepath = ensure_3dsx_file(target_file)
    send_3dsx(ip, filepath)

if __name__ == "__main__":
    main()
