#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#define Moisture_Sensor_PIN 34


unsigned int humiditySufficientThreshold = 0; //Umbral para humedad suficiente (50%)
bool previousMoistureSufficient = false;  // Estado anterior de humedad suficiente

const char * MQTT_BROKER_HOST = "a2mnkqceizplps-ats.iot.us-east-1.amazonaws.com"; // Servidor MQTT de AWS IoT.
const int MQTT_BROKER_PORT = 8883; // Puerto del servidor MQTT.

const char * MQTT_CLIENT_ID = "Sensor Humedad"; // Identificador único del ESP-32 para el MQTT

const char * UPDATE_TOPIC = "$aws/things/thing/shadow/update"; // Topico del shadow - Update                
const char * UPDATE_DELTA_TOPIC = "$aws/things/thing/shadow/update/delta";  // Topico del shadow - Update Delta

// Certificado raíz de AWS
const char AMAZON_ROOT_CA1[] PROGMEM = R"EOF( 
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

//Certificado del servidor MQTT.
const char CERTIFICATE[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUW7B0pi+QTbXmqPDfgHxo6BZdRHMwDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI0MDYwMzAxMTIy
NFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAOv5qYYNj93WZvKnb9J8
ivlTapIYMmDWFySZwNPIEX4nHKj+QxWziw3WSTQ+sAA1K4uBS0IqCIM7ZTkWuGm3
HSJm1E/l2EEcI6Ns8siAhJusaFFwFgdKVYaVdKwEmnD9bCvErGIDTE2pH3HsGwfE
j4sPvS+y2/cDVnh4H3wPoRaqt3rox5nVuVUP4qnd8oDNe2o/lk6fufXQw92hTgdG
JGZNfMEX8INUVuE5Foy8XVuBYVmXd/B9Cm9SrNp8gI5EmvYqRDc6koL7RjUxr9mL
UDTilSUNo21JeydIHdyeIuyL+K++FDkcdIXU2NZHxJ3Df3c3TFSwTO2SjNpynEDW
YpECAwEAAaNgMF4wHwYDVR0jBBgwFoAUDglV7+gIVX4gRRjulrw23TCB+dAwHQYD
VR0OBBYEFNMeuYoOIdJvNQEce+6MooHCSCpdMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQBCajBvttxZ9sZrLiW8ow9oFiv/
ZGZz3Qn6lKe8wRLSHfWt5NKi3bMD/KoSfx3MaA231qaARKDFauuwuEHxcH9UjZSA
9dQD8+CCAEBwE3r+pIEVOUJPkJWSAtTBX0r9gvfaz0Ej+uKdD8SrPwfqedPd6xJX
4RCF9dGOIoq/IvtNtyJZ5atGH5+DQ7BH1nRSwryv0jqdF25FBVaumQMHduTIVL3e
CdTIpcVXIhnXNY99FjPS73089aD5SYoWCfzjw6dYhhikweCg6FEQVWTB84ANzKz4
EWI/VoTrOqkaEDzSBFF7Vo5pMv0UM22dH/+7liNjiiVCHTWqsUgWkbWy45+9
-----END CERTIFICATE-----
)KEY";

// Clave privada para el servidor MQTT.
const char PRIVATE_KEY[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
MIIEpgIBAAKCAQEA6/mphg2P3dZm8qdv0nyK+VNqkhgyYNYXJJnA08gRficcqP5D
FbOLDdZJND6wADUri4FLQioIgztlORa4abcdImbUT+XYQRwjo2zyyICEm6xoUXAW
B0pVhpV0rASacP1sK8SsYgNMTakfcewbB8SPiw+9L7Lb9wNWeHgffA+hFqq3eujH
mdW5VQ/iqd3ygM17aj+WTp+59dDD3aFOB0YkZk18wRfwg1RW4TkWjLxdW4FhWZd3
8H0Kb1Ks2nyAjkSa9ipENzqSgvtGNTGv2YtQNOKVJQ2jbUl7J0gd3J4i7Iv4r74U
ORx0hdTY1kfEncN/dzdMVLBM7ZKM2nKcQNZikQIDAQABAoIBAQC+SWVapDUPPoVl
x390zNmElK+rf7egPwQKj7HfFcaFZCTIYcDXDyFi1xnQ93wVrsqehHSbtimHKa5G
ivvKw87KnwE2LLJmTrquXnypEnnczvqQMUKSPm8ZSAv9avjfhHxmDwKzRtWRCoBA
7ZQef3MBQH/epuyaFCb2nmdFM5VDIHrFYX6QF/n+iziICGED4P+OBViUa2dDJb4s
0FkzlxaoYKn9Ircj7TCTvndGSGjuXOtUWvjzIfZuxCjzqjQPMoL7IpivP/FF6r5G
qAzYBfA36yaP/Z7HcpflgzdnIHrBswjlBKPG4h2/WA/lIeEItXnoGT2S5FkzaC0P
8fZb7mvBAoGBAPkkMP60mA3dpkfFdtoZixkYoaTRyB1i72MamOP0Dbd1lB0viRVZ
bTdSnzWwBZtoSoDzgspB6pd9Wq+EarkdPb7xMJLFJY5ntYqfjhAzRuGqenOkKuo2
5umk9coqfmQ1Y3WH9rusLUDpVkyV0mear4jTPP8dq/8oEM414AwgqFonAoGBAPJ4
ru4XH2TT2eA+Ycf4km4uj5KmwNiR4yd5GZdlh5YZJXtTfgRJbS+EwzZ7hhaNk5p7
mMKx7e+5pgdzlWxTxoT+DOVdgFu8lIkLVsyGUlfDtKLOSw00IXeVQa2hHLaRHSUu
EY2p0Lg3Pog8L+OCiuvhCWM+wg5B0ZKoc9cheWiHAoGBAMf+vnnofNs8n5ujrwnb
UWbe9/t4D31Piz1x/2OV6WDpOPHI0/FYbSzo3TdaytV6/bvQPqCgE3JyRVb6JDXf
fE6IDOb07XYIWJokBQWKNA85K70i/vQvCGRscIZOYFGO7f1OHMmAqH8gEI/AATv7
14ctkxx0NDrKbdsrMmRp3tsnAoGBAKX2yPWayhg/kbAVwuQXVlMod4lPrkwOf0bz
cvrXudiZWbhyS50vjRarbtsJ7ZveSBvDYapSE6S0k9oh2TMOnKFHKxiLyWIBUEIF
iuHvRslf4XJWdbSR7B5oAU6RKpAj/6kFQWqPw5dz6M9jMvwszF3r6HsP58/OaV5T
AwdLmEhdAoGBAMga+N0ChtGLTM8HbP2y4TDyiZYsIsrqUhMrY6uv7GQT/zDlsaJ9
6OEaTFH5v5+oEjabiyOgsH+Pt9dowojTOvH5UBlkgaIXjXZFHbyTedNKiwBJ6pxU
l1W8AUcaxhW9JuFlCakKj4I0Ut/Aa6wUgSJG4Iz3tmMsm8qLP+1/nnTr
-----END RSA PRIVATE KEY-----
)KEY";

unsigned long currentMillis; // Milisegundos desde que se inició el programa
unsigned long previousMillis; // Milisegundos del bucle anterior
StaticJsonDocument<JSON_OBJECT_SIZE(64)> inputDoc; // Almacenamos y procesamos los datos de entrada como JSON.
StaticJsonDocument<JSON_OBJECT_SIZE(64)> outputDoc; // Almacenamos y procesamos los datos de salida como JSON.
char outputBuffer[128]; // Almacenamos los JSON antes de ser enviados.


//////////////////////
// Función de callback para procesar los mensajes recibidos del servidor MQTT

void callback(const char* topic, byte* payload, unsigned int length) {
    Serial.println("Entrando en callback");

    // Copiar el payload en un buffer de cadena
    char jsonBuffer[length + 1];
    memcpy(jsonBuffer, payload, length);
    jsonBuffer[length] = '\0';

    // Mostrar el JSON recibido para depuración
    Serial.print("JSON recibido: ");
    Serial.println(jsonBuffer);

    // Deserializar el mensaje JSON
    DeserializationError err = deserializeJson(inputDoc, jsonBuffer);

    if (err) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(err.f_str());
        return;
    }

    // Mensaje de depuración para mostrar el tópico recibido
    Serial.print("Tópico recibido: ");
    Serial.println(topic);

    // Verificar si el tópico es el esperado
    if (String(topic) == UPDATE_DELTA_TOPIC) {
        Serial.println("Tópico coincide con UPDATE_DELTA_TOPIC");

        // Verificar si el campo "state" existe
        if (!inputDoc["state"]) {
            Serial.println("El campo 'state' no existe en el JSON recibido");
            return;
        }

        // Obtenemos el estado de la válvula
       
        // Registramos área
        if (inputDoc["state"].containsKey("recommended_humidity")) {
            Serial.print("-----ENTRO------");
            humiditySufficientThreshold = inputDoc["state"]["recommended_humidity"].as<int16_t>();
            Serial.print("recommended_humidity: ");
            Serial.println(humiditySufficientThreshold);
        } else {
            Serial.println("recommended_humidity no está en el mensaje JSON");

        } 

        // Reportamos el estado del riego
        //reportWatering();
    } else {
        Serial.println("Tópico no coincide con UPDATE_DELTA_TOPIC");
    }
}


class MQTTHandler {
  private:
    WiFiClientSecure wifiClient; // Cliente WiFi seguro
    PubSubClient mqttClient; // Cliente MQTT
    WiFiManager wifiManager; // Instancia de WiFiManager
  public:
    // Constructor de MQTTHandler
    MQTTHandler() : mqttClient(wifiClient) {}

    // Conectarse al WiFi usando WiFiManager
    void connectWiFi() {

      // Inicia el portal de configuración con nombre y contraseña del AP
      
      if (!wifiManager.autoConnect("ESP32_config", "123456789")) {
        Serial.println("No se pudo conectar a la red WiFi. Reiniciando...");
        delay(3000);
        ESP.restart();
      }

      // Verifica si se ha conectado
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi!");
      } else {
        Serial.println("Failed to connect to WiFi, restarting...");
        delay(3000);
        ESP.restart();
      }
    }

    // Conectarse al MQTT Broker
    void connectMQTT() {
      wifiClient.setCACert(AMAZON_ROOT_CA1); // Añadimos el certificado raíz del servidor MQTT
      wifiClient.setCertificate(CERTIFICATE); // Añadimos el certificado 
      wifiClient.setPrivateKey(PRIVATE_KEY); // Añadimos la clave privada 

      mqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT); // Ingresamos el servidor MQTT y el puerto
      mqttClient.setCallback(callback); // Configuramos el callback para manejar los mensajes MQTT recibidos

      Serial.print("Connecting to " + String(MQTT_BROKER_HOST)); 

      while (!mqttClient.connect(MQTT_CLIENT_ID)) {
        Serial.println("Failed to connect to MQTT broker, retrying in 5 seconds...");
        delay(5000); // Espera antes de intentar reconectar
      }
      Serial.println(" Connected to MQTT!");
      mqttClient.subscribe(UPDATE_DELTA_TOPIC); // Suscripción al tópico de actualización delta
      Serial.println("Subscribed to " + String(UPDATE_DELTA_TOPIC)); 
    }

    // Loop para mantener la conexión MQTT
    void loop() {
      if (!mqttClient.connected()) { // Verificamos conexión
        connectMQTT(); // Ejecutamos loop del cliente MQTT
      }
      mqttClient.loop();
    }

    // Método para publicar un mensaje MQTT en un tópico específico
    void publishMessage(const char* topic, const char* message) {
      mqttClient.publish(topic, message); // Publica el mensaje en el tópico especificado
    }
};

// Creamos una instancia de MQTTHandler
MQTTHandler mqttHandler;



void reportValue(const char* key, const char* value) {
  Serial.println("Reportando valores:");
  Serial.print("Key: ");
  Serial.println(key);
  Serial.print("Value: ");
  Serial.println(value);
  outputDoc["state"]["reported"][key] = value;
  serializeJson(outputDoc, outputBuffer);
  mqttHandler.publishMessage(UPDATE_TOPIC, outputBuffer);
}



void setup() {
  Serial.begin(115200);
  
  pinMode(Moisture_Sensor_PIN, INPUT);
  

  mqttHandler.connectWiFi();
  mqttHandler.connectMQTT();
}

void loop() {

  currentMillis = millis();
  mqttHandler.loop(); 
  if (currentMillis - previousMillis >= 1500) {
    
    int sensorValue = analogRead(Moisture_Sensor_PIN);  // Lectura del sensor de humedad
    float moisturePercentage = (1.0 - (float(sensorValue) / 4095.0)) * 100.0;
    Serial.print("Sensor Value: ");
    Serial.println(sensorValue);
    Serial.print("Moisture Percentage: ");
    Serial.println(moisturePercentage);
    bool moistureSufficient = moisturePercentage >= humiditySufficientThreshold;

    if (moistureSufficient != previousMoistureSufficient) {
        previousMoistureSufficient = moistureSufficient;
        if (moistureSufficient) {
          reportValue("moisture", "sufficient");
          Serial.println("sufficient");
        }
        else{
          reportValue("moisture", "low");
          Serial.println("low");
        }
    }
    delay(1000);  // Espera segundos antes de tomar otra lectura
  } 
}