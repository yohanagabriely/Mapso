#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h> 
#include <Adafruit_MLX90614.h>
#include <time.h> // Necessário para a função time()

// =======================================================
// === CONFIGURAÇÕES DE REDE E PINOS ===
// =======================================================
const char* ssid = "DTEL+GABY 2.4G";
const char* password = "Gaby6868!";
const char* ntpServer = "pool.ntp.org";

// Pinos dos Sensores
#define SOUND_PIN 34 // Pino Analógico
#define IMPACT_PIN 4  // Pino Digital

WebServer server(80);
Adafruit_MLX90614 mlx = Adafruit_MLX90614(); 

// =======================================================
// === FUNÇÕES DE LEITURA ===
// =======================================================

float readTemperatureSensor() {
    // Retorna a temperatura do objeto em Celsius do GY-906
    return mlx.readObjectTempC();
}

bool readImpact() {
    static unsigned long lastTrigger = 0;
    // O KY-027 é geralmente ativo baixo (LOW), então usamos !digitalRead
    bool impact = !digitalRead(IMPACT_PIN); 

    if (impact) {
        // Debounce simples para estabilizar a leitura
        if (millis() - lastTrigger > 150) {
            lastTrigger = millis();
            return true;
        }
    }
    return false;
}

// =======================================================
// === API /data (COM TIMESTAMP) ===
// =======================================================
void handleSensorData() {
    // 1. LÊ OS SENSORES
    int sound_level = analogRead(SOUND_PIN);
    float temperature = readTemperatureSensor(); 
    bool impact_detected = readImpact();

    // 2. OBTÉM O TIMESTAMP (milissegundos desde 1970)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    // Converte segundos e microssegundos para milissegundos (long long)
    long long timestamp_ms = (long long)tv.tv_sec * 1000LL + (long long)tv.tv_usec / 1000LL;

    // 3. FORMATA JSON
    String json = "{";
    json += "\"sound\": " + String(sound_level) + ",";
    json += "\"temp\": " + String(temperature, 2) + ",";
    json += "\"impact\": ";
    json += (impact_detected ? "true" : "false");
    json += ",\"timestamp\": " + String(timestamp_ms); // Adiciona o timestamp
    json += "}";

    // 4. ENVIA RESPOSTA com cabeçalho CORS
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Content-Type", "application/json");
    server.send(200, "application/json", json);
}

// =======================================================
// === SETUP E LOOP ===
// =======================================================
void setup() {
    Serial.begin(115200); 
    pinMode(SOUND_PIN, INPUT);
    pinMode(IMPACT_PIN, INPUT_PULLUP); // INPUT_PULLUP para o sensor digital

    Serial.println("\n--- Inicializando Sensores e WiFi ---");

    // Inicia I2C e o sensor MLX90614
    Wire.begin();
    if (!mlx.begin()) {
        Serial.println("ERRO: MLX90614 (GY-906) não encontrado! Verifique a fiação I2C.");
        while (1); // Trava se o sensor falhar
    }
    Serial.println("Sensor MLX90614 inicializado.");

    // Tenta Conexão WiFi 
    Serial.print("Conectando-se ao WiFi: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    int maxTries = 60; // 30 segundos de timeout
    while (WiFi.status() != WL_CONNECTED && maxTries-- > 0) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi conectado!");
        
        // Configura e sincroniza o tempo NTP (fuso 0, sem horário de verão)
        configTime(0, 0, ntpServer); 
        Serial.println("Tempo sincronizado via NTP.");
        
        Serial.print("Servidor de API rodando em: http://");
        Serial.print(WiFi.localIP());
        Serial.println("/data");
    } else {
        Serial.println("\nFalha ao conectar no WiFi! Verifique credenciais.");
        while(1); // Trava se falhar
    }

    // Configuração das rotas do servidor web
    server.on("/", [](){ server.send(200, "text/plain", "API de Sensores Ativa em /data"); });
    server.on("/data", handleSensorData); 
    server.begin();
}

void loop() {
    server.handleClient();
}