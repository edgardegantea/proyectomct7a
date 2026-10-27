#!/usr/bin/env python3
"""
Servicio de Telemetría y Gestión de Eventos para ESP32-C3 Super Mini.

Módulo de integración backend desarrollado bajo el framework FastAPI. Proporciona
endpoints RESTful para la ingestión síncrona de eventos telemétricos generados por
el microcontrolador SoC ESP32-C3 y el despacho de estado a la interfaz de usuario.

Protocolos implementados:
    - HTTP POST /api/eventos: Ingestión de cargas útiles JSON estructuradas.
    - HTTP GET  /api/eventos: Recuperación del estado del actuador y registro histórico.
    - HTTP GET  /api/estado : Consulta atómica de variables de telemetría.

Ejecución directa:
    python3 api/app_fastapi.py

Ejecución mediante servidor ASGI de producción:
    uvicorn api.app_fastapi:app --host 0.0.0.0 --port 8000 --reload
"""

import os
from datetime import datetime
from typing import Optional, List, Dict, Any
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel, Field

# Localización de las rutas base del proyecto
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WEB_DIR = os.path.join(BASE_DIR, "web")

app = FastAPI(
    title="ESP32-C3 Super Mini Telemetry API",
    description="Servicio API REST para telemetría y control de periféricos en ESP32-C3",
    version="1.1.0"
)

# Configuración de políticas de intercambio de recursos de origen cruzado (CORS)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


class EventoESP32(BaseModel):
    """
    Esquema canónico de validación para las tramas de eventos telemétricos.
    
    Attributes:
        dispositivo (str): Identificador nominal del hardware emisor.
        evento (str): Código mnemónico del evento operativo.
        mensaje (Optional[str]): Detalle cualitativo o estado del actuador.
        valor (Optional[float]): Magnitud cuantitativa (temperatura en °C o estado binario).
        ip (Optional[str]): Dirección IPv4 asignada al cliente en la red de área local.
    """
    dispositivo: str = Field(default="ESP32-C3 Super Mini", description="Identificador del hardware emisor")
    evento: str = Field(..., description="Mnemónico de control (ej. encender_led, apagar_led, temperatura)")
    mensaje: Optional[str] = Field(default="", description="Descripción cualitativa del estado")
    valor: Optional[float] = Field(default=0.0, description="Magnitud numérica reportada")
    ip: Optional[str] = Field(default=None, description="Dirección IPv4 asignada a la interfaz Wi-Fi")


# Estructuras de almacenamiento volátil en memoria
estado_dispositivo: Dict[str, Any] = {
    "led": False,
    "temperatura": 0.0,
    "ultima_ip": None,
    "ultimo_evento": "Ninguno",
    "ultima_actualizacion": None
}

historial_eventos: List[Dict[str, Any]] = []


@app.post("/api/eventos", status_code=200, summary="Ingestión de eventos telemétricos")
async def recibir_evento(datos: EventoESP32):
    """
    Procesa las solicitudes HTTP POST emitidas por el firmware del ESP32-C3.
    
    Actualiza el estado interno de las variables de control y registra la entrada
    en el buffer circular del historial de eventos.
    
    Args:
        datos (EventoESP32): Instancia validada con la carga útil recibida.
        
    Returns:
        dict: Confirmación de recepción y acuse de recibo de la transacción.
    """
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    # Transición de estados según la semántica del evento recibido
    if datos.evento == "encender_led":
        estado_dispositivo["led"] = True
    elif datos.evento == "apagar_led":
        estado_dispositivo["led"] = False
    elif datos.evento == "temperatura":
        estado_dispositivo["temperatura"] = datos.valor

    estado_dispositivo["ultimo_evento"] = datos.evento
    estado_dispositivo["ultima_ip"] = datos.ip
    estado_dispositivo["ultima_actualizacion"] = timestamp

    # Inserción con marca de tiempo normalizada
    evento_dict = datos.model_dump()
    evento_dict["timestamp"] = timestamp
    historial_eventos.append(evento_dict)

    print(f"[{timestamp}] [FASTAPI INGEST] {datos.dispositivo} | Evento: {datos.evento} | Valor: {datos.valor}")

    return {
        "status": "ok",
        "recibido": True,
        "evento": datos.evento
    }


@app.get("/api/eventos", summary="Consulta de estado e historial de telemetría")
async def obtener_eventos():
    """
    Suministra el estado instantáneo del dispositivo y la ventana histórica más reciente.
    
    Returns:
        dict: Diccionario que agrupa el estado global y los últimos 50 registros cronológicos.
    """
    return {
        "estado": estado_dispositivo,
        "eventos": historial_eventos[-50:]
    }


@app.get("/api/estado", summary="Consulta atómica del estado del hardware")
async def obtener_estado():
    """
    Recupera exclusivamente las variables de estado actuales del periférico y telemetría.
    
    Returns:
        dict: Mapeo clave-valor con el estado actual del dispositivo.
    """
    return estado_dispositivo


# Mapeo de recursos estáticos correspondientes a la interfaz de usuario web
if os.path.exists(WEB_DIR):
    app.mount("/", StaticFiles(directory=WEB_DIR, html=True), name="web")

if __name__ == "__main__":
    import uvicorn
    print("=" * 65)
    print(" Subsistema API REST iniciado satisfactoriamente (FastAPI / Uvicorn)")
    print("   -> Especificación OpenAPI / Swagger UI: http://localhost:8000/docs")
    print("   -> Interfaz gráfica de usuario:         http://localhost:8000/")
    print("   -> Endpoint telemétrico:                http://localhost:8000/api/eventos")
    print("=" * 65)
    uvicorn.run("app_fastapi:app", host="0.0.0.0", port=8000, reload=True)
