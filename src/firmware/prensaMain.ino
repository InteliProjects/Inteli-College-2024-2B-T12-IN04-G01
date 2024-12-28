#include <Wire.h> // Inclui a biblioteca Wire para comunicação I2C
#include <LiquidCrystal_I2C.h> // Inclui a biblioteca LiquidCrystal_I2C para controle de displays LCD via I2C
#include <Adafruit_Sensor.h> // Inclui a biblioteca Adafruit Sensor para sensores
#include <Adafruit_BME280.h> // Inclui a biblioteca Adafruit BME280 para o sensor BME280
#include "UbidotsEsp32Mqtt.h" // Inclui a biblioteca UbidotsEsp32Mqtt para comunicação com a plataforma

// ========================= DECLARAÇÕES GLOBAIS E CONSTANTES =========================
const char *UBIDOTS_TOKEN = "BBUS-veCoBVrAsiykWhV0az7GqtT7AnbTQx"; // Token de autenticação do Ubidots
const char *WIFI_SSID = "Inteli.Iot"; // Nome da rede Wi-Fi
const char *WIFI_PASS = "@Intelix10T#"; // Senha da rede Wi-Fi
const char *DEVICE_LABEL = "esp32_t12_g01"; // Nome do dispositivo no Ubidots
const char *TOTAL_CICLOS = "ciclos_prensa"; // Variável para ciclos da prensa
const char *TEMPO_DE_FUNCIONAMENTO = "tempo_prensa"; // Variável para tempo de funcionamento
const char *STATUS_PRENSA = "status_prensa"; // Variável para status da prensa
const char *CLIENT_ID = "id_cliente_prensa_ipt"; // ID do cliente para conexão com o Ubidots

Ubidots ubidots(UBIDOTS_TOKEN, CLIENT_ID); // Instância do objeto Ubidots para enviar dados à plataforma

// Callback do Ubidots mantida globalmente
void callback(char *topic, byte *payload, unsigned int length) { // Callback para mensagens recebidas pelo Ubidots
    // Callback para mensagens recebidas pelo Ubidots
    Serial.print("Mensagem recebida [");
    Serial.print(topic); // Exibe o tópico da mensagem
    Serial.print("] "); // Exibe o colchete de fechamento
    for (int i = 0; i < length; i++) { // Loop para exibir o conteúdo da mensagem
        Serial.print((char)payload[i]); // Exibe o conteúdo da mensagem
    }
    Serial.println(); // Pula para a próxima linha após exibir a mensagem
}

// Função global para publicar dados no Ubidots
void publishData(float ciclos_prensa, float status_prensa, unsigned long runTime) { // Função para publicar dados no Ubidots
  ubidots.add(TOTAL_CICLOS, ciclos_prensa); // Adiciona o total de ciclos ao payload
  ubidots.add(STATUS_PRENSA, status_prensa); // Adiciona o status da prensa ao payload
  ubidots.add(TEMPO_DE_FUNCIONAMENTO, (runTime / 1000 / 3600)); // envia run time em horas
  ubidots.publish(DEVICE_LABEL);
}

// ========================= CLASSES =========================

// ---------- Classe SensorManager ----------
class SensorManager { // Classe para gerenciar os sensores
public: // Métodos públicos
  SensorManager() {} // Construtor da classe SensorManager

  void setupSensorBME(LiquidCrystal_I2C& lcd) { // Método para configurar o sensor BME280
  if (!bme.begin(MY_BME280_ADDRESS)) { // Inicializa o sensor BME280
    Serial.println("Erro ao iniciar o sensor BME280!"); // Exibe mensagem de erro no monitor serial
    lcd.print("Erro: Falha no BME280!"); // Exibe mensagem de erro no display LCD
    while (1); // Loop infinito
  }
}


  float readTemperature() { // Método para ler a temperatura
    return bme.readTemperature(); // retorna a temperatura em graus Celsius
  }

  float readDistance() { // Método para ler a distância
    digitalWrite(trigPin, LOW); // Inicia o pulso de ultrassom
    delayMicroseconds(2); // Aguarda 2 microssegundos
    digitalWrite(trigPin, HIGH); // Finaliza o pulso de ultrassom
    delayMicroseconds(10); // Aguarda 10 microssegundos
    digitalWrite(trigPin, LOW); // Reseta o pulso de ultrassom
    unsigned long duracao = pulseIn(echoPin, HIGH); // Lê a duração do pulso
    unsigned long distancia = duracao * 0.034 / 2; // Cálculo da distância

    return distancia; // retorna a distância
  }

private: // Atributos privados
  uint8_t trigPin = 5; // define o pino do sensor ultrassônico
  uint8_t echoPin = 18; // define o pino do sensor ultrassônico
  Adafruit_BME280 bme; // cria um objeto do sensor BME280
  static const uint8_t MY_BME280_ADDRESS = 0x76; // define o endereço do sensor BME280
};

// ---------- Classe PrensaMonitor ----------
class PrensaMonitor {
public: // Métodos públicos
  // Construtor da classe PrensaMonitor
  PrensaMonitor() : estadoPrensa(true), deslocamentoAcumulado(0.0), ultimaDistancia(0.0), ciclos(0), // Inicializa os atributos
                    tempoLigado(0), inicioUltimoLigado(0), tempoUltimoMovimento(0), ultimoCiclo(0), estadoAnterior(true) {}

  void atualizarCiclos(float distanciaAtual) { // Método para atualizar os ciclos
    if (distanciaAtual < 0) return; // Verifica se a distância é válida
    float deslocamentoAtual = abs(distanciaAtual - ultimaDistancia); // calcula o deslocamento atual
    deslocamentoAcumulado += deslocamentoAtual; // acumula o deslocamento

    if (deslocamentoAcumulado >= deslocamentoCiclo) { // verifica se o deslocamento acumulado é suficiente para um ciclo
      ciclos++; // incrementa o número de ciclos
      deslocamentoAcumulado = 0; // reseta o deslocamento acumulado
      Serial.print("Novo ciclo contabilizado: "); // Exibe uma mensagem no monitor serial
      Serial.println(ciclos); // Essa informação é exibida no display LCD sempre que o usuário pressiona o botão

      // Quando um novo ciclo for concluído, envia dados
      publishData(ciclos, estadoPrensa ? 1.0 : 0.0, tempoLigado); // envia os dados para o Ubidots
    }

    ultimaDistancia = distanciaAtual; // atualiza a última distância
  }

  void verificarEstado(float distanciaAtual) { // Método para verificar o estado da prensa
    if (distanciaAtual < 0) return; // Verifica se a distância é válida

    float deslocamentoAtual = abs(distanciaAtual - ultimaDistancia); // calcula o deslocamento atual
    unsigned long tempoAtual = millis(); // obtém o tempo atual

    if (deslocamentoAtual > 30.0) { // verifica se houve movimento
      tempoUltimoMovimento = tempoAtual; // atualiza o tempo do último movimento
      if (!estadoPrensa) { // verifica se a prensa estava desligada
        estadoPrensa = true; // liga a prensa
        inicioUltimoLigado = tempoAtual; // atualiza o início do último ligado
        Serial.println("Prensa ligada!");

        // Quando o status mudar, envia
        publishData(ciclos, estadoPrensa ? 1.0 : 0.0, tempoLigado); // envia os dados para o Ubidots
      }
    } else if (tempoAtual - tempoUltimoMovimento > tempoInatividade) {
      if (estadoPrensa) { // verifica se a prensa estava ligada
        estadoPrensa = false; // desliga a prensa
        tempoLigado += (tempoAtual - inicioUltimoLigado) / 1000;
        Serial.println("Prensa desligada!"); // exibe uma mensagem no monitor serial

        // Quando o status mudar, envia
        publishData(ciclos, estadoPrensa ? 1.0 : 0.0, tempoLigado); // envia os dados para o Ubidots
      }
    }

    estadoAnterior = estadoPrensa; // atualiza o estado anterior
  }

  void verificarEstadoCiclico() { // Método para verificar o estado cíclico
    unsigned long tempoAtual = millis(); // obtém o tempo atual
    if (tempoAtual - ultimoCiclo >= intervaloCiclo) {
      ultimoCiclo = tempoAtual; // atualiza o tempo do último ciclo
      if (estadoPrensa) {
        tempoLigado += intervaloCiclo / 1000; // incrementa o tempo ligado
        Serial.print("Prensa ativa. Tempo total ligado: ");
        Serial.print(tempoLigado); // exibe o tempo ligado
        Serial.println(" segundos.");
      } else { // se a prensa estiver desligada
        Serial.println("Prensa inativa. Nenhuma atualização no tempo ligado.");
      }
    }
  }

  bool getEstadoPrensa() const { return estadoPrensa; } // retorna o estado da prensa
  uint16_t getCiclos() const { return ciclos; } // retorna o número de ciclos
  unsigned long getTempoLigado() const { return tempoLigado; } // retorna o tempo ligado
  uint16_t getLimiteCiclos() const { return limiteCiclos; } // retorna o limite de ciclos

private:
  // Atributos da classe PrensaMonitor
  bool estadoPrensa; // estado da prensa
  float deslocamentoAcumulado; // deslocamento acumulado
  float ultimaDistancia; // última distância
  uint16_t ciclos; // número de ciclos
  unsigned long tempoLigado; // tempo ligado
  unsigned long inicioUltimoLigado; // início do último ligado
  unsigned long tempoUltimoMovimento; // tempo do último movimento
  unsigned long ultimoCiclo; // tempo do último ciclo
  bool estadoAnterior; // estado anterior

  static const uint16_t limiteCiclos = 5; // limite de ciclos
  static const float deslocamentoCiclo; // deslocamento para um ciclo
  static const unsigned long tempoInatividade = 10000; // tempo de inatividade
  static const unsigned long intervaloCiclo = 10000; // intervalo de ciclo
};

const float PrensaMonitor::deslocamentoCiclo = 150.0; // define o deslocamento para um ciclo

// ---------- Classe DisplayManager ----------
class DisplayManager {
public: // Métodos públicos
  // Construtor da classe DisplayManager
  DisplayManager() : displayState(0), lcdLigado(true), lastButtonPressTime(0), ultimoTempoAtualizacaoLCD(0) {}
  
  // Função para configurar o display
  void setupDisplay() { // Método para configurar o display
    lcd.begin(16, 2); // Inicializa o display com 16 colunas e 2 linhas
    lcd.clear(); // Limpa o display
    lcd.backlight(); // Liga a luz de fundo do display
    lcd.print("Sistema Iniciado"); // Exibe uma mensagem no display
    delay(1000); // Aguarda 1 segundo
    lcd.clear(); // Limpa o display
  }
  
  // Função getter para retornar o objeto lcd
  LiquidCrystal_I2C& getLcd() {
    return lcd; // retorna o objeto lcd
  }

  // Função para lidar com o botão
  void handleButton() { // Método para lidar com o botão
    unsigned long currentTime = millis(); // obtém o tempo atual
    if (digitalRead(botaoPin) == LOW) {
      if (currentTime - lastButtonPressTime >= debounceTime) {
        lastButtonPressTime = currentTime; // atualiza o tempo do último pressionamento
        if (lcdLigado) {
          displayState = (displayState + 1) % 5; // alterna entre os estados
          if (displayState == 0) {
            lcdLigado = false; // desliga o display
            lcd.noBacklight(); // desliga a luz de fundo
            lcd.clear(); // limpa o display
          }
        } else { // se o display estiver desligado
          lcdLigado = true; // liga o display
          displayState = 0; // reseta o estado
          lcd.backlight(); // liga a luz de fundo
        }
      }
    }
  }
  // Função para atualizar o LCD
  void atualizarLCD(bool estadoPrensa, int ciclos, unsigned long tempoLigado, float temperatura, float distancia) { // Método para atualizar o display LCD
    if (!lcdLigado) return; // verifica se o display está ligado

    unsigned long currentTime = millis(); // obtém o tempo atual
    if (currentTime - ultimoTempoAtualizacaoLCD >= intervaloAtualizacaoLCD) { // verifica se é hora de atualizar o display
      ultimoTempoAtualizacaoLCD = currentTime; // atualiza o tempo da última atualização
      lcd.clear(); // limpa o display

      switch (displayState) { // alterna entre os estados
        case 0: // caso 0
          lcd.print("Temperatura:"); // exibe a temperatura
          lcd.setCursor(0, 1); // define o cursor na posição (0, 1)
          lcd.print(temperatura); // exibe a temperatura
          lcd.print(" C"); // exibe a unidade de temperatura
          break; // sai do switch

        case 1:
          lcd.print("Distancia:"); // exibe a distância
          lcd.setCursor(0, 1); // define o cursor na posição (0, 1)
          lcd.print(distancia); // exibe a distância
          lcd.print(" cm"); // exibe a unidade de distância
          break; // sai do switch

        case 2: // caso 2
          lcd.print("Ciclos:"); // exibe o número de ciclos
          lcd.setCursor(0, 1); // define o cursor na posição (0, 1)
          lcd.print(ciclos); // exibe o número de ciclos
          break; // sai do switch

        case 3: // caso 3
          lcd.print("Status:"); // exibe o status
          lcd.setCursor(0, 1); // define o cursor na posição (0, 1)
          if (ciclos < 5) { // verifica se o número de ciclos é menor que 5
            lcd.print("OK"); // exibe "OK" se o número de ciclos for menor que 5 (5, nesse caso, é o número limite de ciclos apenas para testes, na versão final será um número mais próximo da realidade)
          } else {
            lcd.print("Manutencao"); // exibe "Manutencao" se o número de ciclos for maior ou igual a 5
          }
          break; // sai do switch

        case 4: { // caso 4
          unsigned long horas = tempoLigado / 3600; // calcula as horas
          unsigned long minutos = (tempoLigado % 3600) / 60; // calcula os minutos
          unsigned long segundos = tempoLigado % 60; // calcula os segundos

          lcd.print("Tempo ligado:"); // exibe o tempo ligado
          lcd.setCursor(0, 1); // define o cursor na posição (0, 1)
          lcd.print(horas); // exibe as horas
          lcd.print("h "); // exibe a unidade de horas
          lcd.print(minutos); // exibe os minutos 
          lcd.print("m "); // exibe a unidade de minutos
          lcd.print(segundos); // exibe os segundos
          lcd.print("s"); // exibe a unidade de segundos
          break; // sai do switch
        }
      }
    }
  }

private: // Atributos privados
  LiquidCrystal_I2C lcd{0x27, 16, 2}; // cria um objeto do display LCD
  uint8_t displayState; // estado do display
  bool lcdLigado; // estado do display
  unsigned long lastButtonPressTime; // tempo do último pressionamento
  unsigned long ultimoTempoAtualizacaoLCD; // tempo da última atualização

  static const uint8_t botaoPin = 14; // define o pino do botão
  static const unsigned long debounceTime; // tempo de debounce
  static const unsigned long intervaloAtualizacaoLCD; // intervalo de atualização do display
};
// Constantes da classe DisplayManager
const unsigned long DisplayManager::debounceTime = 1000; // define o tempo de debounce
const unsigned long DisplayManager::intervaloAtualizacaoLCD = 2000; // define o intervalo de atualização do display

// ---------- Classe AlertManager ----------
class AlertManager { // Classe para gerenciar alertas
public:
  // Construtor da classe AlertManager
  AlertManager() : contadorMensagensLed(0) {} // Inicializa o contador de mensagens

  void setupLeds() {
    pinMode(ledAmareloPin, OUTPUT); // Configura o pino do LED amarelo como saída
    pinMode(LedVerdePin, OUTPUT); // Configura o pino do LED verde como saída
  }

  void verificarManutencao(uint16_t ciclos, uint16_t limiteCiclos) { // Método para verificar a manutenção
    if (ciclos >= limiteCiclos) { // verifica se o número de ciclos ultrapassou o limite
      digitalWrite(ledAmareloPin, HIGH); // liga o LED amarelo
      if (contadorMensagensLed % 10 == 0) { // exibe a mensagem a cada 10 ciclos
        Serial.println("LED amarelo ligado. Manutencao necessaria."); // exibe uma mensagem no monitor serial
      }
    } else { // se o número de ciclos não ultrapassou o limite
      digitalWrite(ledAmareloPin, LOW); // desliga o LED amarelo
      if (contadorMensagensLed % 10 == 0) { // exibe a mensagem a cada 10 ciclos
        Serial.println("LED amarelo desligado."); // exibe uma mensagem no monitor serial
      }
    }
    contadorMensagensLed++; // incrementa o contador de mensagens
  }
  // Função para atualizar o LED verde
  void atualizarLEDVerde(bool estadoPrensa) { // Método para atualizar o LED verde
    if (estadoPrensa) { // verifica o estado da prensa
      digitalWrite(LedVerdePin, HIGH); // liga o LED verde
    } else {
      digitalWrite(LedVerdePin, LOW); // desliga o LED verde
    }
  }

private: // Atributos privados
  // Atributos da classe AlertManager
  uint8_t contadorMensagensLed; // contador de mensagens
  static const uint8_t ledAmareloPin = 2; // define o pino do LED amarelo
  static const uint8_t LedVerdePin = 17; // define o pino do LED verde
};

// ========================= INSTANCIAS GLOBAIS =========================
PrensaMonitor prensaMonitor; // Instância da classe PrensaMonitor
SensorManager sensorManager; // Instância da classe SensorManager
DisplayManager displayManager; // Instância da classe DisplayManager
AlertManager alertManager; // Instância da classe AlertManager

// Variável para enviar tempo de funcionamento a cada segundo
unsigned long lastRunTimePublish = 0;

// ========================= SETUP =========================
void setup() { // Função de inicialização
  Serial.begin(115200); // Inicializa a comunicação serial

  // Inicializa os pinos
  pinMode(13, OUTPUT);        // LED de alerta
  pinMode(12, INPUT);         // Entrada do sensor
  pinMode(14, INPUT_PULLUP);  // Botão
  pinMode(5, OUTPUT);         // Trigger do ultrassônico
  pinMode(18, INPUT);         // Echo do ultrassônico

  Wire.begin(21, 22);  // Inicializa o I2C nos pinos 21 (SDA) e 22 (SCL)

  // Inicializa o Display
  displayManager.setupDisplay();

  // Inicializa o sensor BME280, passando o lcd do DisplayManager
  sensorManager.setupSensorBME(displayManager.getLcd());

  // Inicializa LEDs e outros alertas
  alertManager.setupLeds();

  // Configura Ubidots
  ubidots.setDebug(true); // Ativa o modo de depuração
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS); // Conecta à rede Wi-Fi
  ubidots.setCallback(callback); // Define a função de callback
  ubidots.setup(); // Configura a conexão MQTT
  ubidots.reconnect(); // Tenta reconectar ao Ubidots caso a conexão seja perdida
}


// ========================= LOOP =========================
void loop() { // Função principal
  float distanciaAtual = sensorManager.readDistance(); // lê a distância atual
  float temperaturaAtual = sensorManager.readTemperature(); // lê a temperatura atual

  // Atualiza estado da prensa e ciclos
  prensaMonitor.atualizarCiclos(distanciaAtual); // atualiza os ciclos
  prensaMonitor.verificarEstado(distanciaAtual); // verifica o estado
  prensaMonitor.verificarEstadoCiclico(); // verifica o estado cíclico

 // Verifica manutenção
  alertManager.verificarManutencao(prensaMonitor.getCiclos(), prensaMonitor.getLimiteCiclos()); // verifica a manutenção

  // Gerencia display
  displayManager.handleButton();  // gerencia o botão
  displayManager.atualizarLCD(   // atualiza o display
    prensaMonitor.getEstadoPrensa(), // obtém o estado da prensa
    prensaMonitor.getCiclos(), // obtém o número de ciclos
    prensaMonitor.getTempoLigado(),  // obtém o tempo ligado
    temperaturaAtual, // obtém a temperatura atual
    distanciaAtual // obtém a distância atualdkdjdjdjdjslahdlsç
  );

  // Atualiza LED verde
  alertManager.atualizarLEDVerde(prensaMonitor.getEstadoPrensa()); // atualiza o LED verde

  // Tempo de funcionamento: envia a cada 1 segundo
  unsigned long currentTime = millis(); // obtém o tempo atual
  if (currentTime - lastRunTimePublish >= 1000) { // verifica se é hora de enviar
    lastRunTimePublish = currentTime; // atualiza o tempo do último envio
    publishData(prensaMonitor.getCiclos(), prensaMonitor.getEstadoPrensa() ? 1.0 : 0.0, prensaMonitor.getTempoLigado()); // envia os dados para o Ubidots
  }
}