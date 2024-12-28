// Inclui as bibliotecas necessárias para o funcionamento do projeto
#include <OneWire.h>             // Biblioteca para comunicação com dispositivos OneWire (como sensores de temperatura DS18B20)
#include <Wire.h>                // Biblioteca para comunicação I2C
#include <DallasTemperature.h>   // Biblioteca específica para sensores de temperatura Dallas (DS18B20)
#include <MPU6050.h>             // Biblioteca para comunicação com o sensor MPU6050 (acelerômetro e giroscópio)
#include <LiquidCrystal.h>       // Biblioteca para controle de displays LCD

// Definições de pinos e constantes
#define ONE_WIRE_BUS 4                       // Pino digital conectado ao barramento OneWire (sensor DS18B20)
#define TEMPERATURE_THRESHOLD 70.0           // Limite de temperatura (em graus Celsius) para acender o LED vermelho
#define BUTTON 5                             // Pino digital conectado ao botão

// Definições dos pinos dos LEDs
#define LED_GREEN 12                         // Pino digital conectado ao LED verde
#define LED_RED 27                           // Pino digital conectado ao LED vermelho

// Criação de instâncias para os sensores e display
OneWire oneWire(ONE_WIRE_BUS);               // Cria uma instância OneWire no pino definido
DallasTemperature sensors(&oneWire);         // Cria uma instância DallasTemperature utilizando o barramento OneWire
MPU6050 mpu;                                 // Cria uma instância do sensor MPU6050
LiquidCrystal lcd(19, 18, 17, 16, 15, 14);    // Cria uma instância do display LCD com os pinos RS, E, D4, D5, D6, D7 conectados aos pinos 19, 18, 17, 16, 15 e 14 respectivamente

float vibrationThreshold = 0.5;              // Valor de vibração para detectar movimento (sensibilidade)

// Função para ler a temperatura do sensor DS18B20
float checkTemperature() {
    sensors.requestTemperatures();            // Solicita a leitura de temperatura do sensor
    float temp = sensors.getTempCByIndex(0); // Lê a temperatura em graus Celsius do primeiro sensor no barramento
    
    return temp;                              // Retorna a temperatura lida
}

// Função para verificar a vibração usando o acelerômetro MPU6050
bool checkVibration() {
    int16_t ax, ay, az;                       // Variáveis para armazenar as leituras de aceleração nos eixos X, Y e Z
    mpu.getAcceleration(&ax, &ay, &az);       // Obtém as leituras de aceleração dos eixos X, Y e Z
    
    // Converte as leituras brutas do acelerômetro para valores em 'g' (aceleração da gravidade)
    float accX = ax / 16384.0;                // Converte a leitura bruta do eixo X para 'g'
    float accY = ay / 16384.0;                // Converte a leitura bruta do eixo Y para 'g'
    float accZ = az / 16384.0;                // Converte a leitura bruta do eixo Z para 'g'
    
    // Calcula a magnitude total da aceleração
    float magnitude = sqrt(accX * accX + accY * accY + accZ * accZ); // Calcula a magnitude vetorial da aceleração
    
    // Verifica se a magnitude ultrapassa o limite definido para detectar vibração
    if (magnitude > vibrationThreshold) {
        return true;                            // Retorna 'true' se houver vibração
    } else {
        return false;                           // Retorna 'false' se não houver vibração
    }
}

// Classe para gerenciar o estado dos LEDs
class LEDManager {
private:
    int _pin_red;       // Pino do LED vermelho
    int _pin_green;     // Pino do LED verde

public:
    // Construtor da classe LEDManager que recebe os pinos dos LEDs
    LEDManager(int pin_red, int pin_green){
        _pin_red = pin_red;             // Inicializa o pino do LED vermelho
        _pin_green = pin_green;         // Inicializa o pino do LED verde

        pinMode(_pin_green, OUTPUT);    // Configura o pino do LED verde como saída
        pinMode(_pin_red, OUTPUT);      // Configura o pino do LED vermelho como saída
    }

    // Método para acender o LED verde e apagar o vermelho (estado normal)
    void setNormal() {
        digitalWrite(_pin_green, HIGH);  // Acende o LED verde
        digitalWrite(_pin_red, LOW);      // Apaga o LED vermelho
    }
    
    // Método para acender o LED vermelho e apagar o verde (temperatura alta)
    void setHighTemp() {
        digitalWrite(_pin_green, LOW);    // Apaga o LED verde
        digitalWrite(_pin_red, HIGH);      // Acende o LED vermelho
    }

    // Método para apagar ambos os LEDs
    void turnOff() {
        digitalWrite(_pin_green, LOW);    // Apaga o LED verde
        digitalWrite(_pin_red, LOW);      // Apaga o LED vermelho
    }
};

// Cria uma instância da classe LEDManager para controlar os LEDs
LEDManager leds(LED_RED, LED_GREEN);          // Instância da classe LEDManager com os pinos definidos

// Configurações iniciais do sistema
void setup() {
    Serial.begin(115200);                      // Inicializa a comunicação serial a 115200 bps

    lcd.begin(16, 2);                           // Inicializa o display LCD com 16 colunas e 2 linhas

    pinMode(BUTTON, INPUT_PULLUP);             // Configura o pino do botão como entrada com resistor de pull-up interno

    sensors.begin();                            // Inicializa o sensor de temperatura DS18B20

    mpu.initialize();                           // Inicializa o sensor MPU6050
    Wire.begin(21, 22);                         // Inicializa a comunicação I2C nos pinos 21 (SDA) e 22 (SCL) para o MPU6050
}

// Variável para armazenar o estado do botão (pressionado ou não)
bool buttonPressed = false;

// Variáveis para controle de tempo
unsigned long previousMillis = 0;              // Armazena o tempo do último ciclo
const long interval = 500;                      // Intervalo de tempo para atualização (500 ms)

// Loop principal do programa
void loop() {
    unsigned long currentMillis = millis();      // Obtém o tempo atual em milissegundos desde o início do programa

    bool isVibrating = checkVibration();        // Verifica se há vibração detectada pelo acelerômetro
    float temperature = checkTemperature();      // Lê a temperatura atual do sensor DS18B20

    // Verifica se o intervalo de tempo definido já passou
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;          // Atualiza o tempo do último ciclo

        if (isVibrating) {                       // Se houver vibração detectada
            // Verifica se a temperatura excede o limite definido
            if (temperature >= TEMPERATURE_THRESHOLD) {
                leds.setHighTemp();               // Acende o LED vermelho (temperatura alta)
            } else {
                leds.setNormal();                 // Acende o LED verde (temperatura normal)
            }
        } else {
            leds.turnOff();                        // Apaga ambos os LEDs se não houver vibração
        }
    }

    // Verifica se o botão foi pressionado (estado LOW com INPUT_PULLUP)
    if (digitalRead(BUTTON) == LOW && !buttonPressed) {
        buttonPressed = true;                      // Marca que o botão foi pressionado

        // Exibe a temperatura atual no display LCD
        lcd.clear();                               // Limpa o display
        lcd.setCursor(0, 0);                       // Move o cursor para a primeira coluna da primeira linha
        lcd.print("Temp: ");                       // Exibe o texto "Temp: "
        lcd.print(temperature);                    // Exibe a temperatura lida
        lcd.print(" C");                           // Adiciona a unidade de medida "C"
    }

    // Verifica se o botão foi solto
    if (digitalRead(BUTTON) == HIGH && buttonPressed) {
        buttonPressed = false;                     // Marca que o botão foi solto
        lcd.clear();                               // Limpa o display LCD
    }
}
