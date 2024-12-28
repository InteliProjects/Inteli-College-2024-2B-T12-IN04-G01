// Inclui as bibliotecas necessárias para o funcionamento do projeto
#include <Wire.h>                    // Biblioteca para comunicação I2C
#include <Adafruit_Sensor.h>         // Biblioteca base para sensores Adafruit
#include <Adafruit_BME280.h>         // Biblioteca para o sensor de temperatura, pressão e umidade BME280
#include <LiquidCrystal_I2C.h>       // Biblioteca para controle de display LCD via I2C
#include <cmath>                     // Biblioteca para funções matemáticas (ex: sqrt)

// Definições de constantes
#define BME280_ADDRESS 0x76           // Endereço I2C do sensor BME280
#define TEMPERATURE_THRESHOLD 28.0     // Limite de temperatura em graus Celsius para acender o LED vermelho
#define BUTTON_PIN 34                  // Pino digital conectado ao botão
#define ACCEL_ADDRESS 0x1C             // Endereço I2C do acelerômetro (assumido)
#define LED_GREEN 12                   // Pino digital conectado ao LED verde
#define LED_RED 27                     // Pino digital conectado ao LED vermelho
#define INTERVAL 500                   // Intervalo de tempo em milissegundos para atualização periódica

// Instância do sensor BME280
Adafruit_BME280 bme;                           // Cria um objeto para o sensor BME280
LiquidCrystal_I2C lcd(0x27, 16, 2);            // Cria um objeto para o display LCD com endereço I2C 0x27 e dimensões 16x2

// Classe para gerenciar o estado dos LEDs
class LEDManager
{
private:
    int pinRed, pinGreen;                     // Pinos dos LEDs vermelho e verde

public:
    // Construtor da classe LEDManager que recebe os pinos dos LEDs
    LEDManager(int redPin, int greenPin) : pinRed(redPin), pinGreen(greenPin)
    {
        pinMode(pinRed, OUTPUT);              // Configura o pino do LED vermelho como saída
        pinMode(pinGreen, OUTPUT);            // Configura o pino do LED verde como saída
    }

    // Método para acender o LED verde e apagar o vermelho (estado normal)
    void setNormal()
    {
        digitalWrite(pinGreen, HIGH);         // Acende o LED verde
        digitalWrite(pinRed, LOW);            // Apaga o LED vermelho
    }

    // Método para acender o LED vermelho e apagar o verde (temperatura alta)
    void setHighTemp()
    {
        digitalWrite(pinGreen, LOW);          // Apaga o LED verde
        digitalWrite(pinRed, HIGH);           // Acende o LED vermelho
    }

    // Método para apagar ambos os LEDs
    void turnOff()
    {
        digitalWrite(pinGreen, LOW);          // Apaga o LED verde
        digitalWrite(pinRed, LOW);            // Apaga o LED vermelho
    }
};

// Classe para gerenciar o sensor de temperatura e pressão
class SensorManager
{
private:
    float lowVibrationThreshold = 970;         // Limite inferior de vibração para detectar movimento
    float highVibrationThreshold = 1050;       // Limite superior de vibração para detectar movimento

public:
    // Construtor da classe SensorManager
    SensorManager() {}

    // Método para obter a temperatura atual do sensor BME280
    float getTemperature()
    {
        return bme.readTemperature();            // Retorna a temperatura em graus Celsius
    }

    // Método para calcular e retornar a magnitude da vibração baseada nos dados do acelerômetro
    float getVibrationMagnitude()
    {
        unsigned int data[7];                    // Buffer para armazenar os dados lidos do acelerômetro
        Wire.requestFrom(ACCEL_ADDRESS, 7);      // Solicita 7 bytes de dados do acelerômetro

        if (Wire.available() == 7)                // Verifica se todos os 7 bytes foram recebidos
        {
            for (int i = 0; i < 7; i++)
                data[i] = Wire.read();            // Lê os dados recebidos e armazena no buffer
        }
        else
        {
            Serial.println("Erro: Dados incompletos do acelerômetro"); // Mensagem de erro no Serial Monitor
            return 0.0;                           // Retorna 0.0 indicando erro na leitura
        }

        // Processamento dos dados recebidos para obter as acelerações nos eixos X, Y e Z
        int xAccl = ((data[1] * 256) + data[2]) / 16; // Calcula a aceleração no eixo X
        int yAccl = ((data[3] * 256) + data[4]) / 16; // Calcula a aceleração no eixo Y
        int zAccl = ((data[5] * 256) + data[6]) / 16; // Calcula a aceleração no eixo Z

        // Ajuste para valores negativos se necessário (assumindo representação em complemento de dois)
        if (xAccl > 2047)
            xAccl -= 4096;                        // Ajusta a aceleração no eixo X para valores negativos
        if (yAccl > 2047)
            yAccl -= 4096;                        // Ajusta a aceleração no eixo Y para valores negativos
        if (zAccl > 2047)
            zAccl -= 4096;                        // Ajusta a aceleração no eixo Z para valores negativos

        // Calcula a magnitude total da aceleração
        float magnitude = sqrt(xAccl * xAccl + yAccl * yAccl + zAccl * zAccl); // Calcula a magnitude vetorial da aceleração
        return magnitude;                           // Retorna a magnitude calculada
    }

    // Método para detectar vibração com base na magnitude calculada
    bool detectVibration()
    {
        static unsigned long lastTimeInRange = 0; // Última vez que a vibração estava no intervalo de descanso
        static bool isMachineOn = true;           // Estado atual da máquina (ligada/desligada)

        float magnitude = getVibrationMagnitude(); // Obtém a magnitude da vibração
        Serial.print("Magnitude da vibração: ");   // Exibe a magnitude no Serial Monitor
        Serial.println(magnitude);

        // Verifica se a magnitude está no intervalo de descanso
        if (magnitude >= lowVibrationThreshold && magnitude <= highVibrationThreshold)
        {
            // Se estiver no intervalo, verifica por quanto tempo permanece
            if (millis() - lastTimeInRange > 3000)
            { // Ajuste o tempo conforme necessário (e.g., 3000 ms)
                isMachineOn = false;               // Considera a máquina desligada após 3 segundos sem vibração
            }
        }
        else
        {
            // Reinicia o timer e o estado se a magnitude estiver fora do intervalo
            lastTimeInRange = millis();            // Atualiza o tempo do último movimento
            isMachineOn = true;                    // Considera a máquina ligada
        }

        return isMachineOn;                         // Retorna o estado da máquina (ligada/desligada)
    }
};

// Variáveis globais e instâncias
LEDManager leds(LED_RED, LED_GREEN);             // Instância da classe LEDManager para controlar os LEDs vermelho e verde
SensorManager sensors;                           // Instância da classe SensorManager para gerenciar sensores
bool buttonPressed = false;                      // Variável para armazenar o estado do botão (pressionado ou não)
unsigned long previousMillis = 0;                 // Armazena o tempo do último ciclo
unsigned long runTime = 0;                        // Tempo total de operação em milissegundos
int screenIndex = 0;                              // Índice para rastrear a tela ativa no LCD
int lastScreenIndex = -1;                         // Rastreia a última tela exibida no LCD
bool lcdOn = true;                                // Indica se o LCD está ligado
unsigned long lastUpdateTimeLCD = 0;              // Armazena o tempo da última atualização do LCD
const unsigned long updateIntervalLCD = 2000;     // Intervalo de atualização do LCD em milissegundos

// Função de configuração inicial
void setup()
{
    Wire.begin(21, 22);                            // Inicializa a comunicação I2C nos pinos 21 (SDA) e 22 (SCL)
    Serial.begin(115200);                          // Inicializa a comunicação serial a 115200 bps
    pinMode(BUTTON_PIN, INPUT_PULLUP);             // Configura o pino do botão como entrada com resistor pull-up interno

    // Inicializa o sensor BME280 no endereço especificado
    if (!bme.begin(BME280_ADDRESS))
    {
        Serial.println("Falha ao inicializar o BME280!"); // Exibe mensagem de erro no Serial Monitor se a inicialização falhar
        while (1);                                        // Entra em loop infinito para parar a execução em caso de falha
    }

    // Inicializa o display LCD
    lcd.begin(16, 2);                               // Inicializa o display LCD com 16 colunas e 2 linhas
    lcd.init();                                     // Inicializa o LCD (redundante com lcd.begin, mas inclui inicializações adicionais)
    lcd.backlight();                                // Liga a luz de fundo do LCD
    Serial.println("LCD inicializado");             // Exibe mensagem de inicialização do LCD no Serial Monitor

    lcd.clear();                                    // Limpa o display LCD

    leds.setNormal();                               // Define o estado inicial dos LEDs como "normal" (LED verde aceso, LED vermelho apagado)
    initializeAccelerometer();                      // Chama a função para inicializar o acelerômetro
}

// Função para inicializar o acelerômetro configurando os registradores necessários
void initializeAccelerometer()
{
    Wire.beginTransmission(ACCEL_ADDRESS);          // Inicia a transmissão I2C com o endereço do acelerômetro
    Wire.write(0x2A); Wire.write(0x00); Wire.endTransmission(); // Escreve 0x00 no registrador 0x2A e finaliza a transmissão

    Wire.beginTransmission(ACCEL_ADDRESS);          // Inicia outra transmissão I2C com o endereço do acelerômetro
    Wire.write(0x2A); Wire.write(0x01); Wire.endTransmission(); // Escreve 0x01 no registrador 0x2A e finaliza a transmissão

    Wire.beginTransmission(ACCEL_ADDRESS);          // Inicia mais uma transmissão I2C com o endereço do acelerômetro
    Wire.write(0x0E); Wire.write(0x00); Wire.endTransmission(); // Escreve 0x00 no registrador 0x0E e finaliza a transmissão
}

// Função para verificar temperatura e vibração, e atualizar LEDs de acordo
void checkTemperatureAndVibration()
{
    bool isOn = sensors.detectVibration();          // Verifica se há vibração detectada pelo acelerômetro
    float temperature = sensors.getTemperature();    // Lê a temperatura atual do sensor BME280

    Serial.print("Temperatura atual: ");            // Exibe mensagem no Serial Monitor
    Serial.print(temperature);                      // Exibe o valor da temperatura
    Serial.println(" C");                           // Exibe a unidade de temperatura

    if (isOn)                                        // Se houver vibração detectada
    {
        if (temperature >= TEMPERATURE_THRESHOLD)    // Se a temperatura estiver acima do limite definido
        {
            leds.setHighTemp();                      // Acende o LED vermelho e apaga o verde
            Serial.println("Temperatura alta! LED vermelho aceso."); // Exibe mensagem no Serial Monitor
        }
        else                                         // Se a temperatura estiver abaixo do limite
        {
            leds.setNormal();                        // Acende o LED verde e apaga o vermelho
            Serial.println("Temperatura normal. LED verde aceso."); // Exibe mensagem no Serial Monitor
        }
    }
    else                                             // Se não houver vibração detectada
    {
        leds.turnOff();                              // Apaga ambos os LEDs
        Serial.println("Sem vibração detectada. LEDs apagados."); // Exibe mensagem no Serial Monitor
    }
}

// Função para atualizar o tempo de operação
void updateRunTime()
{
    if (sensors.detectVibration())                  // Se a vibração está detectada
    {
        runTime += INTERVAL;                         // Incrementa o tempo de operação pelo intervalo definido
        Serial.print("Tempo de operação: ");         // Exibe mensagem no Serial Monitor
        Serial.print(runTime / 1000);                // Exibe o tempo de operação em segundos
        Serial.println(" seg");                       // Exibe a unidade de tempo
    }
}

// Loop principal do programa
void loop()
{
    unsigned long currentMillis = millis();         // Obtém o tempo atual em milissegundos desde o início do programa

    // Verifica se o intervalo de tempo definido já passou
    if (currentMillis - previousMillis >= INTERVAL)
    {
        previousMillis = currentMillis;             // Atualiza o tempo do último ciclo
        checkTemperatureAndVibration();             // Chama a função para verificar temperatura e vibração
        updateRunTime();                             // Atualiza o tempo de operação se necessário
    }

    checkButtonState();                              // Chama a função para verificar o estado do botão

    // Verifica se houve troca de tela no LCD
    if (screenIndex != lastScreenIndex)
    { // Atualiza a tela apenas se necessário
        lastScreenIndex = screenIndex;               // Atualiza o índice da última tela exibida
    }

    // Verifica se é hora de atualizar o LCD
    if (millis() - lastUpdateTimeLCD >= updateIntervalLCD)
    {
        lastUpdateTimeLCD = millis();                // Atualiza o tempo da última atualização do LCD

        // Atualizar o LCD somente se estiver ligado
        if (lcdOn)
        {
            lcd.clear();                             // Limpa o display LCD
            switch (screenIndex)                     // Verifica qual tela deve ser exibida
            {
            case 0:
                lcd.setCursor(0, 0);                 // Move o cursor para a primeira coluna da primeira linha
                lcd.print("Temperatura:");           // Exibe o texto "Temperatura:"
                lcd.setCursor(0, 1);                 // Move o cursor para a primeira coluna da segunda linha
                lcd.print(sensors.getTemperature());  // Exibe a temperatura atual
                lcd.print(" C");                      // Exibe a unidade de temperatura
                break;

            case 1:
                lcd.setCursor(0, 0);                 // Move o cursor para a primeira coluna da primeira linha
                lcd.print("Tempo Ligado:");          // Exibe o texto "Tempo Ligado:"
                lcd.setCursor(0, 1);                 // Move o cursor para a primeira coluna da segunda linha
                lcd.print(runTime / 1000);           // Exibe o tempo de operação em segundos
                lcd.print(" seg");                    // Exibe a unidade de tempo
                break;

            case 2:
                lcd.setCursor(0, 0);                 // Move o cursor para a primeira coluna da primeira linha
                lcd.print("Status:");                 // Exibe o texto "Status:"
                lcd.setCursor(0, 1);                 // Move o cursor para a primeira coluna da segunda linha
                if (digitalRead(LED_GREEN) == HIGH)  // Verifica se o LED verde está aceso
                {
                    lcd.print("OK");                   // Exibe "OK" no LCD
                }
                else if (digitalRead(LED_RED) == HIGH) // Verifica se o LED vermelho está aceso
                {
                    lcd.print("Manutencao");           // Exibe "Manutencao" no LCD
                }
                else
                {
                    lcd.print("Desligado");            // Exibe "Desligado" no LCD
                }
                break;

            case 3:
                lcd.setCursor(0, 0);                 // Move o cursor para a primeira coluna da primeira linha
                lcd.print("Vibracao:");               // Exibe o texto "Vibracao:"
                lcd.setCursor(0, 1);                 // Move o cursor para a primeira coluna da segunda linha
                if (sensors.detectVibration())        // Verifica se há vibração detectada
                {
                    lcd.print("Ativa");                // Exibe "Ativa" no LCD
                }
                else
                {
                    lcd.print("Inativa");              // Exibe "Inativa" no LCD
                }
                break;
            }
        }
    }
}

// Função para verificar o estado do botão e realizar ações correspondentes
void checkButtonState()
{
    // Verifica se o botão foi pressionado (estado LOW) e ainda não está marcado como pressionado
    if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed)
    {
        buttonPressed = true;                        // Marca que o botão foi pressionado
        Serial.println("Botão solto!");              // Exibe mensagem no Serial Monitor
        lcd.clear();                                 // Limpa o display LCD
    }

    // Verifica se o botão foi solto (estado HIGH) e estava marcado como pressionado
    if (digitalRead(BUTTON_PIN) == HIGH && buttonPressed)
    {
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
