#!/usr/bin/env python3
"""
Servidor HTTP de Telemetría para ESP32-C3 Super Mini (Biblioteca Estándar).

Módulo de integración backend desarrollado exclusivamente con la biblioteca
estándar de Python (sin dependencias externas). Implementa un despachador
multihilo para la ingestión de tramas telemétricas JSON provenientes del
microcontrolador SoC ESP32-C3 y el despacho de la aplicación web.

Protocolos implementados:
    - POST /api/eventos: Ingestión síncrona de telemetría y actualización de estado.
    - GET  /api/eventos: Retorno del estado actual y los 50 eventos más recientes.
    - GET  /           : Despacho de la interfaz web estática.

Ejecución:
    python3 api/server.py
"""

from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
import json
import os
import mimetypes
from datetime import datetime
from urllib.parse import urlparse

# Localización de las rutas base del proyecto
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WEB_DIR = os.path.join(BASE_DIR, "web")

# Estructuras de almacenamiento volátil en memoria
estado_dispositivo = {
    "led": False,
    "temperatura": 0.0,
    "ultima_ip": None,
    "ultimo_evento": "Ninguno",
    "ultima_actualizacion": None
}

historial_eventos = []


class APIRestHandler(SimpleHTTPRequestHandler):
    """
    Manejador de transacciones HTTP REST con soporte de CORS y despacho de archivos estáticos.
    """

    def end_headers(self):
        """Inyecta cabeceras de interoperabilidad Cross-Origin Resource Sharing (CORS)."""
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        super().end_headers()

    def do_OPTIONS(self):
        """Responde a las solicitudes de verificación preliminar (pre-flight) CORS."""
        self.send_response(204)
        self.end_headers()

    def do_GET(self):
        """Procesa peticiones HTTP GET para consulta de telemetría o entrega de recursos estáticos."""
        parsed = urlparse(self.path)
        path = parsed.path

        # Endpoint de telemetría: Estado actual y registro de eventos
        if path in ("/api/eventos", "/api/estado"):
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            data = {
                "estado": estado_dispositivo,
                "eventos": historial_eventos[-50:]
            }
            self.wfile.write(json.dumps(data, ensure_ascii=False).encode("utf-8"))
            return

        # Despacho de archivos estáticos de la interfaz web
        if path in ("/", ""):
            filepath = os.path.join(WEB_DIR, "index.html")
        else:
            rel_path = path.lstrip("/")
            filepath = os.path.join(WEB_DIR, rel_path)

        if os.path.exists(filepath) and os.path.isfile(filepath):
            self.send_response(200)
            mime, _ = mimetypes.guess_type(filepath)
            self.send_header("Content-Type", mime or "application/octet-stream")
            self.end_headers()
            with open(filepath, "rb") as f:
                self.wfile.write(f.read())
            return

        # Respuesta estándar ante recursos no encontrados
        self.send_response(404)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.end_headers()
        self.wfile.write(json.dumps({"error": "Recurso no localizado"}).encode("utf-8"))

    def do_POST(self):
        """Procesa peticiones HTTP POST para la ingestión de tramas JSON emitidas por la ESP32."""
        parsed = urlparse(self.path)
        path = parsed.path

        if path == "/api/eventos":
            content_length = int(self.headers.get("Content-Length", 0))
            post_body = self.rfile.read(content_length)

            try:
                evento_data = json.loads(post_body.decode("utf-8"))
            except Exception as e:
                self.send_response(400)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"error": "Carga útil JSON no válida", "detalle": str(e)}).encode("utf-8"))
                return

            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            evento_data["timestamp"] = timestamp

            # Actualización del modelo de estado según el evento recibido
            nombre_evento = evento_data.get("evento", "")
            if nombre_evento == "encender_led":
                estado_dispositivo["led"] = True
            elif nombre_evento == "apagar_led":
                estado_dispositivo["led"] = False
            elif nombre_evento == "temperatura":
                estado_dispositivo["temperatura"] = float(evento_data.get("valor", 0.0))

            estado_dispositivo["ultimo_evento"] = nombre_evento
            estado_dispositivo["ultima_ip"] = evento_data.get("ip")
            estado_dispositivo["ultima_actualizacion"] = timestamp

            historial_eventos.append(evento_data)

            print(f"[{timestamp}] [STD-HTTP INGEST] {evento_data.get('dispositivo')} | Evento: {nombre_evento} | Valor: {evento_data.get('valor')}")

            # Acuse de recibo HTTP 200
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"status": "ok", "recibido": True}).encode("utf-8"))
            return

        self.send_response(404)
        self.end_headers()


def iniciar_servidor(puerto: int = 8000, host: str = "0.0.0.0") -> None:
    """
    Inicializa el bucle de eventos del servidor HTTP en el zócalo especificado.
    
    Args:
        puerto (int): Puerto TCP de enlace (por defecto: 8000).
        host (str): Interfaz de red de enlace (0.0.0.0 para todas las interfaces).
    """
    servidor = ThreadingHTTPServer((host, puerto), APIRestHandler)
    print("=" * 65)
    print(" Servidor HTTP y API REST iniciado (Python Standard Library)")
    print(f"   -> Interfaz gráfica de usuario: http://localhost:{puerto}/")
    print(f"   -> Endpoint telemétrico:        http://localhost:{puerto}/api/eventos")
    print("   Presione Ctrl + C para finalizar la ejecución.")
    print("=" * 65)
    try:
        servidor.serve_forever()
    except KeyboardInterrupt:
        print("\n[INFO] Señal de terminación recibida. Cerrando sockets...")
        servidor.server_close()


if __name__ == "__main__":
    iniciar_servidor()
