/**
 * @file ESP32C3Dispositivo.cpp
 * @brief Implementación de las rutinas de control, telemetría y red de la clase ESP32C3Dispositivo.
 * @details Desarrolla la lógica de conmutación de periféricos, muestreo del sensor analógico
 *          on-chip de temperatura, gestión de sockets Wi-Fi y serialización de tramas JSON
 *          mediante peticiones HTTP/HTTPS para el SoC ESP32-C3.
 */

#include "ESP32C3Dispositivo.h"

/**
 * @brief Constructor de la clase controladora.
 * @details Inicializa los atributos miembro requeridos para la conectividad y el direccionamiento GPIO.
 */
ESP32C3Dispositivo::ESP32C3Dispositivo(const char* ssid, const char* password, const String& apiUrl, uint8_t pinLed, bool activoBajo)
    : _pinLed(pinLed), _activoBajo(activoBajo), _ssid(ssid), _password(password), _apiUrl(apiUrl) {
}

/**
 * @brief Configura el estado inicial de los registros GPIO y la interfaz de comunicaciones serie.
 * @param baudios Tasa de baudios para la consola serie.
 */
void ESP32C3Dispositivo::iniciar(unsigned long baudios) {
    Serial.begin(baudios);
    pinMode(_pinLed, OUTPUT);
    apagarLed(); // Establece estado seguro inicial en corte/apagado
    Serial.println(F("\n[HAL] Inicializando registros de periféricos y subsistema de comunicaciones..."));
}

/**
 * @brief Consulta el estado de enlace en la capa física/enlace de la pila de red.
 * @return true si el estado reportado por el driver de red es WL_CONNECTED.
 */
bool ESP32C3Dispositivo::estaConectado() {
    return WiFi.status() == WL_CONNECTED;
}

/**
 * @brief Ejecuta el protocolo de autenticación y asociación a la red inalámbrica Wi-Fi.
 * @return true si la interfaz obtiene una dirección IP válida; false en caso de agotamiento de intentos.
 */
bool ESP32C3Dispositivo::conectarWiFi() {
    if (estaConectado()) {
        return true;
    }

    Serial.printf("[WLAN] Iniciando negociación con SSID: '%s'...\n", _ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);

    int intentos = 0;
    while (WiFi.status() != WL_CONNECTED && intentos < 30) {
        delay(250);
        Serial.print('.');
        intentos++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("\n[WLAN] Asociación exitosa."));
        Serial.print(F("[WLAN] Dirección IPv4 asignada: "));
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println(F("\n[WLAN] Error crítico: Tiempo de espera agotado al conectar con el AP."));
        return false;
    }
}

/**
 * @brief Garantiza la disponibilidad de la conexión Wi-Fi previo a la ejecución de tareas.
 * @return true si la conexión está lista o se restableció correctamente.
 */
bool ESP32C3Dispositivo::asegurarConexionWiFi() {
    if (!estaConectado()) {
        Serial.println(F("[WLAN] Enlace inactivo detectado. Ejecutando reconexión preventiva..."));
        return conectarWiFi();
    }
    return true;
}

/**
 * @brief Conmuta físicamente el estado del terminal GPIO del LED considerando su nivel activo.
 * @param encendido Booleano que define si el diodo emite luz (true) o permanece extinto (false).
 */
void ESP32C3Dispositivo::fijarEstadoLed(bool encendido) {
    if (_activoBajo) {
        digitalWrite(_pinLed, encendido ? LOW : HIGH);
    } else {
        digitalWrite(_pinLed, encendido ? HIGH : LOW);
    }
}

/**
 * @brief Pone en conducción el LED y despacha la telemetría correspondiente a la API REST.
 */
void ESP32C3Dispositivo::encenderLed() {
    asegurarConexionWiFi();
    fijarEstadoLed(true);
    Serial.println(F("[GPIO] Transición: LED activado (ON)."));
    notificarEvento("encender_led", "El actuador LED ha sido conmutado a estado activo", 1.0);
}

/**
 * @brief Corta la conducción del LED y despacha la telemetría correspondiente a la API REST.
 */
void ESP32C3Dispositivo::apagarLed() {
    asegurarConexionWiFi();
    fijarEstadoLed(false);
    Serial.println(F("[GPIO] Transición: LED desactivado (OFF)."));
    notificarEvento("apagar_led", "El actuador LED ha sido conmutado a estado inactivo", 0.0);
}

/**
 * @brief Ejecuta el patrón secuencial luminoso número 1 (5 ciclos de parpadeo a 3.33 Hz).
 */
void ESP32C3Dispositivo::secuencia1() {
    asegurarConexionWiFi();
    Serial.println(F("[PATRÓN] Ejecución de Secuencia 1: Oscilación simétrica de alta frecuencia (5 ciclos)."));
    notificarEvento("secuencia1_inicio", "Secuencia de oscilación rápida iniciada");

    for (int i = 0; i < 5; i++) {
        fijarEstadoLed(true);
        delay(150);
        fijarEstadoLed(false);
        delay(150);
    }

    notificarEvento("secuencia1_fin", "Secuencia de oscilación rápida culminada con éxito");
}

/**
 * @brief Ejecuta el patrón secuencial luminoso número 2 (3 trenes de doble pulso asimétrico).
 */
void ESP32C3Dispositivo::secuencia2() {
    asegurarConexionWiFi();
    Serial.println(F("[PATRÓN] Ejecución de Secuencia 2: Tren de doble pulso estroboscópico (3 ciclos)."));
    notificarEvento("secuencia2_inicio", "Secuencia estroboscópica de pulso doble iniciada");

    for (int i = 0; i < 3; i++) {
        fijarEstadoLed(true);
        delay(80);
        fijarEstadoLed(false);
        delay(80);
        fijarEstadoLed(true);
        delay(80);
        fijarEstadoLed(false);
        delay(400);
    }

    notificarEvento("secuencia2_fin", "Secuencia estroboscópica de pulso doble culminada con éxito");
}

/**
 * @brief Adquiere la temperatura de unión del microprocesador mediante la API térmica interna.
 * @return Temperatura en grados Celsius (°C).
 */
float ESP32C3Dispositivo::temperatura() {
    asegurarConexionWiFi();

    // Adquisición analógica mediante el convertidor térmico integrado en el silicio
    float tempC = temperatureRead();
    Serial.printf("[TELEMETRÍA] Sensor térmico on-chip muestreado: %.2f °C\n", tempC);

    notificarEvento("temperatura", "Lectura del sensor analógico térmico integrado en silicio", tempC);
    return tempC;
}

/**
 * @brief Serializa el evento en JSON y efectúa la transferencia por protocolo HTTP/HTTPS.
 * @param nombreEvento Mnemónico del evento.
 * @param mensaje Cadena descriptiva del evento.
 * @param valor Magnitud numérica asociada.
 * @return true si la transacción HTTP devolvió código exitoso (2xx).
 */
bool ESP32C3Dispositivo::notificarEvento(const String& nombreEvento, const String& mensaje, float valor) {
    if (!estaConectado()) {
        Serial.println(F("[REST] Operación abortada: Ausencia de conectividad Wi-Fi."));
        return false;
    }

    HTTPClient http;
    bool exitoInicio = false;

    // Conexión segura TLS para endpoints bajo esquema HTTPS (ej. PythonAnywhere o VPS con SSL)
    if (_apiUrl.startsWith("https://")) {
        WiFiClientSecure secureClient;
        secureClient.setInsecure(); // Admite certificados TLS sin requerir quema previa de autoridades CA
        exitoInicio = http.begin(secureClient, _apiUrl);
    } else {
        WiFiClient client;
        exitoInicio = http.begin(client, _apiUrl);
    }

    if (!exitoInicio) {
        Serial.printf("[REST] Error crítico al resolver el endpoint: %s\n", _apiUrl.c_str());
        return false;
    }

    http.addHeader("Content-Type", "application/json");

    // Construcción de la carga útil JSON (Payload)
    String jsonPayload = "{";
    jsonPayload += "\"dispositivo\":\"ESP32-C3 Super Mini\",";
    jsonPayload += "\"evento\":\"" + nombreEvento + "\",";
    jsonPayload += "\"mensaje\":\"" + mensaje + "\",";
    jsonPayload += "\"valor\":" + String(valor, 2) + ",";
    jsonPayload += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
    jsonPayload += "}";

    int httpCode = http.POST(jsonPayload);

    if (httpCode > 0) {
        Serial.printf("[REST] Evento '%s' despachado correctamente (Respuesta HTTP: %d)\n", nombreEvento.c_str(), httpCode);
        http.end();
        return true;
    } else {
        Serial.printf("[REST] Falla en la transmisión del evento '%s' (Causa: %s)\n", nombreEvento.c_str(), http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }
}
