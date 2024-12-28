#include <WiFi.h>  // Biblioteca para gerenciar conexões Wi-Fi no ESP32

// Definições das credenciais da rede Wi-Fi
const char* ssid = "SEU_SSID";       // Nome da rede Wi-Fi (SSID) - **Substitua "SEU_SSID" pelo nome da sua rede**
const char* password = "SUA_SENHA";  // Senha da rede Wi-Fi - **Substitua "SUA_SENHA" pela senha da sua rede**

// Variáveis para controle de tempo
unsigned long previousMillis = 0;     // Armazena o tempo do último evento de reconexão
const long interval = 5000;            // Intervalo de tempo (em milissegundos) para tentar reconectar (5 segundos)

void setup() {
  Serial.begin(115200);                  // Inicializa a comunicação serial a 115200 bps para depuração
  Serial.println();                      // Imprime uma linha em branco no Serial Monitor para melhor legibilidade
  Serial.println("Iniciando conexão ao WiFi...");  // Exibe uma mensagem informando o início da conexão Wi-Fi

  WiFi.begin(ssid, password);            // Inicia a conexão Wi-Fi com as credenciais fornecidas
}

void loop() {
  unsigned long currentMillis = millis(); // Obtém o tempo atual em milissegundos desde que o programa começou

  // Verifica se o ESP32 não está conectado à rede Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    // Se o intervalo definido tiver decorrido desde a última tentativa de reconexão
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;      // Atualiza o tempo do último evento de reconexão
      Serial.println("Tentando conectar ao WiFi..."); // Exibe uma mensagem informando uma nova tentativa de conexão

      WiFi.disconnect();                    // Desconecta qualquer conexão Wi-Fi existente
      WiFi.begin(ssid, password);            // Inicia uma nova tentativa de conexão Wi-Fi com as credenciais fornecidas
    }

  } else {
    // Se a conexão Wi-Fi foi bem-sucedida
    Serial.println("Conectado ao WiFi!");   // Exibe uma mensagem informando que a conexão foi estabelecida
    Serial.print("Endereço IP: ");          // Exibe o texto "Endereço IP: " no Serial Monitor
    Serial.println(WiFi.localIP());         // Exibe o endereço IP atribuído ao ESP32 na rede Wi-Fi
  }

}
