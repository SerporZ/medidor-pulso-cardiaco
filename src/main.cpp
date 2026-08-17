#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

// --- CONFIGURACIÓN DE RED Y MQTT ---
const char* ssid = "TU_RED_WIFI";       // Pon tu red aquí
const char* password = "TU_CLAVE_WIFI"; // Pon tu clave aquí

// Dirección IP de tu computadora (Broker Mosquitto)
const char* mqtt_server = "IP_DEL_BROKER_MQTT"; 
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// --- CONFIGURACIÓN DE LA PANTALLA OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define DIRECCION_I2C 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int graficaOnda[SCREEN_WIDTH]; 

// --- CONFIGURACIÓN DEL SENSOR Y ALGORITMO ---
const int sensorPin = 0; 
int valorSensor = 0;
const int buzzerPin = 2; // Pin para la alarma sonora
int umbral = 100; // Ajusta este valor según tu sensor y condiciones de iluminación
bool picoDetectado = false;
unsigned long ultimoLatido = 0;
int bpm = 0;
float senalFiltrada = 0; // Memoria para el filtro digital
// Función para conectar al Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado.");
  Serial.println("Dirección IP del ESP32: ");
  Serial.println(WiFi.localIP());
}

// Función para reconectar a Mosquitto si se pierde la conexión
void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    // Creamos un ID de cliente aleatorio
    String clientId = "ESP32_Medico_";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("¡Conectado al Broker Mosquitto!");
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(client.state());
      Serial.println(" Intentando de nuevo en 5 segundos...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW); // Asegurarnos de que inicie en silencio
  delay(50); 
  
  analogReadResolution(12);
  Wire.begin(4, 5); // Tus pines I2C del ESP32-C3
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, DIRECCION_I2C)) {
    Serial.println(F("ERROR CRÍTICO: Fallo en la OLED."));
    for(;;); 
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Iniciando...");
  display.display();
  
  for (int i = 0; i < SCREEN_WIDTH; i++) {
    graficaOnda[i] = SCREEN_HEIGHT; 
  }

  // Iniciamos la conexión Wi-Fi y configuramos el Broker
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}
// Variables para el auto-escalado de la pantalla (agregadas automáticamente)
int oledMax = 300;
int oledMin = 100;

void loop() {
  // Mantener la conexión MQTT viva
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 1. Leemos el dato crudo
  int valorCrudo = analogRead(sensorPin);
  
  // 2. FILTRO DIGITAL PASO BAJO (50/50)
  senalFiltrada = (0.50 * senalFiltrada) + (0.50 * valorCrudo);
  valorSensor = (int)senalFiltrada;

  // 3. Publicamos la onda por MQTT para Python
  String ondaString = String(valorSensor);
  client.publish("proyecto/medidor/onda", ondaString.c_str());
  
  // --- ALGORITMO DE BPM CORREGIDO ---
  if (valorSensor > umbral && picoDetectado == false) {
    picoDetectado = true;
    
    // 🔊 ENCIENDE EL BUZZER AL INICIO DEL LATIDO
    digitalWrite(buzzerPin, HIGH); 

    unsigned long tiempoActual = millis();
    unsigned long tiempoEntreLatidos = tiempoActual - ultimoLatido;

    if (tiempoEntreLatidos > 300) { 
      int nuevoBpm = 60000 / tiempoEntreLatidos;
      ultimoLatido = tiempoActual;
      
      if (nuevoBpm > 40 && nuevoBpm < 160) {
        bpm = nuevoBpm; 
        String bpmString = String(bpm);
        client.publish("proyecto/medidor/bpm", bpmString.c_str());
      }
    }
  }
  
  if (valorSensor < umbral) {
    picoDetectado = false;
    
    // 🔇 APAGA EL BUZZER CUANDO LA ONDA VUELVE A BAJAR
    digitalWrite(buzzerPin, LOW); 
  }

  // --- LÓGICA GRÁFICA (OLED) CON AUTO-ESCALADO ---
  for (int i = 0; i < SCREEN_WIDTH - 1; i++) {
    graficaOnda[i] = graficaOnda[i + 1];
  }

  // Algoritmo que rastrea los picos máximos y mínimos en tiempo real
  if (valorSensor > oledMax) oledMax = valorSensor;
  if (valorSensor < oledMin) oledMin = valorSensor;
  
  // Decaimiento lento para que la gráfica se adapte si quitas el dedo
  oledMax -= 1;
  oledMin += 1;
  
  // Mapeo dinámico: la onda siempre se ajustará al tamaño de la pantalla
  int valorMapeado = map(valorSensor, oledMin, oledMax, SCREEN_HEIGHT - 1, 16);
  if (valorMapeado < 16) valorMapeado = 16;
  if (valorMapeado >= SCREEN_HEIGHT) valorMapeado = SCREEN_HEIGHT - 1;
  
  graficaOnda[SCREEN_WIDTH - 1] = valorMapeado;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.print("BPM: ");
  if (bpm > 0) {
    display.print(bpm);
  } else {
    display.print("--"); 
  }

  display.drawLine(0, 15, SCREEN_WIDTH, 15, SSD1306_WHITE);

  for (int i = 0; i < SCREEN_WIDTH - 1; i++) {
    display.drawLine(i, graficaOnda[i], i + 1, graficaOnda[i + 1], SSD1306_WHITE);
  }
  display.display();
  
  delay(15); 
}