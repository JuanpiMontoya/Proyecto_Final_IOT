//Librerias necesarias y PIN del sensor y actuador
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#define Sensor_PIN  13
#define Solenoid_Valve_PIN  15

// MQTT Variables (Variables del sensor)
const char * MQTT_BROKER_HOST = "a2mnkqceizplps-ats.iot.us-east-1.amazonaws.com"; // Servidor MQTT de AWS IoT.
const int MQTT_BROKER_PORT = 8883; // Puerto del servidor MQTT.

const char * MQTT_CLIENT_ID = "ESP-32"; // Identificador único del ESP-32 para el MQTT

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

StaticJsonDocument<2048> inputDoc; // Almacenamos y procesamos los datos de entrada como JSON.
StaticJsonDocument<128> outputDoc; // Almacenamos y procesamos los datos de salida como JSON.
char outputBuffer[256]; // Almacenamos los JSON antes de ser enviados.

// Flow Sensor Variables (Variables del sensor)

unsigned long currentMillis = 0; // Milisegundos desde que se inició el programa
unsigned long previousMillis = 0; // Milisegundos del bucle anterior
byte pulse1Sec = 0; // número de pulsos del sensor que ocurrieron en el último segundo
float flowRate = 0; // El valor de litros / minuto
unsigned int flowMilliLitres = 0; // Cantidad de líquido (ml)
unsigned long totalMilliLitres; // Cantidad de líquido total (ml)
int WATER_LIMIT; // Medida en ml
int area; // área a regar en m2
String plant_type; // tipo de planta
bool valve_state = false;

volatile byte pulseCount; // Variable global para contar los pulsos

Preferences preferences; // Guardado de las variables ante perdidas de conexion

// Clase FlowSensor
class FlowSensor {
  private:
    const float calibrationFactor; 
  public:
    // Constructor de FlowSensor
    FlowSensor(float calibrationFactor) : calibrationFactor(calibrationFactor) {}

    // Método get del factor de calibración
    float getCalibrationFactor() const {
      return calibrationFactor;
    }
};

// Creamos una instancia FlowSensor con el factor de calibración respectivo
FlowSensor flowSensor(3);

// Función para contar los pulsos del sensor de flujo
void IRAM_ATTR pulseCounter() {
  pulseCount++; // Incrementamos el contador de pulsos
}

// Calculo de la la tasa de flujo
void calculateFlowRate(unsigned long currentMillis) {
  // Verifica si ha pasado 1.5 segundos desde la última vez que se calculó la tasa de flujo
  if (currentMillis - previousMillis > 1500) {
    byte pulse1Sec = pulseCount;
    pulseCount = 0;

    // Calculo de la tasa de flujo (L/min)
    flowRate = ((1000.0 / (currentMillis - previousMillis)) * pulse1Sec) / flowSensor.getCalibrationFactor();
    previousMillis = currentMillis;
  }
}

// Calculo del flujo total
void calculateTotalFlow() {
  flowMilliLitres = (flowRate / 60) * 1000; // Conversion de la tasa de flujo (a ml/seg)
  totalMilliLitres += flowMilliLitres; // Incrementa el total de mililitros
}

// Clase SolenoidValve
class SolenoidValve {
  private:
    const int pin; // Pin  de la electroválvula
    bool isOpen; // Estado de la electroválvula (abierta o cerrada)
  public:
    // Constructor de SolenoidValve 
    SolenoidValve(int pin) : pin(pin), isOpen(false) {
        pinMode(pin, OUTPUT); 
    }

    // Método para abrir o cerrar la válvula
    void open_or_close(bool valve_state) {
        digitalWrite(pin, valve_state); // Cambiamos estado del pin
    }

    // Método para verificar si la válvula está abierta
    bool isOpened(){
        return isOpen; 
    }

    void change_Opened_state(bool valve_state){
      isOpen = valve_state; 
    }
};

// Creamos una instancia de SolenoidValve
SolenoidValve solenoidValve(Solenoid_Valve_PIN);

void callback(const char* topic, byte* payload, unsigned int length) {
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

    // Verificar si el tópico es el esperado
    if (String(topic) == UPDATE_DELTA_TOPIC) {

          // Verificar si el campo "watering" existe
          if (inputDoc["state"].containsKey("watering")) {
              Serial.println("ACTUALIZANDO EL ESTADO DE LA VÁLVULA");
              // Obtenemos el estado de la válvula
              valve_state = inputDoc["state"]["watering"].as<int8_t>() != 0;
              solenoidValve.open_or_close(valve_state); // Abrimos o cerramos la válvula
              solenoidValve.change_Opened_state(valve_state);
          }

          // Verificar si el campo "water_limit" existe
          if (inputDoc["state"].containsKey("water_limit")) {
              // Registramos límite de agua
              WATER_LIMIT = inputDoc["state"]["water_limit"].as<int16_t>();
              preferences.putInt("water_limit", WATER_LIMIT);
          }

          // Registramos área
          if (inputDoc["state"].containsKey("area")) {
              area = inputDoc["state"]["area"].as<int16_t>();
              preferences.putInt("area", area);
          } 

          // Registramos tipo de planta
          if (inputDoc["state"].containsKey("plant_type")) {
              plant_type = inputDoc["state"]["plant_type"].as<const char*>();
              preferences.putString("plant_type", plant_type);
          } 
    } 
    // Reportamos el estado del riego
    reportWatering();
} 


// Clase MQTTHandler
class MQTTHandler {
  private:
    WiFiClientSecure wifiClient; // Cliente WiFi seguro
    PubSubClient mqttClient; // Cliente MQTT
    WiFiManager wifiManager; // Instancia de WiFiManager
    unsigned long lastReconnectAttempt = 0;
    const unsigned long reconnectInterval = 10 * 1000;
  public:
    // Constructor de MQTTHandler
    MQTTHandler() : mqttClient(wifiClient) {}

    void setup() {
      connectWiFi();
      connectMQTT();
    }

    // Loop para mantener la conexión MQTT
    void loop() {
      if (!mqttClient.connected()) {
        Serial.println("MQTT broker not connected!. Reconnecting"); 
        reconnect();
      }
      mqttClient.loop();
    }

    // Método para publicar un mensaje MQTT en un tópico específico
    void publishMessage(const char* topic, const char* message) {
      mqttClient.publish(topic, message); // Publica el mensaje en el tópico especificado
    }

    // Conectarse al WiFi usando WiFiManager
    void connectWiFi() {

      // Inicia el portal de configuración con nombre y contraseña del AP
      if (!wifiManager.autoConnect("ESP32-Config", "123456789")) {
        Serial.println("No se pudo conectar a la red WiFi. Reiniciando...");
        delay(3000);
        ESP.restart();
      }

      // Verifica si se ha conectado
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to WiFi!");
      } else{
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
      if (mqttClient.connect(MQTT_CLIENT_ID)) { 
        Serial.println(" Connected to MQTT!"); // Mensaje de conexión MQTT exitosa

        delay(100); 
        mqttClient.subscribe(UPDATE_DELTA_TOPIC); // Suscripción al tópico de actualización delta
        Serial.println("Subscribed to " + String(UPDATE_DELTA_TOPIC)); 
        delay(100); 
      } else {
        Serial.println("Failed to connect to MQTT broker!");
      }
    }

    void reconnect() {
      if (currentMillis - lastReconnectAttempt > reconnectInterval) {
        lastReconnectAttempt = currentMillis;
        Serial.println("Intentando reconectar MQTT...");
        if (mqttClient.connect(MQTT_CLIENT_ID)) {
          Serial.println("Reconexión MQTT exitosa");
          mqttClient.subscribe(UPDATE_TOPIC);
        } else {
          Serial.println("Fallo al reconectar MQTT. RC=" + String(mqttClient.state()));
        }
      }
    }
};

// Creamos una instancia de MQTTHandler
MQTTHandler mqttHandler;

void reportWaterSensor() {
  outputDoc["state"]["reported"]["total_used_water"] = totalMilliLitres;
  outputDoc["state"]["reported"]["watering"] = 0;
  outputDoc["state"]["reported"]["area"] = area;
  outputDoc["state"]["reported"]["plant_type"] = plant_type;
  serializeJson(outputDoc, outputBuffer);
  mqttHandler.publishMessage(UPDATE_TOPIC, outputBuffer);
}

void reportWatering() {
  outputDoc["state"]["reported"]["watering"] = solenoidValve.isOpened();
  serializeJson(outputDoc, outputBuffer);
  mqttHandler.publishMessage(UPDATE_TOPIC, outputBuffer);
}

void setup() {
  Serial.begin(115200);

  pinMode(Sensor_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Sensor_PIN), pulseCounter, FALLING);
  
  mqttHandler.setup();

  // Iniciar la NVS
  preferences.begin("my-app", false);

  // Recuperar las variables guardadas, si existen
  totalMilliLitres = preferences.getULong("total_ml", 0);
  WATER_LIMIT = preferences.getInt("water_limit", 0);
  area = preferences.getInt("area", 0);
  plant_type = preferences.getString("plant_type", "");

  if(WATER_LIMIT != 0)
  {
    solenoidValve.open_or_close(true); 
  }
}

void calculateFlowAndPublishData(unsigned long currentMillis) {
  if (WATER_LIMIT != 0 && solenoidValve.isOpened() == true) {
    calculateFlowRate(currentMillis);
    calculateTotalFlow();

    // Guardar las variables actualizadas en la NVS
    preferences.putULong("total_ml", totalMilliLitres);

    // Mostrar los valores actualizados
    Serial.print("totalMilliLitres: ");
    Serial.println(totalMilliLitres);
    Serial.print("WATER_LIMIT: ");
    Serial.println(WATER_LIMIT);
    Serial.print("Area: ");
    Serial.println(area);
    Serial.print("Tipo de planta:");
    Serial.println(plant_type);
    Serial.println("");

    // Verificar si se superó el límite de agua
    if (totalMilliLitres > WATER_LIMIT) {
      reportWaterSensor(); // Reportar a AWS
      solenoidValve.open_or_close(false); // Cerrar la válvula

      // Borramos valores guardados
      preferences.remove("total_ml");
      preferences.remove("water_limit");
      preferences.remove("area");
      preferences.remove("plant_type");
      preferences.end();
      
      while (true) {
        delay(1000);
      }
    }
  } else {
    Serial.println("NO SE ACTUALIZO LOS DATOS");
    delay(2500);
  }
}

void loop() {
  currentMillis = millis();
  
  if (currentMillis - previousMillis >= 1500) {
    mqttHandler.loop();
    calculateFlowAndPublishData(currentMillis);
  }
}