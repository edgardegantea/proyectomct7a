# Proyecto ESP32-C3 Super Mini - POO & Telemetría Web

Proyecto de comunicación y control orientado a objetos para la tarjeta **ESP32-C3 Super Mini**, integrando conexión Wi-Fi, envío de eventos a un servidor API y visualización en tiempo real en una interfaz web.

---

## 📁 Estructura del Proyecto

```text
proyecto/
├── api/
│   ├── app_fastapi.py         # Versión API con FastAPI + Uvicorn + Pydantic
│   ├── app_flask.py           # Versión API con Flask + Flask-CORS (ideal para PythonAnywhere)
│   ├── index.php              # Versión API en PHP con persistencia JSON (para VPS / cPanel / Apache)
│   ├── pythonanywhere_wsgi.py # Plantilla de configuración WSGI para PythonAnywhere
│   ├── server.py              # Versión API nativa (Python estándar, sin dependencias)
│   └── requirements.txt       # Dependencias de FastAPI y Flask
├── prototipo/
│   ├── arduino/
│   │   └── esp32_c3_supermini/
│   │       ├── esp32_c3_supermini.ino  # Sketch principal para Arduino IDE
│   │       ├── ESP32C3Dispositivo.h    # Clase controladora POO
│   │       └── ESP32C3Dispositivo.cpp  # Implementación POO
│   └── platformio/
│       ├── platformio.ini              # Configuración de compilación y flags USB CDC
│       ├── include/
│       │   └── ESP32C3Dispositivo.h    # Header POO
│       └── src/
│           ├── ESP32C3Dispositivo.cpp  # Implementación POO
│           └── main.cpp                # Punto de entrada C++ para PlatformIO
├── web/
│   ├── index.html             # Panel de telemetría y eventos (universal)
│   ├── index.php              # Entrada para servidores PHP
│   ├── style.css              # Estilos del panel (modo oscuro moderno)
│   └── app.js                 # Lógica de sincronización y selector dinámico de API
└── modelo-ia/                 # Reservado para futuras fases con IA
```

---

## 🛠️ 1. Módulo ESP32-C3 (`prototipo/`)

### Características de la Clase `ESP32C3Dispositivo`
La clase encapsula el control del hardware bajo el paradigma de **Programación Orientada a Objetos**:
- **Verificación previa de Wi-Fi:** Cada método de acción valida primero que exista conexión Wi-Fi activa antes de proceder (`asegurarConexionWiFi()`), reintentando la conexión si fuese necesario.
- **`encenderLed()`**: Enciende el LED azul integrado en GPIO 8 y notifica a la API (`encender_led`).
- **`apagarLed()`**: Apaga el LED y notifica a la API (`apagar_led`).
- **`secuencia1()`**: Ejecuta un patrón de parpadeo rápido (5 ciclos).
- **`secuencia2()`**: Ejecuta un patrón estroboscópico de doble destello (3 ciclos).
- **`temperatura()`**: Lee la temperatura interna del silicio del chip ESP32-C3 (`temperatureRead()`) y envía la medición en °C a la API.

> [!NOTE]
> En la tarjeta **ESP32-C3 Super Mini**, el LED azul está conectado en **GPIO 8** y funciona en lógica de nivel bajo (**Active LOW**: `LOW` = Encendido, `HIGH` = Apagado).

### Versión Arduino IDE (`prototipo/arduino/`)
1. Abre la carpeta [esp32_c3_supermini/](file:///media/edegantea/F0D6412CD640F47E/development/MECATRONICA/7A/ProgramacionAvanzada/proyecto/prototipo/arduino/esp32_c3_supermini/) y el archivo [esp32_c3_supermini.ino](file:///media/edegantea/F0D6412CD640F47E/development/MECATRONICA/7A/ProgramacionAvanzada/proyecto/prototipo/arduino/esp32_c3_supermini/esp32_c3_supermini.ino).
2. Selecciona la placa **ESP32C3 Dev Module**.
3. Activa **Tools > USB CDC On Boot: Enabled** (indispensable para ver la salida Serial por USB).

### Versión PlatformIO (`prototipo/platformio/`)
1. Abre la carpeta [platformio/](file:///media/edegantea/F0D6412CD640F47E/development/MECATRONICA/7A/ProgramacionAvanzada/proyecto/prototipo/platformio/) en VS Code con la extensión de PlatformIO.
2. El archivo [platformio.ini](file:///media/edegantea/F0D6412CD640F47E/development/MECATRONICA/7A/ProgramacionAvanzada/proyecto/prototipo/platformio/platformio.ini) ya incluye las banderas críticas de compilación:
   ```ini
   build_flags = 
       -D ARDUINO_USB_MODE=1
       -D ARDUINO_USB_CDC_ON_BOOT=1
   ```
3. Compila (`Ctrl+Alt+B` o `pio run`) y sube (`Ctrl+Alt+U` o `pio run -t upload`).

---

## 🚀 2. Versiones del Servidor API (`api/`)

Dispones de tres alternativas para la API según tu preferencia de framework:

### Instalación de dependencias (FastAPI / Flask)
```bash
python3 -m venv venv
source venv/bin/activate
pip install -r api/requirements.txt
```

### Opción A: FastAPI (`app_fastapi.py`)
- Validación de datos automática con **Pydantic**.
- Generación de documentación interactiva Swagger en `/docs`.
- Ejecución:
  ```bash
  python3 api/app_fastapi.py
  # O mediante uvicorn directamente:
  uvicorn api.app_fastapi:app --host 0.0.0.0 --port 8000 --reload
  ```

### Opción B: Flask (`app_flask.py`)
- Estructura tradicional y ligera con **Flask**.
- Soporte de CORS y rutas REST sencillas.
- Ejecución:
  ```bash
  python3 api/app_flask.py
  ```

### Opción C: PHP para VPS / Apache / Nginx (`index.php`)
- Diseñado para funcionar en cualquier servidor web tradicional con soporte de PHP.
- Persiste los eventos en un archivo local `datos.json`.
- No requiere instalar Python ni demonios de sistema.

### Opción D: Python Nativo (`server.py`)
- No requiere instalar ningún paquete de `pip`.
- Ejecución:
  ```bash
  python3 api/server.py
  ```

---

## 🌐 3. Despliegue en la Web (`web/`)

La interfaz web ha sido adaptada para funcionar de manera agnóstica en múltiples entornos gracias a su selector de endpoint configurable (botón **⚙️ Configurar API** o automático).

### A. Despliegue en VPS con PHP (Apache / Nginx / cPanel)
1. Sube las carpetas `web/` y `api/` a la raíz de tu servidor web (por ejemplo, en `/var/www/html/` o `public_html/`).
2. Asegúrate de que el usuario del servidor web (`www-data` o `apache`) tenga permisos de escritura en la carpeta `api/` para generar `datos.json`:
   ```bash
   chmod 775 api/
   ```
3. Accede a tu dominio: `http://tudominio.com/web/`.
4. El panel detectará automáticamente `api/index.php`. En la ESP32 ajusta:
   ```cpp
   const String API_URL = "http://tudominio.com/api/index.php";
   ```

### B. Despliegue en PythonAnywhere
1. Sube el proyecto a tu cuenta de PythonAnywhere en `/home/tu_usuario/proyecto`.
2. En la pestaña **Web**:
   - Crea una nueva web app seleccionando **Manual Configuration** con Python 3.10+.
   - Abre el archivo de configuración WSGI (`/var/www/tu_usuario_pythonanywhere_com_wsgi.py`) y pega el contenido de [pythonanywhere_wsgi.py](file:///media/edegantea/F0D6412CD640F47E/development/MECATRONICA/7A/ProgramacionAvanzada/proyecto/api/pythonanywhere_wsgi.py).
   - En la sección **Static Files**, añade:
     - **URL:** `/`
     - **Directory:** `/home/tu_usuario/proyecto/web`
   - Haz clic en **Reload tu_usuario.pythonanywhere.com**.
3. En la ESP32 ([prototipo.ino](file:///media/edegantea/F0D6412CD640F47E/development/MECATRONICA/7A/ProgramacionAvanzada/proyecto/prototipo/prototipo.ino)), configura la URL con **HTTPS** (soportado de forma nativa por la clase [`ESP32C3Dispositivo`](file:///media/edegantea/F0D6412CD640F47E/development/MECATRONICA/7A/ProgramacionAvanzada/proyecto/prototipo/ESP32C3Dispositivo.h) usando `WiFiClientSecure`):
   ```cpp
   const String API_URL = "https://tu_usuario.pythonanywhere.com/api/eventos";
   ```

