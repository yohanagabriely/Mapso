#include <WiFi.h>
#include <Wire.h> 
#include <Adafruit_MLX90614.h> 
#include "ThingSpeak.h" // Biblioteca oficial

// --- CONFIGURAÇÕES ---
const char* ssid = "DTEL+GABY 2.4G"; 
const char* password = "Gaby6868!"; 
unsigned long myChannelNumber = 3202067;
const char * myWriteAPIKey = "T71IMITM56CA1O35";

const int PINO_SOM_ANALOGICO = 36; 
const int PINO_MERCURIO_DIGITAL = 4; 

Adafruit_MLX90614 mlx;
WiFiClient client;

void setup() {
  Serial.begin(115200);
  pinMode(PINO_MERCURIO_DIGITAL, INPUT_PULLUP);
  Wire.begin();
  mlx.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  ThingSpeak.begin(client); // Inicializa o ThingSpeak
  Serial.println("\nConectado e Pronto!");
}

int readSoundAmplitude() {
    int leituraMinima = 4095, leituraMaxima = 0;
    for (int i = 0; i < 100; i++) {
        int valor = analogRead(PINO_SOM_ANALOGICO);
        if (valor < leituraMinima) leituraMinima = valor;
        if (valor > leituraMaxima) leituraMaxima = valor;
    }
    return leituraMaxima - leituraMinima;
}

void loop() {
  // Coleta dados
  int som = readSoundAmplitude();
  float temp = mlx.readObjectTempC();
  int impacto = (digitalRead(PINO_MERCURIO_DIGITAL) == LOW) ? 1 : 0;

  // Envia para os campos do ThingSpeak
  ThingSpeak.setField(1, som);
  ThingSpeak.setField(2, temp);
  ThingSpeak.setField(3, impacto);

  // Escreve no canal
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

  if(x == 200){
    Serial.println("Canais atualizados com sucesso no ThingSpeak.");
  } else {
    Serial.println("Erro na atualização. HTTP error code " + String(x));
  }

  // O ThingSpeak gratuito aceita atualizações a cada 15 segundos
  delay(15000); 
}