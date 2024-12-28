// Inclui as bibliotecas necessárias para o funcionamento do projeto
#include <Wire.h>                     // Biblioteca para comunicação I2C
#include <LiquidCrystal_I2C.h>        // Biblioteca para controle de display LCD via I2C
#include <Adafruit_Sensor.h>          // Biblioteca base para sensores Adafruit
#include <Adafruit_BME280.h>          // Biblioteca para o sensor de temperatura, pressão e umidade BME280

// Configuração do display LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);     // Cria um objeto para o display LCD com endereço I2C 0x27 e dimensões 16x2

// Configuração do sensor BME280
Adafruit_BME280 bme;                    // Cria um objeto para o sensor BME280
#define BME280_ADDRESS 0x76              // Endereço I2C padrão do BME280

// Configuração do sensor de distância HC-SR04
const int trigPin = 13;                  // GPIO 13 no ESP32 - Pino Trigger do HC-SR04
const int echoPin = 12;                  // GPIO 12 no ESP32 - Pino Echo do HC-SR04

// Configuração dos LEDs de alerta
const int ledAmareloPin = 2;             // GPIO 2 no ESP32 - Pino do LED amarelo

// Configuração do botão
const int botaoPin = 14;                 // GPIO 14 no ESP32 - Pino do botão
bool lcdLigado = true;                   // Estado inicial do LCD (ligado)

// Variáveis de controle
int displayState = 0;                     // Índice para rastrear a tela ativa no LCD
unsigned long lastButtonPressTime = 0;    // Armazena o tempo do último clique no botão
const unsigned long debounceTime = 1000;  // Tempo de debounce em milissegundos para evitar múltiplas detecções de um único clique
const int limiteCiclos = 2500;            // Número de ciclos para acender o LED amarelo de manutenção

// Parâmetro de deslocamento para definir um ciclo
const float deslocamentoCiclo = 10.0;      // Deslocamento em centímetros para completar um ciclo
float deslocamentoAcumulado = 0;           // Acumulador de deslocamento
float ultimaDistancia = 0;                 // Última distância medida
int ciclos = 0;                            // Contador de ciclos completos

void setup() {
  // Configuração dos pinos do HC-SR04
  pinMode(trigPin, OUTPUT);                // Configura o pino Trigger como saída
  pinMode(echoPin, INPUT);                 // Configura o pino Echo como entrada
  
  // Configuração do LED amarelo
  pinMode(ledAmareloPin, OUTPUT);          // Configura o pino do LED amarelo como saída
  
  // Configuração do botão
  pinMode(botaoPin, INPUT_PULLUP);         // Configura o pino do botão como entrada com resistor pull-up interno
  
  // Inicializa a comunicação I2C nos pinos GPIO 21 (SDA) e GPIO 22 (SCL)
  Wire.begin(21, 22);
  
  // Inicializa o display LCD
  lcd.begin(16, 2);                         // Inicializa o display LCD com 16 colunas e 2 linhas
  lcd.clear();                              // Limpa o display LCD
  lcd.backlight();                          // Liga a luz de fundo do LCD
  
  // Exibe mensagem de inicialização no LCD
  lcd.print("Sistema Iniciado");
  delay(1000);                               // Aguarda 1 segundo
  lcd.clear();                               // Limpa o display LCD
  
  // Inicialização do sensor BME280
  if (!bme.begin(BME280_ADDRESS)) {          // Tenta iniciar o sensor BME280 no endereço especificado
    Serial.println("Erro ao iniciar o sensor BME280!"); // Exibe mensagem de erro no Serial Monitor se a inicialização falhar
    while (1);                                // Entra em loop infinito para parar a execução em caso de falha
  }
  
  Serial.begin(115200);                      // Inicializa a comunicação serial a 115200 bps
}

// Função para verificar se a manutenção é necessária com base no número de ciclos
void verificarManutencao() {
  if (ciclos >= limiteCiclos) {               // Verifica se o número de ciclos atingiu o limite
    digitalWrite(ledAmareloPin, HIGH);        // Acende o LED amarelo
    Serial.println("LED amarelo ligado. Manutenção necessária."); // Exibe mensagem no Serial Monitor
  } else {
    digitalWrite(ledAmareloPin, LOW);         // Apaga o LED amarelo
    Serial.println("LED amarelo desligado.");  // Exibe mensagem no Serial Monitor
  }
}

void loop() {
  unsigned long currentTime = millis();       // Obtém o tempo atual em milissegundos desde o início do programa
  
  // Verificar o botão para alternar entre as telas
  if (digitalRead(botaoPin) == LOW) {        // Verifica se o botão foi pressionado (estado LOW)
    if (currentTime - lastButtonPressTime >= debounceTime) { // Verifica se o tempo de debounce foi respeitado
      lastButtonPressTime = currentTime;      // Atualiza o tempo do último clique
      
      if (lcdLigado) {                        // Se o LCD estiver ligado
        displayState = (displayState + 1) % 4; // Avança para a próxima tela (0 a 3)
        
        if (displayState == 0) {               // Após a última tela, desliga o LCD
          lcdLigado = false;                   // Atualiza o estado do LCD para desligado
          lcd.noBacklight();                    // Apaga a luz de fundo do LCD
          lcd.clear();                          // Limpa o display LCD
        }
      } else {
        lcdLigado = true;                      // Liga o LCD
        displayState = 0;                      // Reinicia o índice da tela para 0
        lcd.backlight();                       // Liga a luz de fundo do LCD
      }
    }
  }
  
  if (lcdLigado) {                            // Se o LCD estiver ligado
    lcd.clear();                              // Limpa o display LCD
    if (displayState == 0) {                   // Tela 0: Exibe a temperatura
      float temperature = readTemperature();    // Lê a temperatura do sensor BME280
      lcd.print("Temperatura:");               // Exibe o texto "Temperatura:" no LCD
      lcd.setCursor(0, 1);                     // Move o cursor para a primeira coluna da segunda linha
      lcd.print(temperature);                  // Exibe o valor da temperatura
      lcd.print(" C");                         // Exibe a unidade de temperatura
    } 
    else if (displayState == 1) {             // Tela 1: Exibe a distância medida
      float distance = readSmoothDistance();    // Lê a distância do sensor HC-SR04 com suavização
      lcd.print("Distancia:");                  // Exibe o texto "Distancia:" no LCD
      lcd.setCursor(0, 1);                      // Move o cursor para a primeira coluna da segunda linha
      lcd.print(distance);                      // Exibe o valor da distância
      lcd.print(" cm");                         // Exibe a unidade de distância
    } 
    else if (displayState == 2) {             // Tela 2: Exibe o número de ciclos completos
      lcd.print("Ciclos:");                     // Exibe o texto "Ciclos:" no LCD
      lcd.setCursor(0, 1);                      // Move o cursor para a primeira coluna da segunda linha
      lcd.print(ciclos);                        // Exibe o número de ciclos completos
    } 
    else if (displayState == 3) {             // Tela 3: Exibe o tempo total ligado
      unsigned long tempoLigado = millis() / 1000; // Calcula o tempo total ligado em segundos
      unsigned long horas = tempoLigado / 3600;     // Calcula o número de horas
      unsigned long minutos = (tempoLigado % 3600) / 60; // Calcula o número de minutos
      unsigned long segundos = tempoLigado % 60;      // Calcula o número de segundos
      
      lcd.print("Tempo ligado:");                        // Exibe o texto "Tempo ligado:" no LCD
      lcd.setCursor(0, 1);                               // Move o cursor para a primeira coluna da segunda linha
      lcd.print(horas);                                   // Exibe o número de horas
      lcd.print("h ");
      lcd.print(minutos);                                 // Exibe o número de minutos
      lcd.print("m ");
      lcd.print(segundos);                                // Exibe o número de segundos
      lcd.print("s");
    }
  }
  
  verificarCicloPorDeslocamento();             // Chama a função para verificar ciclos com base no deslocamento
  verificarManutencao();                        // Chama a função para verificar se a manutenção é necessária
}

// Função para verificar ciclos com base no deslocamento acumulado
void verificarCicloPorDeslocamento() {
  float distanciaAtual = readSmoothDistance();   // Lê a distância atual do sensor HC-SR04 com suavização
  
  // Calcula o deslocamento entre a última e a atual distância
  float deslocamentoAtual = abs(distanciaAtual - ultimaDistancia); // Calcula o deslocamento absoluto
  deslocamentoAcumulado += deslocamentoAtual;       // Adiciona o deslocamento atual ao acumulado
  
  // Exibe os deslocamentos no Serial Monitor para depuração
  Serial.print("Deslocamento atual: ");
  Serial.print(deslocamentoAtual);
  Serial.print(" cm, Acumulado: ");
  Serial.print(deslocamentoAcumulado);
  Serial.println(" cm");
  
  // Verifica se o deslocamento acumulado completou um ciclo
  if (deslocamentoAcumulado >= deslocamentoCiclo) {
    ciclos++;                                     // Incrementa o contador de ciclos
    deslocamentoAcumulado = 0;                    // Reseta o deslocamento acumulado
    Serial.print("Ciclo completado. Total ciclos: ");
    Serial.println(ciclos);
  }
  
  ultimaDistancia = distanciaAtual;               // Atualiza a última distância medida
}

// Função para ler a temperatura do sensor BME280
float readTemperature() {
  return bme.readTemperature();                   // Retorna a temperatura em graus Celsius
}

// Função para ler a distância com suavização (média de 5 leituras)
float readSmoothDistance() {
  float totalDistance = 0;                        // Inicializa o acumulador de distância
  for (int i = 0; i < 5; i++) {                   // Realiza 5 leituras
    totalDistance += readDistance();               // Soma as leituras de distância
    delay(10);                                     // Pequeno atraso entre as leituras
  }
  return totalDistance / 5;                        // Retorna a média das 5 leituras
}

// Função para ler a distância do sensor HC-SR04
float readDistance() {
  digitalWrite(trigPin, LOW);                      // Garante que o pino Trigger esteja LOW
  delayMicroseconds(2);                            // Aguarda 2 microssegundos
  digitalWrite(trigPin, HIGH);                     // Gera um pulso HIGH no pino Trigger
  delayMicroseconds(10);                           // Mantém o pulso HIGH por 10 microssegundos
  digitalWrite(trigPin, LOW);                      // Finaliza o pulso no pino Trigger
  
  long duration = pulseIn(echoPin, HIGH);           // Mede o tempo do pulso no pino Echo
  float distance = duration * 0.034 / 2;            // Calcula a distância em centímetros (velocidade do som = 0.034 cm/us)
  return distance;                                  // Retorna a distância calculada
}
