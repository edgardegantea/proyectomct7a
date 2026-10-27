/**
 * @file ESP32C3Dispositivo.h
 * @brief Especificación formal de la clase controladora para el módulo ESP32-C3 Super Mini.
 * @details Esta biblioteca implementa un modelo de diseño basado en el paradigma orientado
 *          a objetos (POO) para la gestión del microcontrolador SoC ESP32-C3. Encapsula
 *          el control de periféricos de propósito general (GPIO), la adquisición de
 *          temperatura mediante el sensor térmico on-chip, el mantenimiento del enlace
 *          inalámbrico IEEE 802.11 b/g/n (Wi-Fi) y el protocolo de telemetría HTTP/HTTPS
 *          hacia un servicio de backend RESTful.
 * 
 * @author Mecatrónica - Programación Avanzada
 * @version 1.1.0
 */

#ifndef ESP32C3_DISPOSITIVO_H
#define ESP32C3_DISPOSITIVO_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

/**
 * @class ESP32C3Dispositivo
 * @brief Controlador integral del dispositivo embebido ESP32-C3 Super Mini.
 * 
 * Gestiona el ciclo de vida del periférico emisor de luz (LED), la ejecución de patrones
 * secuenciales de conmutación, la lectura del convertidor térmico integrado y la
 * transmisión síncrona de tramas JSON de eventos a través de sockets TCP/TLS.
 */
class ESP32C3Dispositivo {
private:
    uint8_t _pinLed;          /**< Línea GPIO asignada al periférico LED. */
    bool _activoBajo;         /**< Lógica de excitación eléctrica (true: nivel activo bajo). */
    const char* _ssid;        /**< Identificador de conjunto de servicios (SSID) de la red Wi-Fi. */
    const char* _password;    /**< Clave criptográfica de acceso a la infraestructura Wi-Fi. */
    String _apiUrl;           /**< Localizador uniforme de recursos (URI) del endpoint de la API. */

    /**
     * @brief Establece el estado lógico en la línea física del LED.
     * @param encendido Estado booleano deseado (true: conducción/iluminación, false: reposo/corte).
     */
    void fijarEstadoLed(bool encendido);

    /**
     * @brief Verifica la continuidad del enlace Wi-Fi e inicia la reconexión de ser necesario.
     * @details Precondición obligatoria ejecutada de manera previa a cualquier operación de hardware
     *          o emisión telemétrica para garantizar la conectividad de la pila de red.
     * @return true si la interfaz de red cuenta con una dirección IP asignada válida; false en caso de fallo.
     */
    bool asegurarConexionWiFi();

public:
    /**
     * @brief Constructor parametrizado de la clase controladora.
     * @param ssid Identificador de la red inalámbrica objetivo (2.4 GHz).
     * @param password Clave WPA/WPA2/WPA3 de la red inalámbrica.
     * @param apiUrl Endpoint del servicio API REST receptor (soporta esquemas http:// y https://).
     * @param pinLed Identificador del terminal GPIO (por defecto: GPIO 8 en ESP32-C3 Super Mini).
     * @param activoBajo Configuración lógica del actuador (true: nivel bajo excita el LED).
     */
    ESP32C3Dispositivo(const char* ssid, const char* password, const String& apiUrl, uint8_t pinLed = 8, bool activoBajo = true);

    /**
     * @brief Inicializa los controladores de periféricos y la consola serie del SoC.
     * @param baudios Velocidad de transmisión en baudios para la interfaz UART/USB CDC (por defecto: 115200).
     */
    void iniciar(unsigned long baudios = 115200);

    /**
     * @brief Establece la asociación y autenticación en la infraestructura inalámbrica Wi-Fi.
     * @details Configura la estación en modo STA (Station Mode) y bloquea temporalmente hasta
     *          obtener una concesión DHCP válida o agotar la ventana de temporización (timeout).
     * @return true si el enlace se estableció satisfactoriamente (WL_CONNECTED); false en caso de expiración.
     */
    bool conectarWiFi();

    /**
     * @brief Activa el estado de conducción del periférico LED y notifica el evento al servidor API.
     * @pre Requiere conexión activa a la red Wi-Fi; invoca internamente asegurarConexionWiFi().
     */
    void encenderLed();

    /**
     * @brief Interrumpe la conducción del periférico LED y notifica el evento al servidor API.
     * @pre Requiere conexión activa a la red Wi-Fi; invoca internamente asegurarConexionWiFi().
     */
    void apagarLed();

    /**
     * @brief Ejecuta el patrón secuencial luminoso número 1 (conmutación periódica rápida).
     * @details Realiza un ciclo de 5 repeticiones con una frecuencia fija de oscilación y registra
     *          los eventos de inicio y culminación en el backend.
     * @pre Requiere conexión activa a la red Wi-Fi; invoca internamente asegurarConexionWiFi().
     */
    void secuencia1();

    /**
     * @brief Ejecuta el patrón secuencial luminoso número 2 (destellos bifásicos estroboscópicos).
     * @details Emite trenes de pulsos dobles con intervalos asimétricos de atenuación, registrando
     *          el estado de ejecución ante la API remota.
     * @pre Requiere conexión activa a la red Wi-Fi; invoca internamente asegurarConexionWiFi().
     */
    void secuencia2();

    /**
     * @brief Adquiere el valor térmico actual reportado por el transductor analógico-digital on-chip.
     * @details Invoca la API nativa de bajo nivel del silicio para muestrear la unión semiconductora
     *          del microprocesador y transmite el resultado serializado a la API REST.
     * @pre Requiere conexión activa a la red Wi-Fi; invoca internamente asegurarConexionWiFi().
     * @return Temperatura en la escala Celsius (°C) calculada por el hardware.
     */
    float temperatura();

    /**
     * @brief Ensambla y transmite una trama telemétrica en formato JSON al backend.
     * @details Resuelve dinámicamente la capa de transporte: inicializa un socket seguro TLS (WiFiClientSecure)
     *          si el esquema es HTTPS, o un socket estándar TCP (WiFiClient) si es HTTP plano.
     * @param nombreEvento Código mnemónico que identifica el evento originado en el dispositivo.
     * @param mensaje Descripción cualitativa o detalle del estado operativo del evento.
     * @param valor Magnitud escalar cuantitativa asociada (ej. temperatura o estado binario).
     * @return true si el servidor devolvió un código de estado HTTP 2xx; false en caso contrario.
     */
    bool notificarEvento(const String& nombreEvento, const String& mensaje = "", float valor = 0.0);

    /**
     * @brief Dictamina el estado instantáneo de la asociación a la capa de enlace inalámbrico.
     * @return true si la interfaz Wi-Fi mantiene una dirección IP válida (WL_CONNECTED); false en su defecto.
     */
    bool estaConectado();
};

#endif // ESP32C3_DISPOSITIVO_H
