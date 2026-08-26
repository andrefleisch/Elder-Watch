#include <Wire.h>
#include <MPU6050.h>
#include <math.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <WebServer.h>

// === Constantes e Pinos === //
#define MAX_DATA_POINTS 100
#define BOTAO_PIN 18
#define LED_ALARME 25       
#define MICROFONE_PIN 33   
#define BUZZER 19
#define LED_VERMELHO 5
#define MAX_ALARMES 5

// === Estrutura para Alarme === //
// Adicionado 'ultimoDiaDisparo' para garantir que o alarme toque todos os dias.
struct Alarme {
  String hora; 
  String nome;
  int ultimoDiaDisparo; // Armazena o dia do ano (1-366) em que o alarme tocou pela última vez
};

// === Variáveis Globais === //
Alarme alarmes[MAX_ALARMES];
int qtdAlarmes = 0;
float movimentoHistorico[MAX_DATA_POINTS];
int movimentoIndex = 0;

// === Configurações de Rede e Telegram === //
#define BOT_TOKEN "#" 
#define CHAT_ID "#"                                     
const char* ssid = "WiFiTest";
const char* password = "12345678";


// === Parâmetros de Sensibilidade para Detecção de Queda === //
const float FREE_FALL_THRESHOLD = 1.2;
const float IMPACT_THRESHOLD = 2.0;
const float TILT_THRESHOLD = 45.0;
const unsigned long FALL_TIME_WINDOW = 1500;
const unsigned long TILT_TIME_THRESHOLD = 3000;

// === Configurações de Tempo (NTP) === //
#define GMT_OFFSET_SEC -3 * 3600 // Fuso horário de -3 horas (Brasília)
#define DAYLIGHT_OFFSET_SEC 0

// === Objetos Globais === //
MPU6050 mpu;
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
WebServer server(80);

// === Variáveis de Detecção de Queda === //
unsigned long fallStartTime = 0;
unsigned long tiltStartTime = 0;
bool fallDetected = false;
bool tiltAlert = false;
const int FILTER_SIZE = 5;
float accelBuffer[FILTER_SIZE] = {0};
int bufferIndex = 0;

// === Variável para Exibição no Servidor Web === //
String ultimaQueda = "Nenhuma queda registrada ainda";

// === Funções Auxiliares (Filtro, Ângulo, Data/Hora, Localização) === //

// Filtro de Média Móvel para suavizar os dados do acelerômetro
float filterAccel(float rawAccel) {
  accelBuffer[bufferIndex] = rawAccel;
  bufferIndex = (bufferIndex + 1) % FILTER_SIZE;
  float sum = 0;
  for (int i = 0; i < FILTER_SIZE; i++) {
    sum += accelBuffer[i];
  }
  return sum / FILTER_SIZE;
}

// Calcula o ângulo de inclinação
float calculateTiltAngle(float accelX, float accelY, float accelZ) {
  float tiltX = atan2(accelY, sqrt(accelX * accelX + accelZ * accelZ)) * 180 / PI;
  float tiltY = atan2(accelX, sqrt(accelY * accelY + accelZ * accelZ)) * 180 / PI;
  return max(abs(tiltX), abs(tiltY));
}

// Retorna a data e hora formatadas
String getFormattedDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Data/hora indisponível";
  }
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
  return String(buffer);
}

// Obtém a localização aproximada via IP
String getLocationFromIP() {
  HTTPClient http;
  http.setTimeout(5000);
  String url = "http://ip-api.com/json/?fields=lat,lon";
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(512);
    deserializeJson(doc, payload);
    float lat = doc["lat"] | 0.0;
    float lon = doc["lon"] | 0.0;
    return "http://maps.google.com/?q=" + String(lat, 6) + "," + String(lon, 6);
  }
  return "Localização não disponível";
}

// === Funções de Alerta (Telegram) === //

// <<< ALTERADO: A função agora aceita um booleano para indicar detecção de som.
void enviarAlertaQueda(bool somDetectado) {
  String localizacao = getLocationFromIP();
  String horario = getFormattedDateTime();
  ultimaQueda = horario;

  String mensagem = "\xf0\x9f\x9a\xa8 ALERTA DE QUEDA DETECTADA!\n";
  mensagem += "\xf0\x9f\x95\x92 Data/Hora: " + horario + "\n";
  
  // <<< NOVO: Adiciona a informação sobre o som à mensagem
  if (somDetectado) {
    mensagem += "\xf0\x9f\x94\x8a Som alto detectado: SIM\n";
  } else {
    mensagem += "\xf0\x9f\x94\x8a Som alto detectado: NÃO\n";
  }
  
  mensagem += "\xe2\x9a\xa0\xef\xb8\x8f Um idoso pode ter caído. Verifique imediatamente!";

  bot.sendMessage(CHAT_ID, mensagem);
  Serial.println("Alerta de QUEDA enviado para o Telegram!");
}

void enviarAlertaBotao() {
  String localizacao = getLocationFromIP();
  String horario = getFormattedDateTime();
  ultimaQueda = horario; 

  String mensagem = "\xf0\x9f\x9a\xa8 ALERTA DE BOTAO ACIONADO!\n";
  mensagem += "\xf0\x9f\x95\x92 Data/Hora: " + horario + "\n";
  mensagem += "\xe2\x9a\xa0\xef\xb8\x8f Um idoso pode estar em risco. Verifique imediatamente!";

  bot.sendMessage(CHAT_ID, mensagem);
  Serial.println("Alerta de BOTÃO enviado para o Telegram!");
}


// === Funções do Servidor Web === //

// Adiciona um novo alarme
void handleSetAlarme() {
  if (server.hasArg("hora") && server.hasArg("nome")) {
    if (qtdAlarmes < MAX_ALARMES) {
      alarmes[qtdAlarmes].hora = server.arg("hora");
      alarmes[qtdAlarmes].nome = server.arg("nome");
      alarmes[qtdAlarmes].ultimoDiaDisparo = -1; 
      qtdAlarmes++;
      server.sendHeader("Location", "/");
      server.send(303); 
    } else {
      server.send(200, "text/plain", "Limite de alarmes atingido.");
    }
  } else {
    server.send(400, "text/plain", "Parâmetros 'hora' ou 'nome' ausentes.");
  }
}

// Exclui um alarme existente
void handleDeleteAlarme() {
  if (server.hasArg("id")) {
    int id = server.arg("id").toInt();
    if (id >= 0 && id < qtdAlarmes) {
      for (int i = id; i < qtdAlarmes - 1; i++) {
        alarmes[i] = alarmes[i + 1];
      }
      qtdAlarmes--;
      server.sendHeader("Location", "/");
      server.send(303); 
    } else {
      server.send(400, "text/plain", "ID de alarme inválido.");
    }
  }
}


// Envia os dados de aceleração para o gráfico
void handleData() {
  String json = "[";
  for (int i = 0; i < MAX_DATA_POINTS; i++) {
    int index = (movimentoIndex + i) % MAX_DATA_POINTS;
    json += String(movimentoHistorico[index], 2);
    if (i < MAX_DATA_POINTS - 1) json += ",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

// Gera a página HTML principal
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="pt-br">
  <head>
    <meta charset="UTF-8">
    <title>Monitor e Alarmes</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
      body { font-family: Arial, sans-serif; background-color: #f4f4f4; margin: 0; padding: 20px; }
      .container { background-color: white; padding: 20px; border-radius: 10px; max-width: 700px; margin: auto; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
      h2, h3 { color: #333; }
      input[type='time'], input[type='text'], button { padding: 12px; font-size: 1em; margin: 8px 0; width: 100%; box-sizing: border-box; border-radius: 5px; border: 1px solid #ccc; }
      button { background-color: #007bff; color: white; border: none; cursor: pointer; transition: background-color 0.3s; }
      button:hover { background-color: #0056b3; }
      ul { list-style: none; padding: 0; }
      li { display: flex; justify-content: space-between; align-items: center; padding: 12px; background-color: #f9f9f9; margin: 8px 0; border-radius: 5px; }
      .delete-btn { background-color: #dc3545; color: white; padding: 8px 12px; text-decoration: none; border-radius: 5px; transition: background-color 0.3s; }
      .delete-btn:hover { background-color: #c82333; }
      canvas { max-width: 100%; height: auto; margin-top: 20px; }
      hr { border: 0; height: 1px; background: #ddd; margin: 20px 0; }
    </style>
  </head>
  <body>
    <div class="container">
      <h2>📈 Monitor de Quedas</h2>
      <p>Última queda registrada: <strong>)rawliteral" + ultimaQueda + R"rawliteral(</strong></p>
      <canvas id="grafico"></canvas>
      <hr>
      <h3>⏰ Alarmes Programados</h3>
      <ul>)rawliteral";

  if (qtdAlarmes == 0) {
    html += "<li>Nenhum alarme programado.</li>";
  } else {
    for (int i = 0; i < qtdAlarmes; i++) {
      html += "<li><span><strong>" + alarmes[i].nome + "</strong> - " + alarmes[i].hora + "</span>";
      html += "<a href='/delalarme?id=" + String(i) + "' class='delete-btn'>Excluir</a></li>";
    }
  }

  html += R"rawliteral(</ul>
      <hr>
      <h3>➕ Adicionar Novo Alarme</h3>
      <form action="/setalarme" method="get">
        <label for="hora">Horário:</label>
        <input type="time" id="hora" name="hora" required>
        <label for="nome">Nome do Alarme:</label>
        <input type="text" id="nome" name="nome" placeholder="Ex: Tomar remédio" required>
        <button type="submit">Adicionar Alarme</button>
      </form>
    </div>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <script>
      const ctx = document.getElementById('grafico').getContext('2d');
      const grafico = new Chart(ctx, {
        type: 'line',
        data: {
          labels: Array.from({length: 100}, (_, i) => i),
          datasets: [{
            label: 'Aceleração (g)',
            data: Array(100).fill(0),
            borderColor: 'rgba(0, 123, 255, 1)',
            borderWidth: 2,
            fill: false,
            tension: 0.4
          }]
        },
        options: {
          responsive: true,
          animation: false,
          scales: { y: { beginAtZero: true, title: { display: true, text: 'Aceleração (g)' } } }
        }
      });

      function atualizarGrafico() {
        fetch('/dados')
          .then(res => res.json())
          .then(data => {
            grafico.data.datasets[0].data = data;
            grafico.update();
          });
      }

      setInterval(atualizarGrafico, 1000);
      atualizarGrafico();
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

// === Função de Configuração (setup) === //
void setup() {
  Serial.begin(115200);

  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_ALARME, OUTPUT);
  pinMode(BOTAO_PIN, INPUT_PULLUP);
  pinMode(MICROFONE_PIN, INPUT); // <<< NOVO: Configura o pino do microfone como entrada
  digitalWrite(BUZZER, LOW);
  digitalWrite(LED_ALARME, LOW);

  Wire.begin();
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("Erro: MPU6050 não conectado!");
    while (1);
  }
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_4);

  Serial.println("Iniciando conexão Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    delay(500);
    Serial.print(".");
    yield();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Wi-Fi Conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Falha ao obter horário do servidor NTP.");
    } else {
      Serial.println("Horário NTP sincronizado com sucesso.");
    }
    
    client.setInsecure();

    server.on("/", handleRoot);
    server.on("/dados", handleData);
    server.on("/setalarme", handleSetAlarme);
    server.on("/delalarme", handleDeleteAlarme);
    server.begin();
    Serial.println("Servidor web iniciado!");

  } else {
    Serial.println("\n❌ Falha na conexão Wi-Fi. Reiniciando...");
    delay(3000);
    ESP.restart();
  }
}

// === Função Principal (loop) === //
void loop() {
  server.handleClient();

  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  float accelX = ax / 8192.0; 
  float accelY = ay / 8192.0;
  float accelZ = az / 8192.0;
  
  float totalAccel = sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);
  float filteredAccel = filterAccel(totalAccel);
  float tiltAngle = calculateTiltAngle(accelX, accelY, accelZ);

  movimentoHistorico[movimentoIndex] = filteredAccel;
  movimentoIndex = (movimentoIndex + 1) % MAX_DATA_POINTS;

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char horaAtual[6];
    strftime(horaAtual, sizeof(horaAtual), "%H:%M", &timeinfo);
    int diaDoAnoAtual = timeinfo.tm_yday;

    for (int i = 0; i < qtdAlarmes; i++) {
      if (String(horaAtual) == alarmes[i].hora && alarmes[i].ultimoDiaDisparo != diaDoAnoAtual) {
        Serial.println("⏰ Alarme disparado: " + alarmes[i].nome);
        
        digitalWrite(LED_ALARME, HIGH);
        digitalWrite(BUZZER, HIGH);
        delay(5000); 
        digitalWrite(LED_ALARME, LOW);
        digitalWrite(BUZZER, LOW);
        
        alarmes[i].ultimoDiaDisparo = diaDoAnoAtual;
        break; 
      }
    }
  }

  bool caiu = false;

  if (!fallDetected && filteredAccel < FREE_FALL_THRESHOLD) {
    fallStartTime = millis();
    fallDetected = true;
    Serial.println("Possível queda livre detectada...");
  }

  if (fallDetected) {
    if (filteredAccel > IMPACT_THRESHOLD) {
      Serial.println("ALERTA DE QUEDA CONFIRMADA: Impacto detectado!");
      caiu = true;
      fallDetected = false;
    } 
    else if (millis() - fallStartTime > FALL_TIME_WINDOW) {
      Serial.println("Queda livre sem impacto. Cancelando alerta.");
      fallDetected = false;
    }
  }

  if (tiltAngle > TILT_THRESHOLD) {
    if (!tiltAlert) {
      tiltStartTime = millis();
      tiltAlert = true;
    } else if (millis() - tiltStartTime > TILT_TIME_THRESHOLD) {
      Serial.println("ALERTA: Inclinação prolongada! Possível queda.");
      caiu = true; 
      tiltAlert = false;
    }
  } else {
    tiltAlert = false;
  }

  int estadoBotao = digitalRead(BOTAO_PIN);

  if (caiu) {
    digitalWrite(LED_VERMELHO, HIGH);
    
    // Verifica o estado do microfone antes de enviar o alerta
    // A maioria dos módulos de som digital envia LOW quando o som é detectado.
    int estadoSom = analogRead(MICROFONE_PIN);
    bool somAltoDetectado;
    if (estadoSom <= 900){
      somAltoDetectado = 1;
    }else{
      somAltoDetectado = 0;
    }
     
    
    enviarAlertaQueda(somAltoDetectado); // <<< ALTERADO: Passa o resultado para a função de alerta
    
    delay(10000); 
    digitalWrite(LED_VERMELHO, LOW);
  } else if (estadoBotao == LOW) { 
    digitalWrite(LED_VERMELHO, HIGH);
    enviarAlertaBotao();
    delay(10000); 
    digitalWrite(LED_VERMELHO, LOW);
  }

  Serial.print("Aceleração: ");
  Serial.print(filteredAccel, 2);
  Serial.print(" g | Ângulo: ");
  Serial.print(tiltAngle, 1);
  Serial.println("°");

  // * ALTERAÇÃO CRÍTICA PARA A SENSIBILIDADE *
  // Delay ajustado para 50ms para aumentar a frequência de leitura do sensor.
  delay(50); 
}
