/**
 * @file app.js
 * @brief Controlador del cliente web para la sincronización telemétrica con el ESP32-C3 Super Mini.
 * @details Realiza peticiones asíncronas periódicas (polling) hacia el endpoint REST configurado,
 *          actualiza dinámicamente el árbol DOM con el estado del actuador LED, la lectura térmica
 *          y renderiza el historial cronológico de eventos.
 */

// Referencias a los elementos del Modelo de Objetos del Documento (DOM)
const connectionBadge = document.getElementById('connectionBadge');
const connectionText = document.getElementById('connectionText');
const ledBulb = document.getElementById('ledBulb');
const ledText = document.getElementById('ledText');
const ledFooter = document.getElementById('ledFooter');
const tempValue = document.getElementById('tempValue');
const ipValue = document.getElementById('ipValue');
const lastEventValue = document.getElementById('lastEventValue');
const lastUpdateValue = document.getElementById('lastUpdateValue');
const eventsCount = document.getElementById('eventsCount');
const eventsTableBody = document.getElementById('eventsTableBody');
const currentApiDisplay = document.getElementById('currentApiDisplay');

// Componentes del cuadro de diálogo modal de configuración
const btnConfig = document.getElementById('btnConfig');
const configModal = document.getElementById('configModal');
const btnCancelConfig = document.getElementById('btnCancelConfig');
const btnSaveConfig = document.getElementById('btnSaveConfig');
const apiUrlInput = document.getElementById('apiUrlInput');
const presetButtons = document.querySelectorAll('.btn-preset');

// =========================================================================
// GESTIÓN Y RESOLUCIÓN DEL ENDPOINT DE LA API
// =========================================================================

/**
 * Determina el endpoint de conexión inicial evaluando persistencia local y contexto del servidor.
 * @returns {string} URI del servicio de backend seleccionado.
 */
function obtenerUrlApiInicial() {
    const urlGuardada = localStorage.getItem('esp32_api_url');
    if (urlGuardada) {
        return urlGuardada;
    }

    // Detección heurística para despliegues sobre infraestructura PHP
    const esEntornoPhp = window.location.pathname.endsWith('.php') || window.location.href.includes('.php');
    if (esEntornoPhp) {
        return '/api/index.php';
    }

    // Endpoint por defecto para servidores FastAPI, Flask o ejecuciones locales
    return '/api/eventos';
}

let API_ENDPOINT = obtenerUrlApiInicial();

/**
 * Actualiza el indicador textual del endpoint activo en la interfaz de usuario.
 */
function actualizarDisplayApi() {
    currentApiDisplay.textContent = API_ENDPOINT;
}

actualizarDisplayApi();

// Asignación de manejadores de eventos para la configuración del modal
btnConfig.addEventListener('click', () => {
    apiUrlInput.value = API_ENDPOINT;
    configModal.classList.add('active');
});

btnCancelConfig.addEventListener('click', () => {
    configModal.classList.remove('active');
});

configModal.addEventListener('click', (e) => {
    if (e.target === configModal) {
        configModal.classList.remove('active');
    }
});

presetButtons.forEach(btn => {
    btn.addEventListener('click', () => {
        apiUrlInput.value = btn.getAttribute('data-url');
    });
});

btnSaveConfig.addEventListener('click', () => {
    const nuevaUrl = apiUrlInput.value.trim();
    if (nuevaUrl) {
        API_ENDPOINT = nuevaUrl;
        localStorage.setItem('esp32_api_url', nuevaUrl);
        actualizarDisplayApi();
        configModal.classList.remove('active');
        actualizarEstado(); // Ejecuta una consulta inmediata con el nuevo endpoint
    }
});

// =========================================================================
// CICLO DE SINCRONIZACIÓN Y ADQUISICIÓN DE ESTADO (POLLING)
// =========================================================================

/**
 * Ejecuta una consulta asíncrona hacia el servicio REST para recuperar el estado global.
 */
async function actualizarEstado() {
    try {
        const respuesta = await fetch(API_ENDPOINT);
        
        // Mecanismo de degradación suave: si /api/eventos retorna 404, prueba con /api/index.php
        if (respuesta.status === 404 && API_ENDPOINT === '/api/eventos' && !localStorage.getItem('esp32_api_url')) {
            console.warn("[TELEMETRÍA] Endpoint no encontrado. Intentando alternativa PHP (/api/index.php)...");
            API_ENDPOINT = '/api/index.php';
            actualizarDisplayApi();
            return actualizarEstado();
        }

        if (!respuesta.ok) {
            throw new Error(`Código de estado HTTP: ${respuesta.status}`);
        }

        const data = await respuesta.json();
        const estado = data.estado || {};
        const eventos = data.eventos || [];

        // Notificación de estado de enlace saludable
        connectionBadge.className = 'status-indicator connected';
        connectionText.textContent = 'API Conectada';

        // Renderizado del componente visual del actuador LED
        if (estado.led) {
            ledBulb.classList.add('active');
            ledText.textContent = 'ENCENDIDO';
            ledFooter.textContent = 'Línea lógica GPIO 8 activa';
        } else {
            ledBulb.classList.remove('active');
            ledText.textContent = 'APAGADO';
            ledFooter.textContent = 'Línea lógica GPIO 8 inactiva';
        }

        // Renderizado del valor térmico
        if (typeof estado.temperatura === 'number' && estado.temperatura !== 0.0) {
            tempValue.textContent = estado.temperatura.toFixed(1);
        }

        // Actualización de metadatos de red y último evento
        ipValue.textContent = estado.ultima_ip || 'No registrada';
        lastEventValue.textContent = estado.ultimo_evento || 'Ninguno';
        lastUpdateValue.textContent = estado.ultima_actualizacion || '--';
        eventsCount.textContent = `${eventos.length} evento${eventos.length === 1 ? '' : 's'}`;

        // Renderizado de la tabla histórica
        renderizarEventos(eventos);

    } catch (error) {
        connectionBadge.className = 'status-indicator error';
        connectionText.textContent = 'API Desconectada';
        console.warn(`[TELEMETRÍA] Fallo en la comunicación con el endpoint (${API_ENDPOINT}):`, error);
    }
}

/**
 * Genera el marcado HTML de la tabla con los registros cronológicos de eventos.
 * @param {Array<Object>} eventos Colección de eventos provistos por el backend.
 */
function renderizarEventos(eventos) {
    if (!eventos || eventos.length === 0) {
        eventsTableBody.innerHTML = `
            <tr class="empty-row">
                <td colspan="6">En espera de eventos emitidos por el microcontrolador ESP32-C3...</td>
            </tr>
        `;
        return;
    }

    // Ordenamiento inverso: presenta el registro más reciente en la primera posición
    const eventosOrdenados = [...eventos].reverse();

    const filasHtml = eventosOrdenados.map(ev => {
        const claseBadge = `badge-event ${ev.evento || ''}`;
        const valorFormateado = (ev.valor !== undefined && ev.valor !== null) ? ev.valor : '-';
        return `
            <tr>
                <td>${ev.timestamp || '--'}</td>
                <td><strong>${ev.dispositivo || 'ESP32'}</strong></td>
                <td><span class="${claseBadge}">${ev.evento || 'desconocido'}</span></td>
                <td>${ev.mensaje || ''}</td>
                <td>${valorFormateado}</td>
                <td>${ev.ip || 'Local'}</td>
            </tr>
        `;
    }).join('');

    eventsTableBody.innerHTML = filasHtml;
}

// Inicialización del temporizador de muestreo periódico (1000 ms)
actualizarEstado();
setInterval(actualizarEstado, 1000);
