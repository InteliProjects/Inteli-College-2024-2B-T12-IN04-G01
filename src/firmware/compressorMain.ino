// Inclui as bibliotecas necessárias

#include <Wire.h> // Inclui a biblioteca Wire para comunicação I2C
#include <Adafruit_Sensor.h> // Inclui a biblioteca Adafruit Sensor para sensores
#include <Adafruit_BME280.h> // Inclui a biblioteca Adafruit BME280 para o sensor BME280
#include <LiquidCrystal_I2C.h> // Inclui a biblioteca LiquidCrystal_I2C para controle de displays LCD via I2C
#include "UbidotsEsp32Mqtt.h" // Inclui a biblioteca UbidotsEsp32Mqtt para comunicação com a plataforma 

// ========================= DECLARAÇÕES GLOBAIS E CONSTANTES =========================

const char *UBIDOTS_TOKEN = "BBUS-veCoBVrAsiykWhV0az7GqtT7AnbTQx"; // Token de autenticação do Ubidots
const char *WIFI_SSID = "Inteli.Iot";     // Nome da rede Wi-Fi
const char *WIFI_PASS = "@Intelix10T#";   // Senha da rede Wi-Fi

const char *DEVICE_LABEL = "esp32_t12_g01"; // Nome do dispositivo no Ubidots
const char *TEMP_ATUAL = "temperatura"; // Variável para temperatura
const char *MAGNITUDE_ATUAL = "magnitude"; // Variável para magnitude
const char *TEMPO_DE_FUNCIONAMENTO = "tempo_de_funcionamento"; // Variável para tempo de funcionamento
const char *STATUS_COMPRESSOR = "status_compressor"; // Variável para status do compressor
const char *CLIENT_ID = "id_do_cliente_compressor"; // ID do cliente para conexão com o Ubidots


// Instância do objeto Ubidots para enviar dados à plataforma
Ubidots ubidots(UBIDOTS_TOKEN, CLIENT_ID);

// ========================= DEFINIÇÕES DO HARDWARE =========================
// Endereço do sensor BME280 e pinos dos componentes
#define BME280_ADDRESS 0x76 // pino do bme
#define BUTTON_PIN 34 // pino do botão
#define BUTTON_PIN_BUZZER 18 // pino do botão do buzzer 
#define BUZZER_PIN 4 // pino do buzzer
#define ACCEL_ADDRESS 0x1C // endereço do acelerômetro
#define LED_GREEN 12 // pino do LED verde
#define LED_RED 27 // pino do LED vermelho
#define INTERVAL 1000 // Intervalo de atualização em milissegundos
#define TEMPERATURE_THRESHOLD 28.0 // Limite de temperatura

// Instâncias dos componentes de hardware
Adafruit_BME280 bme; // Cria um objeto do sensor BME280
LiquidCrystal_I2C lcd(0x27, 16, 2); // Cria um objeto LCD com endereço I2C e dimensões definidas

// Gerenciamento de LEDs

class LEDManager {
private:
    uint8_t pinRed, pinGreen; // Pinos dos LEDs

public:
    LEDManager(uint8_t redPin, uint8_t greenPin) : pinRed(redPin), pinGreen(greenPin) {
        pinMode(pinRed, OUTPUT); // Configura pino do LED vermelho como saída
        pinMode(pinGreen, OUTPUT); // Configura pino do LED verde como saída
    }

    void setNormal() {
        digitalWrite(pinGreen, HIGH); // Liga o LED verde
        digitalWrite(pinRed, LOW);   // Desliga o LED vermelho
    }

    void setHighTemp() {
        digitalWrite(pinGreen, LOW); // Desliga o LED verde
        digitalWrite(pinRed, HIGH);  // Liga o LED vermelho
    }

    void turnOff() {
        digitalWrite(pinGreen, LOW); // Desliga ambos os LEDs
        digitalWrite(pinRed, LOW);
    }
};

// Classe para gerenciar os sensores (temperatura e vibração)
class SensorManager {
private:

    float lowVibrationThreshold = 2506; // Limite inferior inicial de vibração
    float highVibrationThreshold = 2522; // Limite superior inicial de vibração

    float minMagnitude = 4096.0; // Inicializa com o maior valor possível
    float maxMagnitude = 0.0;    // Inicializa com o menor valor possível
    unsigned long calibrationStartTime = 0; // Tempo inicial da calibração
    bool calibrationDone = false; // Flag de calibração concluída


public:
    SensorManager() {}

    float getTemperature() {
        return bme.readTemperature(); // Lê a temperatura do sensor BME280
    }

    void calibrateThresholds() {
        if (millis() - calibrationStartTime < 15000) { // Calibração por 15 segundos
            float magnitude = getVibrationMagnitude(); // Obtém a magnitude da vibração

            // Atualiza valores mínimo e máximo durante a calibração
            if (magnitude < minMagnitude) minMagnitude = magnitude; // Atualiza mínimo
            if (magnitude > maxMagnitude) maxMagnitude = magnitude; // Atualiza máximo

            Serial.print("Calibrando... Magnitude: "); // Exibe informações no Serial Monitor
            Serial.print(magnitude); // Exibe a magnitude atual
            Serial.print(" | Min: "); // Exibe os valores mínimo e máximo
            Serial.print(minMagnitude); // Exibe o valor mínimo
            Serial.print(" | Max: "); // Exibe o valor máximo
            Serial.println(maxMagnitude); // Exibe o valor máximo
        } else {
            // Define thresholds após calibração
            lowVibrationThreshold = max(minMagnitude - 15.0, 0.0); // Define o limite inferior
            highVibrationThreshold = min(maxMagnitude + 15.0, 4096.0); // Define o limite superior
            calibrationDone = true; // Marca a calibração como concluída

            Serial.println("Calibração concluída!"); // Exibe mensagem de conclusão
            Serial.print("Low Threshold: "); // Exibe os valores de threshold
            Serial.println(lowVibrationThreshold); // Exibe o limite inferior
            Serial.print("High Threshold: "); // Exibe o limite superior
            Serial.println(highVibrationThreshold); // Exibe o limite superior
        }
    }

    float getVibrationMagnitude() { // Obtém a magnitude da vibração
      
        unsigned int data[7]; // Array para armazenar os dados do acelerômetro
        Wire.requestFrom(ACCEL_ADDRESS, 7); // Solicita dados do acelerômetro

        if (Wire.available() == 7) { // Se os dados estiverem disponíveis
            for (int i = 0; i < 7; i++) // Lê os dados do acelerômetro
                data[i] = Wire.read(); // Armazena os dados no array
        } else {
            Serial.println("Erro: Dados incompletos do acelerômetro"); // Exibe mensagem de erro
            return 0.0; // retorna 0
        }

        int xAccl = ((data[1] * 256) + data[2]) / 16; // Calcula a aceleração em x
        int yAccl = ((data[3] * 256) + data[4]) / 16; // Calcula a aceleração em y
        int zAccl = ((data[5] * 256) + data[6]) / 16; // Calcula a aceleração em z

        if (xAccl > 2047) xAccl -= 4096; // Converte valores para inteiros
        if (yAccl > 2047) yAccl -= 4096; // Converte valores para inteiros
        if (zAccl > 2047) zAccl -= 4096;  // Converte valores para inteiros

        float magnitude = sqrt(xAccl * xAccl + yAccl * yAccl + zAccl * zAccl); // Calcula a magnitude
        return magnitude; // Retorna a magnitude
    }

    bool detectVibration() {
        static unsigned long lastTimeInRange = 0; // Último tempo dentro da faixa de repouso
        static bool isMachineOn = true; // Estado atual da máquina

        // Se a calibração ainda não estiver pronta, continua calibrando
        if (!calibrationDone) {
            calibrateThresholds();
            return true; // Assume que a máquina está ativa durante a calibração
        }

        float magnitude = getVibrationMagnitude(); // Obtém a magnitude da vibração
        Serial.print("Magnitude da vibração: "); // Exibe a magnitude no Serial Monitor
        Serial.print(magnitude); // Exibe a magnitude
        Serial.print(" | Thresholds: ["); // Exibe os thresholds
        Serial.print(lowVibrationThreshold); // Exibe o limite inferior
        Serial.print(", "); // Exibe a vírgula
        Serial.print(highVibrationThreshold); // Exibe o limite superior
        Serial.println("]"); // Exibe o colchete de fechamento

        // Lógica de detecção de vibração
        if (magnitude >= lowVibrationThreshold && magnitude <= highVibrationThreshold) {
            if (millis() - lastTimeInRange > 3000) {
                isMachineOn = false; // Considera desligado após 3 segundos na faixa
            }
        } else {
            lastTimeInRange = millis(); // Reseta o tempo de repouso
            isMachineOn = true; // Vibração fora da faixa → máquina ligada
        }

        return isMachineOn; // Retorna o estado da máquina
    }

    void startCalibration() { // Inicia a calibração dos thresholds
        calibrationStartTime = millis(); // Inicia a contagem do tempo
        calibrationDone = false; // Marca a calibração como não concluída
        minMagnitude = 4096.0; // Reseta o valor mínimo
        maxMagnitude = 0.0; // Reseta o valor máximo
        Serial.println("Iniciando calibração de thresholds...");  // Exibe mensagem de início
        lcd.print("Calibrando vibracao :15sec"); // Exibe uma informação de erro no display LCD
    }
};


// Variáveis globais e instâncias dos gerenciadores de LEDs e sensores
LEDManager leds(LED_RED, LED_GREEN);
SensorManager sensors;

unsigned long previousMillis = 0; // Armazena o tempo anterior para atualizações periódicas
bool buttonPressed = false;

// Variáveis do LCD e tempo de funcionamento
float runTime = 0.0;
uint8_t screenIndex = 0; // Índice da tela ativa
int8_t lastScreenIndex = -1; // Rastreia a última tela exibida

bool lcdOn = true; // Indica se o LCD está ligado
unsigned long lastUpdateTimeLCD = 0;

const unsigned long updateIntervalLCD = 2000;


// Função de callback para tratar mensagens recebidas via MQTT
void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived ["); // Inicia a mensagem no Serial Monitor
    Serial.print(topic); // Exibe o tópico da mensagem
    Serial.print("] "); // Exibe o colchete de fechamento

    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]); // Exibe o conteúdo da mensagem recebida
    }


    Serial.println(); // Pula para a próxima linha após exibir a mensagem
}

// Inicializa o acelerômetro configurando os registros necessários.
void initializeAccelerometer() {
    Wire.beginTransmission(ACCEL_ADDRESS); // Inicia a comunicação com o acelerômetro
    Wire.write(0x2A); // Escreve no registro de controle
    Wire.write(0x00); // Configuração do acelerômetro
    Wire.endTransmission(); // Finaliza a transmissão

    Wire.beginTransmission(ACCEL_ADDRESS); // Inicia a comunicação com o acelerômetro
    Wire.write(0x2A); // Escreve no registro de controle
    Wire.write(0x01); // Configuração do acelerômetro
    Wire.endTransmission();  // Finaliza a transmissão

    Wire.beginTransmission(ACCEL_ADDRESS); // Inicia a comunicação com o acelerômetro
    Wire.write(0x0E); // Escreve no registro de controle
    Wire.write(0x00); // Configuração do acelerômetro
    Wire.endTransmission(); // Finaliza a transmissão
}

// Função de configuração inicial do sistema.
void setup() {
    Wire.begin(21, 22); // Inicia a comunicação I2C nos pinos 21 (SDA) e 22 (SCL)
    Serial.begin(115200); // Inicia a comunicação serial com baud rate de 115200

    pinMode(BUTTON_PIN, INPUT_PULLUP); // Configura o pino do botão como entrada com resistor pull-up
    pinMode(BUTTON_PIN_BUZZER, INPUT_PULLUP); // Configura o pino do botão do buzzer como entrada com resistor pull-up
    pinMode(BUZZER_PIN, OUTPUT); // Configura o pino do buzzer como saída

   lcd.begin(16, 2); // Inicializa o LCD com 16 colunas e 2 linhas
   lcd.backlight(); // Liga a luz de fundo do LCD
   lcd.clear(); // Limpa o display
   leds.setNormal(); // Liga o LED verde

   if (!bme.begin(BME280_ADDRESS)) { // Inicializa o sensor BME280
       Serial.println("Falha ao inicializar o BME280!");

       lcd.print("Erro: Falha ao inicializar o BME280!"); // Exibe uma informação de erro no display LCD
       while (1) ; // Entra em loop infinito em caso de falha
   }

   sensors.startCalibration(); // Inicia a calibração dos thresholds

   initializeAccelerometer(); // Inicializa o acelerômetro

   ubidots.setDebug(true); // Ativa mensagens de depuração se necessário.
   ubidots.connectToWifi(WIFI_SSID, WIFI_PASS); // Conecta à rede Wi-Fi definida
   ubidots.setCallback(callback); // Define a função de callback para mensagens recebidas
   ubidots.setup(); // Configura a conexão MQTT com o Ubidots
   ubidots.reconnect(); // Tenta reconectar ao Ubidots caso a conexão seja perdida

}

// Variáveis para armazenar leituras dos sensores.
float currentTemperature = 0.0; // Variável para armazenar a temperatura atual
float currentMagnitude = 0.0; // Variável para armazenar a magnitude da vibração

// Função principal que roda continuamente após a configuração inicial.
void loop() {
   checkButtonState(); // Verifica o estado do botão

   if (!ubidots.connected()) ubidots.reconnect(); // Reconecta ao Ubidots se a conexão for perdida

   unsigned long currentMillis = millis(); // Obtém o tempo atual em milissegundos

   if (currentMillis - previousMillis >= INTERVAL) { // Atualiza a cada INTERVAL milissegundos
       previousMillis = currentMillis; // Atualiza o tempo anterior

       checkTemperatureAndVibration(&currentTemperature, &currentMagnitude); // Verifica temperatura e vibração
       updateRunTime(&runTime); // Atualiza o tempo de funcionamento
       publishData(currentTemperature, currentMagnitude, runTime); // Publica os dados no Ubidots
   }

   if (screenIndex != lastScreenIndex) { // Atualiza o display LCD
       lastScreenIndex = screenIndex; // Atualiza o índice da tela
   }

   if (millis() - lastUpdateTimeLCD >= updateIntervalLCD) { // Atualiza o display LCD
       lastUpdateTimeLCD = millis(); // Atualiza o tempo da última atualização

       if (lcdOn) { // Se o LCD estiver ligado
           lcd.clear(); // Limpa o display

           switch (screenIndex) { // Alterna entre as telas
                // Exibe a temperatura no LCD
               case 0: // Caso 0
                   lcd.setCursor(0, 0); // Define o cursor na posição (0, 0)
                   lcd.print("Temperatura:"); // Exibe a mensagem "Temperatura:"
                   lcd.setCursor(0, 1); // Define o cursor na posição (0, 1)
                   lcd.print(sensors.getTemperature()); // Exibe a temperatura atual
                   lcd.print(" C"); // Exibe a unidade de temperatura
                   break; // Encerra o case

                // Exibe o tempo de operação no LCD
               case 1: // Caso 1
                   lcd.setCursor(0, 0); // Define o cursor na posição (0, 0)
                   lcd.print("Tempo Ligado:"); // Exibe a mensagem "Tempo Ligado:"
                   lcd.setCursor(0, 1); // Define o cursor na posição (0, 1)
                   lcd.print(runTime / 3600.0); // Exibe o tempo de funcionamento em horas
                   lcd.print(" horas"); // Exibe a unidade de tempo
                   break; // Encerra o case

                // Exibe o status da máquina no LCD
               case 2:
                   lcd.setCursor(0, 0); // Define o cursor na posição (0, 0)
                   lcd.print("Status:"); // Exibe a mensagem "Status:"
                   lcd.setCursor(0, 1); // Define o cursor na posição (0, 1)

                   if (digitalRead(LED_GREEN) == HIGH) { // Se o LED verde estiver ligado
                       lcd.print("OK"); // Exibe a mensagem "OK"
                   } else if (digitalRead(LED_RED) == HIGH) { // Se o LED vermelho estiver ligado
                       lcd.print("Manutencao"); // Exibe a mensagem "Manutencao"
                   } else {
                       lcd.print("Desligado"); // Exibe a mensagem "Desligado"
                   }
                   break; // Encerra o case

                // Exibe a vibração no LCD
               case 3:
                   lcd.setCursor(0, 0); // Define o cursor na posição (0, 0)
                   lcd.print("Vibracao:"); // Exibe a mensagem "Vibracao:"
                   lcd.setCursor(0, 1); // Define o cursor na posição (0, 1)

                   if (sensors.detectVibration()) { // Se a vibração for detectada
                       lcd.print("Ativa"); // Exibe a mensagem "Ativa"
                   } else { // Se a vibração não for detectada
                       lcd.print("Inativa"); // Exibe a mensagem "Inativa"
                   }
                   break; // Encerra o case
           }
       }
   }

   ubidots.loop(); // Mantém a conexão com o Ubidots
}

// Função para verificar temperatura e magnitude da vibração.
void checkTemperatureAndVibration(float *temperature, float *magnitude) { // Verifica temperatura e vibração
   *temperature = sensors.getTemperature(); // Obtém a temperatura do sensor BME280
   *magnitude = sensors.getVibrationMagnitude(); // Obtém a magnitude da vibração

   Serial.print("Temperatura atual: ");
   Serial.print(*temperature); // Essas informações são exibidas no dashboard e no display LCD
   Serial.println(" C"); // Exibe a unidade de temperatura

   bool isOn = sensors.detectVibration(); // Verifica se a vibração foi detectada
   publishStatusData(isOn); // Publica o status do compressor no Ubidots

   if (isOn) { // Se a vibração for detectada
       if (*temperature >= TEMPERATURE_THRESHOLD)  { // Se a temperatura for maior que o limite
           leds.setHighTemp(); // Liga o LED vermelho
           Serial.println("Temperatura alta! LED vermelho aceso.");
           lcd.clear(); // Limpa o display LCD
           lcd.print("Temperatura alta!");
           tone(BUZZER_PIN, 2000); // Emite um som de alerta
       } else {
           leds.setNormal(); // Liga o LED verde
           Serial.println("Temperatura normal. LED verde aceso.");
           noTone(BUZZER_PIN); // Desliga o som do buzzer
       }
   } else {
       leds.turnOff(); // Desliga ambos os LEDs
       Serial.println("Sem vibração detectada. LEDs apagados.");
       noTone(BUZZER_PIN); // Desliga o som do buzzer
   }
}

// Atualiza o tempo de funcionamento baseado na detecção de vibração.
void updateRunTime(float *runTime) {
   if (sensors.detectVibration()) {
       *runTime += (float)INTERVAL / 1000.0; // run time em segundos
   }
}

// Publica os dados coletados no Ubidots.
void publishData(float temperature, float magnitude, unsigned long runTime) {
   ubidots.add(TEMP_ATUAL, temperature); // Adiciona a temperatura ao payload
   ubidots.add(MAGNITUDE_ATUAL, magnitude); // Adiciona a magnitude ao payload
   ubidots.add(TEMPO_DE_FUNCIONAMENTO, runTime / 3600.0); // envia run time em horas
   ubidots.publish(DEVICE_LABEL); // Publica os dados no Ubidots
}

// Publica dados sobre o status do compressor no Ubidots.
void publishStatusData(bool isOn) { // Publica o status do compressor no Ubidots
   ubidots.add(STATUS_COMPRESSOR, isOn);
   ubidots.publish(DEVICE_LABEL); // Publica os dados no Ubidots
}

// Função para verificar o estado do botão.
void checkButtonState() { // Verifica o estado do botão
   if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
       buttonPressed = true; // Marca o botão como pressionado
       screenIndex = (screenIndex + 1) % 4;
   } else if (digitalRead(BUTTON_PIN) == HIGH && buttonPressed) {
       buttonPressed = false; // Marca o botão como não pressionado
   }
}