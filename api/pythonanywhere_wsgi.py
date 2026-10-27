"""
Archivo de configuración WSGI para PythonAnywhere.

INSTRUCCIONES PARA PYTHONANYWHERE:
1. Sube tu proyecto a PythonAnywhere (por ejemplo a /home/tu_usuario/proyecto).
2. En la pestaña 'Web' de PythonAnywhere, crea una aplicación web seleccionando Manual Configuration (Python 3.10 o superior).
3. Haz clic en el enlace al archivo de configuración WSGI (/var/www/tu_usuario_pythonanywhere_com_wsgi.py).
4. Borra su contenido y pega el código que se muestra a continuación, ajustando 'tu_usuario'.
5. En la sección 'Static Files' de la pestaña Web, añade:
     URL: /
     Directory: /home/tu_usuario/proyecto/web
6. Haz clic en 'Reload tu_usuario.pythonanywhere.com'.
"""

import sys
import os

# Ajusta el nombre de tu usuario en PythonAnywhere:
USUARIO_PYTHONANYWHERE = "tu_usuario"
PROJECT_DIR = f"/home/{USUARIO_PYTHONANYWHERE}/proyecto"

if PROJECT_DIR not in sys.path:
    sys.path.insert(0, PROJECT_DIR)

# Importamos la instancia de la aplicación Flask
from api.app_flask import app as application

