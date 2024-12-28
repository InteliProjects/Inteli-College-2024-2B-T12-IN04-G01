// Inclui as bibliotecas necessárias para o funcionamento do projeto
#include <Wire.h>                     // Biblioteca para comunicação I2C
#include <Adafruit_Sensor.h>          // Biblioteca base para sensores Adafruit
#include <Adafruit_BME280.h>          // Biblioteca para o sensor de temperatura, pressão e umidade BME280
#include <LiquidCrystal_I2C.h>        // Biblioteca para controle de display LCD via I2C
#include <cmath>                      // Biblioteca para funções matemáticas (ex: sqrt)
#include "UbidotsEsp32Mqtt.h"         // Biblioteca para integração com a plataforma Ubidots via MQTT

// Definições Ubidots
const char *UBIDOTS_TOKEN = "BBUS-veCoBVrAsiykWhV0az7GqtT7AnbTQx"; // Token de autenticação do Ubidots
const char *WIFI_SSID = "Inteli.Iot";                              // Nome da rede Wi-Fi
const char *WIFI_PASS = "@Intelix10T#";                           // Senha da rede Wi-Fi
const char *DEVICE_LABEL = "esp32_t12_g01";                       // Nome do dispositivo no Ubidots
const char *TEMP_ATUAL = "temperatura";                           // Nome da variável de temperatura no Ubidots
const char *MAGNITUDE_ATUAL = "magnitude";                        // Nome da variável de magnitude no Ubidots

Ubidots ubidots(UBIDOTS_TOKEN); // Cria uma instância da biblioteca Ubidots com o token fornecido

// Definições do hardware
#define BME280_ADDRESS 0x76      // Endereço I2C do sensor BME280
#define BUTTON_PIN 34            // Pino digital conectado ao botão
#define ACCEL_ADDRESS 0x1C       // Endereço I2C do acelerômetro (assumido)
#define LED_GREEN 12             // Pino digital conectado ao LED verde
#define LED_RED 27               // Pino digital conectado ao LED vermelho
#define INTERVAL 1000            // Intervalo de execução do loop (em milissegundos)
#define TEMPERATURE_THRESHOLD 28.0 // Limite de temperatura para ativar alerta (em graus Celsius)

// Instâncias dos sensores e do display
Adafruit_BME280 bme;                    // Cria um objeto para o sensor BME280
LiquidCrystal_I2C lcd(0x27, 16, 2);     // Cria um objeto para o display LCD com endereço I2C 0x27 e dimensões 16x2

// Classe para gerenciar o estado dos LEDs
class LEDManager {
private:
    int pinRed, pinGreen; // Pinos dos LEDs vermelho e verde

public:
    // Construtor da classe LEDManager que recebe os pinos dos LEDs
    LEDManager(int redPin, int greenPin) : pinRed(redPin), pinGreen(greenPin) {
        pinMode(pinRed, OUTPUT);   // Configura o pino do LED vermelho como saída
        pinMode(pinGreen, OUTPUT); // Configura o pino do LED verde como saída
    }

    // Método para acender o LED verde e apagar o vermelho (estado normal)
    void setNormal() {
        digitalWrite(pinGreen, HIGH); // Acende o LED verde
        digitalWrite(pinRed, LOW);    // Apaga o LED vermelho
    }

    // Método para acender o LED vermelho e apagar o verde (temperatura alta)
    void setHighTemp() {
        digitalWrite(pinGreen, LOW);  // Apaga o LED verde
        digitalWrite(pinRed, HIGH);   // Acende o LED vermelho
    }

    // Método para apagar ambos os LEDs
    void turnOff() {
        digitalWrite(pinGreen, LOW);  // Apaga o LED verde
        digitalWrite(pinRed, LOW);    // Apaga o LED vermelho
    }
};

// Classe para gerenciar os sensores de temperatura e vibração
class SensorManager {
private:
    float lowVibrationThreshold = 970;  // Limite inferior de vibração para detectar movimento
    float highVibrationThreshold = 1010; // Limite superior de vibração para detectar movimento

public:
    // Construtor da classe SensorManager
    SensorManager() {}

    // Método para obter a temperatura atual do sensor BME280
    float getTemperature() {
        return bme.readTemperature(); // Retorna a temperatura em graus Celsius
    }

    // Método para calcular e retornar a magnitude da vibração baseada nos dados do acelerômetro
    float getVibrationMagnitude() {
        unsigned int data[7];                   // Buffer para armazenar os dados lidos do acelerômetro
        Wire.requestFrom(ACCEL_ADDRESS, 7);     // Solicita 7 bytes de dados do acelerômetro

        // Verifica se todos os 7 bytes foram recebidos
        if (Wire.available() == 7) {
            for (int i = 0; i < 7; i++) 
                data[i] = Wire.read();           // Lê os dados recebidos e armazena no buffer
        }
        else {
            Serial.println("Erro: Dados incompletos do acelerômetro"); // Mensagem de erro no Serial Monitor
            return 0.0;                          // Retorna 0.0 indicando erro na leitura
        }

        // Processamento dos dados recebidos para obter as acelerações nos eixos X, Y e Z
        int xAccl = ((data[1] * 256) + data[2]) / 16; // Calcula a aceleração no eixo X
        int yAccl = ((data[3] * 256) + data[4]) / 16; // Calcula a aceleração no eixo Y
        int zAccl = ((data[5] * 256) + data[6]) / 16; // Calcula a aceleração no eixo Z

        // Ajuste para valores negativos se necessário (assumindo representação em complemento de dois)
        if (xAccl > 2047) xAccl -= 4096;            // Ajusta a aceleração no eixo X para valores negativos
        if (yAccl > 2047) yAccl -= 4096;            // Ajusta a aceleração no eixo Y para valores negativos
        if (zAccl > 2047) zAccl -= 4096;            // Ajusta a aceleração no eixo Z para valores negativos

        // Calcula a magnitude total da aceleração
        float magnitude = sqrt(xAccl * xAccl + yAccl * yAccl + zAccl * zAccl); // Calcula a magnitude vetorial da aceleração
        return magnitude;                          // Retorna a magnitude calculada
    }

    // Método para detectar vibração com base na magnitude calculada
    bool detectVibration() {
        static unsigned long lastTimeInRange = 0; // Última vez que a vibração estava no intervalo de descanso
        static bool isMachineOn = true;           // Estado atual da máquina (ligada/desligada)

        float magnitude = getVibrationMagnitude(); // Obtém a magnitude da vibração
        Serial.print("Magnitude da vibração: ");   // Exibe a magnitude no Serial Monitor
        Serial.println(magnitude);

        // Verifica se a magnitude está no intervalo de descanso
        if (magnitude >= lowVibrationThreshold && magnitude <= highVibrationThreshold) {
            // Se estiver no intervalo, verifica por quanto tempo permanece
            if (millis() - lastTimeInRange > 3000) { // Ajuste o tempo conforme necessário (e.g., 3000 ms)
                isMachineOn = false;                  // Considera a máquina desligada após 3 segundos sem vibração
            }
        }
        else {
            // Reinicia o timer e o estado se a magnitude estiver fora do intervalo
            lastTimeInRange = millis();               // Atualiza o tempo do último movimento
            isMachineOn = true;                       // Considera a máquina ligada
        }

        return isMachineOn;                           // Retorna o estado da máquina (ligada/desligada)
    }
};

// Função de callback para mensagens recebidas via MQTT do Ubidots
void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Mensagem recebida [");         // Inicia a mensagem no Serial Monitor
    Serial.print(topic);                         // Exibe o tópico da mensagem
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);           // Exibe o conteúdo da mensagem recebida
    }
    Serial.println();                            // Pula para a próxima linha após exibir a mensagem
}

// Função para inicializar o acelerômetro configurando os registradores necessários
void initializeAccelerometer() {
    Wire.beginTransmission(ACCEL_ADDRESS);          // Inicia a transmissão I2C com o endereço do acelerômetro
    Wire.write(0x2A); Wire.write(0x00); Wire.endTransmission(); // Escreve 0x00 no registrador 0x2A e finaliza a transmissão
    Wire.beginTransmission(ACCEL_ADDRESS);          // Inicia outra transmissão I2C com o endereço do acelerômetro
    Wire.write(0x2A); Wire.write(0x01); Wire.endTransmission(); // Escreve 0x01 no registrador 0x2A e finaliza a transmissão
    Wire.beginTransmission(ACCEL_ADDRESS);          // Inicia mais uma transmissão I2C com o endereço do acelerômetro
    Wire.write(0x0E); Wire.write(0x00); Wire.endTransmission(); // Escreve 0x00 no registrador 0x0E e finaliza a transmissão
}

// Função de configuração inicial
void setup() {
    Wire.begin(21, 22);                            // Inicializa a comunicação I2C nos pinos 21 (SDA) e 22 (SCL)
    Serial.begin(115200);                          // Inicializa a comunicação serial a 115200 bps
    pinMode(BUTTON_PIN, INPUT_PULLUP);             // Configura o pino do botão como entrada com resistor pull-up interno

    // Inicializa o sensor BME280 no endereço especificado
    if (!bme.begin(BME280_ADDRESS)) {
        Serial.println("Falha ao inicializar o BME280!"); // Exibe mensagem de erro no Serial Monitor se a inicialização falhar
        while (1);                                        // Entra em loop infinito para parar a execução em caso de falha
    }

    // Inicializa o display LCD
    lcd.begin(16, 2);                               // Inicializa o display LCD com 16 colunas e 2 linhas
    lcd.backlight();                                // Liga a luz de fundo do LCD
    lcd.clear();                                    // Limpa o display LCD
    leds.setNormal();                               // Define o estado inicial dos LEDs como "normal" (LED verde aceso, LED vermelho apagado)
    initializeAccelerometer();                      // Chama a função para inicializar o acelerômetro

    // Configuração do Ubidots
    ubidots.setDebug(true);                         // Habilita o modo de depuração da biblioteca Ubidots
    ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);    // Conecta à rede Wi-Fi
    ubidots.setCallback(callback);                   // Define a função de callback para mensagens recebidas
    ubidots.setup();                                 // Configura a biblioteca Ubidots
    ubidots.reconnect();                             // Tenta reconectar ao servidor Ubidots
}

// Loop principal do sistema
void loop() {
    unsigned long currentMillis = millis();         // Obtém o tempo atual em milissegundos desde o início do programa

    // Verifica se o intervalo de tempo definido já passou
    if (currentMillis - previousMillis >= INTERVAL) {
        previousMillis = currentMillis;             // Atualiza o tempo da última execução do loop
        checkTemperatureAndVibration();             // Chama a função para verificar temperatura e vibração
        publishData();                               // Chama a função para publicar os dados no Ubidots
    }

    checkButtonState();                              // Chama a função para verificar o estado do botão
    ubidots.loop();                                  // Processa o loop da biblioteca Ubidots para gerenciar conexões e publicações
}

// Função para verificar temperatura e vibração, e atualizar LEDs de acordo
void checkTemperatureAndVibration() {
    bool isOn = sensors.detectVibration();          // Verifica se há vibração detectada pelo acelerômetro
    float temperature = sensors.getTemperature();    // Lê a temperatura atual do sensor BME280

    Serial.print("Temperatura atual: ");            // Exibe mensagem no Serial Monitor
    Serial.print(temperature);                      // Exibe o valor da temperatura
    Serial.println(" C");                           // Exibe a unidade de temperatura

    if (isOn) {                                      // Se houver vibração detectada
        if (temperature >= TEMPERATURE_THRESHOLD) {  // Se a temperatura estiver acima do limite definido
            leds.setHighTemp();                      // Acende o LED vermelho e apaga o verde
            Serial.println("Temperatura alta! LED vermelho aceso."); // Exibe mensagem no Serial Monitor
        }
        else {                                       // Se a temperatura estiver abaixo do limite
            leds.setNormal();                        // Acende o LED verde e apaga o vermelho
            Serial.println("Temperatura normal. LED verde aceso."); // Exibe mensagem no Serial Monitor
        }
    }
    else {                                           // Se não houver vibração detectada
        leds.turnOff();                              // Apaga ambos os LEDs
        Serial.println("Sem vibração detectada. LEDs apagados."); // Exibe mensagem no Serial Monitor
    }
}

// Função para verificar o estado do botão e atualizar o display
void checkButtonState() {
    // Verifica se o botão foi pressionado (estado LOW) e ainda não está marcado como pressionado
    if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
        buttonPressed = true;                        // Marca que o botão foi pressionado
        Serial.println("Botão solto!");              // Exibe mensagem no Serial Monitor
        lcd.clear();                                 // Limpa o display LCD
    }
    // Verifica se o botão foi solto (estado HIGH) e estava marcado como pressionado
    if (digitalRead(BUTTON_PIN) == HIGH && buttonPressed) {
        buttonPressed = false;                       // Marca que o botão não está mais pressionado
        Serial.println("Botão pressionado!");        // Exibe mensagem no Serial Monitor
        float temperature = sensors.getTemperature(); // Lê a temperatura atual do sensor
        lcd.clear();                                 // Limpa o display LCD
        lcd.setCursor(0, 0);                         // Move o cursor para a primeira coluna da primeira linha
        lcd.print("Temperatura:");                   // Exibe o texto "Temperatura:" no LCD
        lcd.setCursor(0, 1);                         // Move o cursor para a primeira coluna da segunda linha
        lcd.print(temperature);                      // Exibe o valor da temperatura
        lcd.print(" C");                             // Exibe a unidade de temperatura
    }
}

// Função para publicar os dados no Ubidots
void publishData() {
    float temperature = sensors.getTemperature();        // Obtém a temperatura atual
    float magnitude = sensors.getVibrationMagnitude();   // Obtém a magnitude da vibração

    ubidots.add(TEMP_ATUAL, temperature);               // Adiciona a temperatura à mensagem de publicação
    ubidots.publish(DEVICE_LABEL);                       // Publica os dados no dispositivo especificado no Ubidots
    ubidots.add(MAGNITUDE_ATUAL, magnitude);             // Adiciona a magnitude da vibração à mensagem de publicação
    ubidots.publish(DEVICE_LABEL);                       // Publica os dados no dispositivo especificado no Ubidots
}
