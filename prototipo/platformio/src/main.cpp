/**
 * @file main.cpp
 * @brief Programa principal de control y telemetría para ESP32-C3 Super Mini (PlatformIO).
 * @details Orquesta la inicialización del hardware, la conexión a la infraestructura
 *          Wi-Fi y la ejecución de los métodos de control lumínico y lectura térmica
 *          bajo el paradigma orientado a objetos (POO).
 * 
 * @author Mecatrónica - Programación Avanzada
 * @version 1.1.0
 */

#include <Arduino.h>
#include "ESP32C3Dispositivo.h"

// =========================================================================
// PARÁMETROS DE CONFIGURACIÓN DE RED Y SERVICIOS
// =========================================================================

/** @brief Identificador del conjunto de servicios inalámbricos (SSID en 2.4 GHz). */
const char* WIFI_SSID     = "MKT7A";

/** @brief Clave de autenticación para la infraestructura Wi-Fi. */
const char* WIFI_PASSWORD = "87654321";

/**
 * @brief Localizador uniforme de recursos (URI) del endpoint de la API telemétrica.
 * @note Opciones típicas de despliegue:
 *       - Local / LAN:     "http://192.168.1.100:8000/api/eventos"
 *       - VPS con PHP:     "http://tu-dominio.com/api/index.php"
 *       - PythonAnywhere:  "https://tu_usuario.pythonanywhere.com/api/eventos"
 */
const String API_URL      = "http://10.147.64.179:8000/api/eventos";

// =========================================================================
// ASIGNACIÓN DE HARDWARE (ESP32-C3 SUPER MINI)
// =========================================================================

/**
 * @brief Terminal GPIO asignado al diodo emisor de luz integrado en placa.
 * @note En la revisión ESP32-C3 Super Mini, el LED azul de usuario reside en GPIO 8.
 */
const uint8_t PIN_LED     = 8;

/**
 * @brief Configuración de lógica activa para la excitación del LED integrado.
 * @note La topología de circuito utiliza nivel bajo activo (Active LOW: LOW = ON, HIGH = OFF).
 */
const bool LED_ACTIVO_BAJO = true;

// =========================================================================
// INSTANCIACIÓN DE LA ENTIDAD CONTROLADORA (POO)
// =========================================================================

/**
 * @brief Instancia global del controlador de la tarjeta embebida.
 */
ESP32C3Dispositivo dispositivo(WIFI_SSID, WIFI_PASSWORD, API_URL, PIN_LED, LED_ACTIVO_BAJO);

/**
 * @brief Rutina de configuración inicial del sistema (ejecutada una única vez al arranque).
 * @details Configura registros, inicializa la consola CDC USB nativa, establece la asociación
 *          de red y valida secuencialmente cada uno de los métodos requeridos.
 */
void setup() {
    // 1. Inicialización de periféricos y UART/USB CDC a 115200 baudios
    dispositivo.iniciar(115200);

    // Retardo de estabilización para el bus USB CDC nativo
    delay(1000);

    // 2. Establecimiento mandatorio de enlace inalámbrico Wi-Fi
    Serial.println(F("\n========================================================"));
    Serial.println(F(" SISTEMA DE TELEMETRÍA Y CONTROL ESP32-C3 SUPER MINI"));
    Serial.println(F("========================================================"));
    Serial.println(F("[INICIO] Negociando asociación a la red inalámbrica..."));
    
    if (dispositivo.conectarWiFi()) {
        Serial.println(F("[INICIO] Conectividad Wi-Fi operativa. Iniciando batería de pruebas..."));
    } else {
        Serial.println(F("[ALERTA] Fallo inicial de enlace Wi-Fi. Las funciones intentarán reconectar."));
    }

    // 3. Ejecución secuencial de los métodos de control y telemetría:

    // Método 1: Excitación del LED indicador
    Serial.println(F("\n--- [Prueba 1] Método: encenderLed() ---"));
    dispositivo.encenderLed();
    delay(2000);

    // Método 2: Desactivación del LED indicador
    Serial.println(F("\n--- [Prueba 2] Método: apagarLed() ---"));
    dispositivo.apagarLed();
    delay(1000);

    // Método 3: Secuencia luminosa rápida
    Serial.println(F("\n--- [Prueba 3] Método: secuencia1() ---"));
    dispositivo.secuencia1();
    delay(1000);

    // Método 4: Secuencia luminosa bifásica estroboscópica
    Serial.println(F("\n--- [Prueba 4] Método: secuencia2() ---"));
    dispositivo.secuencia2();
    delay(1000);

    // Método 5: Muestreo de temperatura del silicio
    Serial.println(F("\n--- [Prueba 5] Método: temperatura() ---"));
    float temp = dispositivo.temperatura();
    Serial.printf("[TELEMETRÍA] Lectura térmica actual: %.2f °C\n", temp);
}

/**
 * @brief Bucle cíclico principal del sistema.
 * @details Ejecuta de forma periódica el muestreo y despacho telemétrico de la temperatura on-chip.
 */
void loop() {
    delay(10000);
    dispositivo.temperatura();
}
