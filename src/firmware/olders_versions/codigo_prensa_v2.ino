// Inclui as bibliotecas necessárias para o funcionamento do projeto
#include <Wire.h>                     // Biblioteca para comunicação I2C
#include <LiquidCrystal_I2C.h>        // Biblioteca para controle de display LCD via I2C
#include <Adafruit_Sensor.h>          // Biblioteca base para sensores Adafruit
#include <Adafruit_BME280.h>          // Biblioteca para o sensor de temperatura, pressão e umidade BME280

// Variáveis para controle de ciclos baseados em tempo
unsigned long ultimoCiclo = 0;          // Armazena o tempo do último ciclo de 10 segundos
const unsigned long intervaloCiclo = 10000; // Intervalo de tempo para cada ciclo (10.000 ms = 10 segundos)

// Configuração do display LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);     // Cria um objeto para o display LCD com endereço I2C 0x27 e dimensões 16x2

// Configuração do sensor BME280
Adafruit_BME280 bme;                    // Cria um objeto para o sensor BME280
#define BME280_ADDRESS 0x76              // Define o endereço I2C padrão do BME280

// Configuração do sensor de distância HC-SR04
const int trigPin = 13;                  // Define o pino Trigger do HC-SR04 conectado ao GPIO 13 no ESP32
const int echoPin = 12;                  // Define o pino Echo do HC-SR04 conectado ao GPIO 12 no ESP32

// Configuração dos LEDs de alerta
const int ledAmareloPin = 2;             // Define o pino do LED amarelo conectado ao GPIO 2 no ESP32

// Configuração do botão
const int botaoPin = 14;                 // Define o pino do botão conectado ao GPIO 14 no ESP32
bool lcdLigado = true;                   // Variável para controlar o estado do LCD (ligado/desligado)

// Variáveis de controle
int displayState = 0;                     // Índice para rastrear a tela ativa no LCD (0 a 4)
unsigned long lastButtonPressTime = 0;    // Armazena o tempo do último clique no botão para debounce
const unsigned long debounceTime = 1000;  // Tempo de debounce em milissegundos para evitar múltiplas detecções de um único clique
const int limiteCiclos = 5;                // Número de ciclos para acender o LED amarelo de manutenção

// Parâmetro de deslocamento para definir um ciclo
const float deslocamentoCiclo = 150.0;      // Deslocamento em centímetros para completar um ciclo
float deslocamentoAcumulado = 0;            // Acumulador de deslocamento acumulado
float ultimaDistancia = 0;                  // Armazena a última distância medida pelo sensor
int ciclos = 0;                             // Contador de ciclos completos

// Variáveis para estado de "ligado/desligado"
bool estadoPrensa = true;                   // true = ligado, false = desligado
unsigned long tempoUltimoMovimento = 0;     // Armazena o tempo do último movimento detectado
unsigned long tempoInatividade = 10000;     // Tempo de inatividade para considerar a prensa desligada (10 segundos para testes)
unsigned long tempoLigado = 0;              // Tempo total que a prensa está ligada (em segundos)
unsigned long inicioUltimoLigado = 0;       // Armazena o início do último período de "ligado"

// Controle da frequência de mensagens no monitor serial
int contadorMensagensLed = 0;                // Contador para limitar a frequência das mensagens do LED amarelo

// Função para verificar o estado cíclico com base no tempo
void verificarEstadoCiclico() {
  unsigned long tempoAtual = millis();       // Obtém o tempo atual em milissegundos desde o início do programa

  // Verifica se o intervalo de 10 segundos foi atingido
  if (tempoAtual - ultimoCiclo >= intervaloCiclo) {
    ultimoCiclo = tempoAtual;                // Atualiza o tempo do último ciclo

    if (estadoPrensa) {                      // Se a prensa estiver ligada
      tempoLigado += intervaloCiclo / 1000;  // Adiciona 10 segundos ao tempo total ligado
      Serial.print("Prensa ativa. Tempo total ligado: ");
      Serial.print(tempoLigado);
      Serial.println(" segundos.");
    } else {
      Serial.println("Prensa inativa. Nenhuma atualização no tempo ligado.");
    }
  }
}

// Função de configuração inicial
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
  delay(1000);                               // Aguarda 1 segundo para que a mensagem seja visível
  lcd.clear();                               // Limpa o display LCD após a mensagem

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
    if (contadorMensagensLed % 10 == 0) {     // Condição para limitar a frequência das mensagens
      Serial.println("LED amarelo ligado. Manutenção necessária."); // Exibe mensagem de manutenção necessária
    }
  } else {
    digitalWrite(ledAmareloPin, LOW);         // Apaga o LED amarelo
    if (contadorMensagensLed % 10 == 0) {     // Condição para limitar a frequência das mensagens
      Serial.println("LED amarelo desligado."); // Exibe mensagem indicando que a manutenção não é necessária
    }
  }
  contadorMensagensLed++;                      // Incrementa o contador de mensagens para controle de frequência
}

// Função principal que roda continuamente
void loop() {
  unsigned long currentTime = millis();       // Obtém o tempo atual em milissegundos desde o início do programa

  // Verificar o botão para alternar entre as telas
  if (digitalRead(botaoPin) == LOW) {        // Verifica se o botão foi pressionado (estado LOW)
    if (currentTime - lastButtonPressTime >= debounceTime) { // Verifica se o tempo de debounce foi respeitado
      lastButtonPressTime = currentTime;      // Atualiza o tempo do último clique

      if (lcdLigado) {                        // Se o LCD estiver ligado
        displayState = (displayState + 1) % 5; // Avança para a próxima tela (0 a 4)

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
    } else if (displayState == 1) {           // Tela 1: Exibe a distância medida
      float distance = readSmoothDistance();    // Lê a distância do sensor HC-SR04 com suavização
      lcd.print("Distancia:");                  // Exibe o texto "Distancia:" no LCD
      lcd.setCursor(0, 1);                      // Move o cursor para a primeira coluna da segunda linha
      lcd.print(distance);                      // Exibe o valor da distância
      lcd.print(" cm");                         // Exibe a unidade de distância
    } else if (displayState == 2) {           // Tela 2: Exibe o número de ciclos completos
      lcd.print("Ciclos:");                     // Exibe o texto "Ciclos:" no LCD
      lcd.setCursor(0, 1);                      // Move o cursor para a primeira coluna da segunda linha
      lcd.print(ciclos);                        // Exibe o número de ciclos completos
    } else if (displayState == 3) {           // Tela 3: Exibe o status da prensa
      lcd.print("Status:");                     // Exibe o texto "Status:" no LCD
      lcd.setCursor(0, 1);                      // Move o cursor para a primeira coluna da segunda linha
      if (ciclos < limiteCiclos) {             // Se o número de ciclos estiver abaixo do limite
        lcd.print("OK");                         // Exibe "OK" indicando que está tudo bem
      } else {
        lcd.print("Manutencao");                 // Exibe "Manutencao" indicando que é necessário manutenção
      }
    } else if (displayState == 4) {           // Tela 4: Exibe o tempo total ligado
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

  // Chama as funções para verificar ciclos, estado da prensa e manutenção
  verificarCicloPorDeslocamento();             // Verifica ciclos com base no deslocamento acumulado
  verificarEstadoPrensa();                     // Verifica o estado da prensa (ligado/desligado) com base no movimento
  verificarManutencao();                       // Verifica se a manutenção é necessária com base no número de ciclos
  verificarEstadoCiclico();                    // Atualiza o estado e o tempo ligado em ciclos de 10 segundos
}

// Função para verificar ciclos com base no deslocamento acumulado
void verificarCicloPorDeslocamento() {
  float distanciaAtual = readSmoothDistance();   // Lê a distância atual do sensor HC-SR04 com suavização
  float deslocamentoAtual = abs(distanciaAtual - ultimaDistancia); // Calcula o deslocamento absoluto entre a última e a atual distância
  deslocamentoAcumulado += deslocamentoAtual;    // Adiciona o deslocamento atual ao deslocamento acumulado

  // Verifica se o deslocamento acumulado completou um ciclo
  if (deslocamentoAcumulado >= deslocamentoCiclo) {
    ciclos++;                                     // Incrementa o contador de ciclos completos
    deslocamentoAcumulado = 0;                    // Reseta o deslocamento acumulado para iniciar a contagem do próximo ciclo
    Serial.print("Ciclo completado. Total ciclos: ");
    Serial.println(ciclos);                       // Exibe o total de ciclos completados no Serial Monitor
  }

  ultimaDistancia = distanciaAtual;               // Atualiza a última distância medida para a próxima comparação
}

// Função para verificar o estado da prensa (ligado/desligado) com base no movimento
void verificarEstadoPrensa() {
  unsigned long tempoAtual = millis();           // Obtém o tempo atual em milissegundos desde o início do programa
  float distanciaAtual = readSmoothDistance();   // Lê a distância atual do sensor HC-SR04 com suavização
  float deslocamentoAtual = abs(distanciaAtual - ultimaDistancia); // Calcula o deslocamento absoluto entre a última e a atual distância

  if (deslocamentoAtual > 30.0) {                // Verifica se o deslocamento atual excede o mínimo para considerar como movimento (ligado)
    tempoUltimoMovimento = tempoAtual;            // Atualiza o tempo do último movimento detectado

    if (!estadoPrensa) {                          // Se a prensa estava desligada
      estadoPrensa = true;                        // Altera o estado para ligado
      inicioUltimoLigado = tempoAtual;             // Marca o início do período de atividade
      Serial.println("Prensa ligada!");            // Exibe mensagem no Serial Monitor
    }
  } else if (tempoAtual - tempoUltimoMovimento > tempoInatividade) { // Se não houve movimento por tempo suficiente
    if (estadoPrensa) {                           // Se a prensa estava ligada
      estadoPrensa = false;                       // Altera o estado para desligado
      tempoLigado += (tempoAtual - inicioUltimoLigado) / 1000; // Incrementa o tempo total de ligado (em segundos)
      Serial.println("Prensa desligada!");         // Exibe mensagem no Serial Monitor
    }
  }
}

// Função para ler a temperatura do sensor BME280
float readTemperature() {
  return bme.readTemperature();                   // Retorna a temperatura em graus Celsius
}

// Função para ler a distância com suavização (média de 5 leituras)
float readSmoothDistance() {
  float totalDistance = 0;                        // Inicializa o acumulador de distância
  for (int i = 0; i < 5; i++) {                   // Realiza 5 leituras de distância
    totalDistance += readDistance();               // Soma as leituras de distância
    delay(10);                                     // Pequeno atraso entre as leituras para estabilidade
  }
  return totalDistance / 5;                        // Retorna a média das 5 leituras
}

// Função para ler a distância do sensor HC-SR04
float readDistance() {
  digitalWrite(trigPin, LOW);                      // Garante que o pino Trigger esteja LOW antes de iniciar a medição
  delayMicroseconds(2);                            // Aguarda 2 microssegundos
  digitalWrite(trigPin, HIGH);                     // Gera um pulso HIGH no pino Trigger para iniciar a medição
  delayMicroseconds(10);                           // Mantém o pulso HIGH por 10 microssegundos, conforme especificação do HC-SR04
  digitalWrite(trigPin, LOW);                      // Finaliza o pulso no pino Trigger

  long duration = pulseIn(echoPin, HIGH);           // Mede o tempo do pulso no pino Echo (tempo que o som levou para ir e voltar)
  float distance = duration * 0.034 / 2;            // Calcula a distância em centímetros (velocidade do som = 0.034 cm/us) e divide por 2 para considerar a ida e volta
  return distance;                                  // Retorna a distância calculada
}
