/*
 * Projeto: ESP32-CAM para Google Drive
 * Descrição:
 *   Captura uma imagem com a ESP32-CAM e envia o arquivo para uma pasta
 *   do Google Drive por meio de um Web App do Google Apps Script.
 *
 * Antes de compilar:
 *   1. Informe o nome e a senha da sua rede Wi-Fi.
 *   2. Informe o ID da implantação do seu Google Apps Script.
 *   3. Confira se o modelo da placa selecionado na Arduino IDE é AI Thinker
 *      ESP32-CAM (ou o modelo correspondente ao seu hardware).
 *
 * Segurança:
 *   Não publique suas credenciais reais. Mantenha os campos abaixo como
 *   espaços reservados no GitHub e configure seus dados apenas localmente.
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"
#include "esp_camera.h"

// ============================================================================
// 1. PINAGEM DA CÂMERA - MODELO AI-THINKER ESP32-CAM
// ============================================================================

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ============================================================================
// 2. CONFIGURAÇÕES DO PROJETO
// ============================================================================

// Credenciais da rede Wi-Fi.
// Substitua os textos de exemplo somente na sua cópia local.
const char* WIFI_SSID = "SEU_WIFI";
const char* WIFI_PASSWORD = "SUA_SENHA";

// ID da implantação do Web App do Google Apps Script.
// Não compartilhe seu ID real em repositórios públicos.
const char* DEPLOYMENT_ID = "SEU_DEPLOYMENT_ID";

// Nome da pasta que será criada/utilizada no Google Drive.
const char* NOME_PASTA = "ESP32-CAM";

// Endereço do servidor do Google Apps Script.
const char* HOST = "script.google.com";

// Intervalo entre as capturas: 10 minutos.
// Conversão: minutos * 60 segundos * 1000 milissegundos.
const unsigned long INTERVALO_FOTO = 10UL * 60UL * 1000UL;

// Momento da última captura/envio programado.
unsigned long instanteUltimaFoto = 0;

// Cliente HTTPS utilizado para a comunicação com o Google.
WiFiClientSecure client;

// ============================================================================
// 3. TESTE DE CONEXÃO COM O GOOGLE APPS SCRIPT
// ============================================================================

/*
 * testarConexao()
 * Tenta abrir uma conexão HTTPS com o servidor do Google Apps Script.
 * A função continua tentando até conseguir estabelecer a conexão.
 *
 * Observação: setInsecure() desativa a validação do certificado TLS.
 * É mantido aqui para compatibilidade com a configuração original.
 */
void testarConexao() {
  client.setInsecure();

  while (!client.connect(HOST, 443)) {
    Serial.println("Falha ao conectar ao servidor. Tentando novamente...");
    client.stop();
    delay(1000);
  }

  Serial.println("Conexao com script.google.com estabelecida.");
  client.stop();
}

// ============================================================================
// 4. CAPTURA E ENVIO DA IMAGEM PARA O GOOGLE DRIVE
// ============================================================================

/*
 * enviarFotoParaDrive()
 * Realiza as seguintes etapas:
 *   1. Conecta ao servidor do Google Apps Script.
 *   2. Descarta alguns quadros iniciais para estabilizar a câmera.
 *   3. Captura a imagem em JPEG.
 *   4. Codifica a imagem em Base64 e envia por HTTP POST.
 *   5. Aguarda a resposta do servidor.
 *
 * O Apps Script deve estar configurado para receber o conteúdo Base64
 * enviado no corpo da requisição (e.postData.contents).
 */
void enviarFotoParaDrive() {
  Serial.println();
  Serial.println("Iniciando captura e envio da foto...");

  client.setInsecure();

  // Abre a conexão HTTPS com o Google Apps Script.
  if (!client.connect(HOST, 443)) {
    Serial.println("Nao foi possivel conectar ao servidor.");
    client.stop();
    return;
  }

  // Descarta três quadros iniciais para estabilizar a exposição da câmera.
  for (int i = 0; i < 3; i++) {
    camera_fb_t* quadroTemporario = esp_camera_fb_get();

    if (quadroTemporario == nullptr) {
      Serial.println("Falha ao preparar a camera.");
      client.stop();
      return;
    }

    esp_camera_fb_return(quadroTemporario);
    delay(200);
  }

  // Captura o quadro que será enviado ao Google Drive.
  camera_fb_t* foto = esp_camera_fb_get();

  if (foto == nullptr) {
    Serial.println("Falha ao capturar a foto.");
    client.stop();
    return;
  }

  Serial.println("Foto capturada com sucesso.");
  Serial.println("Tamanho: " + String(foto->len) + " bytes");

  // Monta o caminho do Web App e informa a pasta de destino.
  String url = "/macros/s/" + String(DEPLOYMENT_ID) +
               "/exec?folder=" + String(NOME_PASTA);

  // Define o tamanho dos blocos usados na codificação Base64.
  // O valor 3000 é múltiplo de 3, evitando quebra entre blocos.
  const int tamanhoFoto = foto->len;
  const int tamanhoBloco = 3000;
  const int tamanhoBufferBase64 = base64_enc_len(tamanhoBloco);
  char bufferBase64[tamanhoBufferBase64 + 1];

  // Calcula o tamanho total do conteúdo Base64 para o cabeçalho HTTP.
  const int tamanhoTotalBase64 = base64_enc_len(tamanhoFoto);

  // Envia os cabeçalhos da requisição HTTP POST.
  client.println("POST " + url + " HTTP/1.1");
  client.println("Host: " + String(HOST));
  client.println("Content-Type: text/plain");
  client.print("Content-Length: ");
  client.println(tamanhoTotalBase64);
  client.println("Connection: close");
  client.println();

  // Converte a imagem para Base64 e envia um bloco por vez.
  char* dadosFoto = (char*)foto->buf;

  for (int posicao = 0; posicao < tamanhoFoto; posicao += tamanhoBloco) {
    int bytesNesteBloco = min(tamanhoFoto - posicao, tamanhoBloco);
    int bytesBase64 = base64_encode(
      bufferBase64,
      dadosFoto,
      bytesNesteBloco
    );

    client.write((uint8_t*)bufferBase64, bytesBase64);
    dadosFoto += bytesNesteBloco;

    Serial.print(".");
  }

  // Libera o buffer da imagem depois que todos os dados foram enviados.
  esp_camera_fb_return(foto);

  // Aguarda a resposta HTTP do servidor por até 60 segundos.
  Serial.println();
  Serial.println("Aguardando resposta do servidor...");

  unsigned long inicioEspera = millis();

  while (millis() - inicioEspera < 60000UL) {
    while (client.available()) {
      char caractere = client.read();
      Serial.write(caractere);

      // Reinicia o contador enquanto ainda estiver chegando resposta.
      inicioEspera = millis();
    }

    // Encerra a espera quando a conexão foi fechada e não há dados pendentes.
    if (!client.connected() && !client.available()) {
      break;
    }

    delay(10);
  }

  Serial.println();
  Serial.println("Fim da resposta do servidor.");

  // Fecha a conexão HTTPS.
  client.stop();
}

// ============================================================================
// 5. INICIALIZAÇÃO DO ESP32-CAM
// ============================================================================

/*
 * setup()
 * Executada uma única vez quando o ESP32 é ligado ou reiniciado.
 * Inicializa a comunicação serial, conecta ao Wi-Fi, configura a câmera
 * e testa a conexão com o servidor.
 */
void setup() {
  // Inicializa a comunicação serial para mensagens de diagnóstico.
  Serial.begin(115200);
  delay(1000);

  // Desativa o detector de queda de tensão, conforme o projeto original.
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // Configura o ESP32 como estação Wi-Fi e inicia a conexão.
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Conectando ao Wi-Fi");

  // Aguarda até 20 segundos pela conexão Wi-Fi.
  int tentativasRestantes = 40;

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);

    tentativasRestantes--;

    if (tentativasRestantes <= 0) {
      Serial.println();
      Serial.println("Nao foi possivel conectar ao Wi-Fi. Reiniciando...");
      delay(1000);
      ESP.restart();
    }
  }

  Serial.println();
  Serial.println("Wi-Fi conectado com sucesso.");

  // Preenche a estrutura com os pinos e parâmetros da câmera.
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Ajusta resolução, qualidade e quantidade de buffers conforme a PSRAM.
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 8;
    config.fb_count = 1;
  }

  // Inicializa a câmera. Reinicia a placa se ocorrer algum erro.
  esp_err_t erroCamera = esp_camera_init(&config);

  if (erroCamera != ESP_OK) {
    Serial.printf(
      "Falha ao inicializar a camera. Codigo: 0x%x\n",
      erroCamera
    );
    delay(1000);
    ESP.restart();
  }

  // Define a resolução final utilizada nas capturas.
  sensor_t* sensor = esp_camera_sensor_get();
  sensor->set_framesize(sensor, FRAMESIZE_SXGA);

  Serial.println("Camera inicializada com sucesso.");

  // Verifica se o servidor do Google Apps Script está acessível.
  testarConexao();

  // Começa a contar o intervalo depois que a inicialização termina.
  instanteUltimaFoto = millis();

  Serial.println("O ESP32-CAM enviara uma foto a cada 10 minutos.");
}

// ============================================================================
// 6. ROTINA PRINCIPAL
// ============================================================================

/*
 * loop()
 * Executada continuamente após setup().
 * Verifica o tempo decorrido e chama a função de captura/envio quando
 * o intervalo configurado em INTERVALO_FOTO é atingido.
 */
void loop() {
  unsigned long agora = millis();

  // A subtração funciona corretamente mesmo quando millis() sofre overflow.
  if (agora - instanteUltimaFoto >= INTERVALO_FOTO) {
    instanteUltimaFoto = agora;
    enviarFotoParaDrive();
  }

  // Pequena pausa para evitar que o loop rode sem necessidade.
  delay(100);
}
