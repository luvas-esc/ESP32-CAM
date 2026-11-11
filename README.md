# ESP32-CAM para Google Drive

Projeto com ESP32-CAM que captura imagens periodicamente e envia os arquivos
para o Google Drive usando um Web App do Google Apps Script.

## Recursos

- Captura automática a cada 10 minutos.
- Envio da imagem em formato JPEG, codificada em Base64.
- Comunicação HTTPS com o Google Apps Script.
- Uso da pasta `ESP32-CAM` no Google Drive.
- Flash LED não é acionado pelo programa.

## Hardware

- ESP32-CAM AI-Thinker (ou placa compatível com a pinagem configurada).
- Fonte de alimentação adequada para a ESP32-CAM.

## Bibliotecas

Além do suporte à placa ESP32 na Arduino IDE, o projeto utiliza:

- `WiFi.h`
- `WiFiClientSecure.h`
- `esp_camera.h`
- `Base64.h`

As três primeiras fazem parte do ecossistema ESP32/Arduino. Instale uma
biblioteca compatível com `Base64.h` caso ela não esteja disponível no seu
ambiente.

## Configuração

1. Abra `ESP32_CAM_Google_Drive.ino` na Arduino IDE.
2. Preencha `WIFI_SSID` com o nome da sua rede Wi-Fi.
3. Preencha `WIFI_PASSWORD` com a senha da sua rede.
4. Preencha `DEPLOYMENT_ID` com o ID da implantação do seu Web App do Google Apps Script.
5. Confira `NOME_PASTA` para definir a pasta de destino no Drive.
6. Selecione a placa correspondente à sua ESP32-CAM e envie o programa.

**Importante:** não publique suas credenciais reais, o ID privado da implantação
ou outros dados de acesso. Mantenha os espaços reservados no repositório público.

## Intervalo de captura

O intervalo é configurado pela constante:

```cpp
const unsigned long INTERVALO_FOTO = 10UL * 60UL * 1000UL;
```

A fórmula converte minutos em milissegundos:

- `10` = minutos
- `60` = segundos por minuto
- `1000` = milissegundos por segundo

Para alterar o intervalo, substitua o primeiro número.

## Google Apps Script

O projeto depende de um Web App do Google Apps Script que receba o corpo
Base64 da requisição POST e crie o arquivo no Google Drive. O script deve estar
implantado com permissões compatíveis com o uso pretendido.

## Observações de segurança

O código mantém `client.setInsecure()` para compatibilidade com a configuração
original. Isso desativa a validação do certificado TLS do servidor. Para uso
em produção, prefira validar o certificado/autoridade certificadora.

Não compartilhe arquivos contendo SSID, senha Wi-Fi ou IDs de implantação.

# ATENÇÃO
##### Esse projeto foi feito com grande auxílio de Inteligência Artificial.  
##### Fiz com a intenção de usar a placa ESP32-Cam para um propósito prático 
##### e pretendo me aprofundar mais em C++ e microcontroladores futuramente.       
