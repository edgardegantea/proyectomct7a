#!/usr/bin/env python3
"""
Servicio WSGI de Telemetría y Gestión de Eventos para ESP32-C3 Super Mini.

Módulo de integración backend desarrollado bajo el microframework Flask. Diseñado
para entornos de ejecución compatibles con la especificación WSGI, tales como
PythonAnywhere, servidores Apache con mod_wsgi o contenedores independientes.

Protocolos implementados:
    - POST /api/eventos: Ingestión de cargas útiles JSON serializadas.
    - GET  /api/eventos: Consulta agregada del estado y registro de eventos.
    - GET  /api/estado : Consulta unificada de variables de telemetría.

Ejecución directa:
    python3 api/app_flask.py
"""

import os
from datetime import datetime
from flask import Flask, request, jsonify, send_from_directory

# Localización de las rutas base del proyecto
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WEB_DIR = os.path.join(BASE_DIR, "web")

app = Flask(__name__, static_folder=WEB_DIR)

# Configuración de cabeceras de control de acceso HTTP (CORS)
try:
    from flask_cors import CORS
    CORS(app)
except ImportError:
    @app.after_request
    def habilitar_cors(response):
        """Añade cabeceras estándar para permitir interoperabilidad entre orígenes cruzados."""
        response.headers["Access-Control-Allow-Origin"] = "*"
        response.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS"
        response.headers["Access-Control-Allow-Headers"] = "Content-Type"
        return response

# Almacenamiento en memoria volátil de estado y eventos
estado_dispositivo = {
    "led": False,
    "temperatura": 0.0,
    "ultima_ip": None,
    "ultimo_evento": "Ninguno",
    "ultima_actualizacion": None
}

historial_eventos = []


# =========================================================================
# RUTAS DE LA API REST
# =========================================================================

@app.route("/api/eventos", methods=["POST"])
def recibir_evento():
    """
    Endpoint para la ingestión de tramas JSON generadas por el ESP32-C3.
    
    Returns:
        Response: Objeto JSON con acuse de recibo y código HTTP 200 (éxito) o 400 (error sintáctico).
    """
    datos = request.get_json(silent=True)
    if not datos:
        return jsonify({"error": "Carga útil JSON nula o con errores de sintaxis"}), 400

    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    evento = datos.get("evento", "")

    # Actualización del modelo de estado según el evento procesado
    if evento == "encender_led":
        estado_dispositivo["led"] = True
    elif evento == "apagar_led":
        estado_dispositivo["led"] = False
    elif evento == "temperatura":
        estado_dispositivo["temperatura"] = float(datos.get("valor", 0.0))

    estado_dispositivo["ultimo_evento"] = evento
    estado_dispositivo["ultima_ip"] = datos.get("ip")
    estado_dispositivo["ultima_actualizacion"] = timestamp

    # Registro en el histórico de eventos telemétricos
    evento_registro = {
        "dispositivo": datos.get("dispositivo", "ESP32-C3 Super Mini"),
        "evento": evento,
        "mensaje": datos.get("mensaje", ""),
        "valor": datos.get("valor", 0.0),
        "ip": datos.get("ip"),
        "timestamp": timestamp
    }
    historial_eventos.append(evento_registro)

    print(f"[{timestamp}] [FLASK INGEST] {evento_registro['dispositivo']} | Evento: {evento} | Valor: {evento_registro['valor']}")

    return jsonify({"status": "ok", "recibido": True, "evento": evento}), 200


@app.route("/api/eventos", methods=["GET"])
def obtener_eventos():
    """
    Recupera el estado actual del dispositivo y los 50 eventos más recientes.
    
    Returns:
        Response: Objeto JSON serializado con el estado y la lista cronológica de eventos.
    """
    return jsonify({
        "estado": estado_dispositivo,
        "eventos": historial_eventos[-50:]
    }), 200


@app.route("/api/estado", methods=["GET"])
def obtener_estado():
    """
    Recupera de forma atómica únicamente las variables de estado operativas.
    
    Returns:
        Response: Objeto JSON con las variables de telemetría y estado del hardware.
    """
    return jsonify(estado_dispositivo), 200


# =========================================================================
# ENRUTAMIENTO DE RECURSOS ESTÁTICOS
# =========================================================================

@app.route("/")
def index():
    """Despacha la página principal del panel de control web."""
    return send_from_directory(WEB_DIR, "index.html")


@app.route("/<path:archivo>")
def archivos_estaticos(archivo):
    """Despacha recursos estáticos auxiliares (hojas de estilo CSS, scripts JavaScript)."""
    return send_from_directory(WEB_DIR, archivo)


if __name__ == "__main__":
    puerto = 8000
    print("=" * 65)
    print(" Subsistema API REST iniciado satisfactoriamente (Flask WSGI)")
    print(f"   -> Interfaz gráfica de usuario: http://localhost:{puerto}/")
    print(f"   -> Endpoint telemétrico:        http://localhost:{puerto}/api/eventos")
    print("=" * 65)
    app.run(host="0.0.0.0", port=puerto, debug=True)
