<?php
/**
 * @file index.php
 * @package ESP32C3_Telemetry_Backend
 * @brief Implementación de la API REST en PHP para servidores web (Apache/Nginx/cPanel/VPS).
 * @details Procesa peticiones síncronas HTTP POST para la ingestión de tramas de telemetría
 *          y peticiones HTTP GET para el suministro de estado a la interfaz de usuario.
 *          Garantiza la persistencia de datos mediante almacenamiento transaccional en un
 *          archivo JSON local con bloqueo exclusivo de concurrencia (flock).
 */

header("Access-Control-Allow-Origin: *");
header("Access-Control-Allow-Methods: GET, POST, OPTIONS");
header("Access-Control-Allow-Headers: Content-Type");
header("Content-Type: application/json; charset=UTF-8");

// Gestión de solicitudes de verificación preliminar (Pre-flight CORS)
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

/** @var string Ruta absoluta al archivo de persistencia transaccional. */
$archivoDatos = __DIR__ . '/datos.json';

/**
 * Carga y deserializa el estado del sistema desde el medio de almacenamiento persistente.
 *
 * @param string $ruta Ubicación física del archivo de datos JSON.
 * @return array Estructura asociativa con el estado y el historial de eventos.
 */
function cargarDatos(string $ruta): array {
    if (file_exists($ruta)) {
        $contenido = file_get_contents($ruta);
        $data = json_decode($contenido, true);
        if (is_array($data)) {
            return $data;
        }
    }
    return [
        "estado" => [
            "led" => false,
            "temperatura" => 0.0,
            "ultima_ip" => null,
            "ultimo_evento" => "Ninguno",
            "ultima_actualizacion" => null
        ],
        "eventos" => []
    ];
}

/**
 * Serializa y almacena de forma segura el estado del sistema aplicando bloqueo atómico de archivo.
 *
 * @param string $ruta Ubicación física del archivo de datos JSON.
 * @param array $data Estructura de datos asociativa a persistir.
 * @return bool true si la operación de escritura y sincronización culminó con éxito.
 */
function guardarDatos(string $ruta, array $data): bool {
    $fp = fopen($ruta, 'w');
    if ($fp && flock($fp, LOCK_EX)) {
        fwrite($fp, json_encode($data, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE));
        flock($fp, LOCK_UN);
        fclose($fp);
        return true;
    }
    return false;
}

$metodo = $_SERVER['REQUEST_METHOD'];

// =========================================================================
// ENRUTAMIENTO HTTP REST
// =========================================================================

if ($metodo === 'POST') {
    $rawInput = file_get_contents('php://input');
    $payload = json_decode($rawInput, true);

    if (!is_array($payload)) {
        http_response_code(400);
        echo json_encode(["error" => "Carga útil JSON nula o con errores de sintaxis"]);
        exit;
    }

    $db = cargarDatos($archivoDatos);
    $timestamp = date("Y-m-d H:i:s");
    $evento = $payload['evento'] ?? '';

    // Transición de estados del hardware
    if ($evento === 'encender_led') {
        $db['estado']['led'] = true;
    } elseif ($evento === 'apagar_led') {
        $db['estado']['led'] = false;
    } elseif ($evento === 'temperatura') {
        $db['estado']['temperatura'] = floatval($payload['valor'] ?? 0.0);
    }

    $db['estado']['ultimo_evento'] = $evento;
    $db['estado']['ultima_ip'] = $payload['ip'] ?? $_SERVER['REMOTE_ADDR'];
    $db['estado']['ultima_actualizacion'] = $timestamp;

    // Inserción en el historial circular (limitado a los últimos 50 eventos)
    $payload['timestamp'] = $timestamp;
    $db['eventos'][] = $payload;
    if (count($db['eventos']) > 50) {
        $db['eventos'] = array_slice($db['eventos'], -50);
    }

    guardarDatos($archivoDatos, $db);

    echo json_encode([
        "status" => "ok",
        "recibido" => true,
        "evento" => $evento,
        "timestamp" => $timestamp
    ]);
    exit;
}

if ($metodo === 'GET') {
    $db = cargarDatos($archivoDatos);
    echo json_encode($db);
    exit;
}

http_response_code(405);
echo json_encode(["error" => "Método HTTP no admitido"]);
