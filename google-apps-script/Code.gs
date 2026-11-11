/**
 * Projeto: ESP32-CAM para Google Drive
 *
 * Recebe uma imagem em Base64 enviada pela ESP32-CAM por meio de HTTP POST
 * e salva o arquivo JPG no Google Drive.
 *
 * A ESP32-CAM envia o nome da pasta pelo parâmetro "folder" na URL.
 */

// Resposta simples para testar se o Web App está acessível.
function doGet(e) {
  return ContentService.createTextOutput(
    "ESP32-CAM - Conexao OK"
  );
}

// Recebe a imagem enviada pela ESP32-CAM.
function doPost(e) {
  try {
    // Verifica se o conteúdo da requisição foi recebido.
    if (!e || !e.postData || !e.postData.contents) {
      throw new Error("Nenhuma imagem foi recebida.");
    }

    // Define data e horário para organizar os arquivos.
    const fusoHorario = Session.getScriptTimeZone();
    const agora = new Date();

    // Cria o nome do arquivo no formato AAAAMMDD-HHMMSS.jpg.
    const nomeArquivo =
      Utilities.formatDate(agora, fusoHorario, "yyyyMMdd-HHmmss") + ".jpg";

    // Cria uma subpasta para cada dia.
    const nomeSubpasta =
      Utilities.formatDate(agora, fusoHorario, "yyyyMMdd");

    // Lê o nome da pasta principal enviado pela ESP32-CAM.
    // Usa "ESP32-CAM" como padrão caso o parâmetro não seja informado.
    const nomePasta = e.parameter.folder || "ESP32-CAM";

    // Converte o texto Base64 recebido em bytes.
    const dadosImagem = Utilities.base64Decode(
      e.postData.contents
    );

    // Cria o arquivo JPG a partir dos bytes da imagem.
    const blob = Utilities.newBlob(
      dadosImagem,
      "image/jpeg",
      nomeArquivo
    );

    // Procura a pasta principal no Google Drive.
    // Se ela não existir, cria uma nova.
    const pastas = DriveApp.getFoldersByName(nomePasta);

    const pastaPrincipal = pastas.hasNext()
      ? pastas.next()
      : DriveApp.createFolder(nomePasta);

    // Procura a subpasta do dia dentro da pasta principal.
    // Se ela não existir, cria uma nova.
    const subpastas = pastaPrincipal.getFoldersByName(nomeSubpasta);

    const subpasta = subpastas.hasNext()
      ? subpastas.next()
      : pastaPrincipal.createFolder(nomeSubpasta);

    // Salva a imagem na subpasta correspondente.
    const arquivo = subpasta.createFile(blob);

    // Retorna uma mensagem de confirmação.
    return ContentService.createTextOutput(
      "Done: " + arquivo.getName()
    );

  } catch (erro) {
    // Registra o erro no histórico de execução do Apps Script.
    console.error("Erro ao receber imagem: " + erro.message);

    // Retorna a mensagem de erro.
    return ContentService.createTextOutput(
      "Erro: " + erro.message
    );
  }
}
