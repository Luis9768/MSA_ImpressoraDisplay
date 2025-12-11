#include "NetworkManager.h"
#include "Config.h"
#include "StorageManager.h"
#include "DisplayManager.h" 
#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>

WebServer server(80);
DNSServer dnsServer;
uint8_t broadcastAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// =================================================================================
// HTML PREMIUM - RESPONSIVO E COLORIDO
// =================================================================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>MSA Smart System</title>
  <style>
    /* --- Base --- */
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Segoe UI', Roboto, sans-serif; -webkit-tap-highlight-color: transparent; }
    
    body { 
      background: linear-gradient(135deg, #003366 0%, #00d2ff 100%); /* Gradiente Azul Vibrante */
      min-height: 100vh;
      display: flex; 
      justify-content: center; 
      align-items: center; 
      padding: 20px;
    }

    /* --- Cartão Principal --- */
    .container {
      background: rgba(255, 255, 255, 0.95); /* Quase branco */
      width: 100%;
      max-width: 420px;
      padding: 30px;
      border-radius: 20px;
      box-shadow: 0 20px 50px rgba(0,0,0,0.2);
      backdrop-filter: blur(10px); /* Efeito de vidro */
      border-top: 6px solid #ff9900; /* Detalhe Laranja MSA */
    }

    /* --- Cabeçalho --- */
    .header { text-align: center; margin-bottom: 30px; }
    
    .logo {
      font-size: 32px;
      font-weight: 900;
      color: #003366;
      letter-spacing: -1px;
      margin-bottom: 5px;
    }
    .logo span { color: #ff9900; } /* O 'A' ou detalhe em laranja */
    
    .subtitle { color: #666; font-size: 14px; font-weight: 500; }

    /* --- Inputs --- */
    .form-group { margin-bottom: 20px; position: relative; }
    
    .label { 
      font-size: 12px; 
      font-weight: 700; 
      color: #888; 
      margin-bottom: 8px; 
      display: block; 
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }

    .input-wrapper {
      position: relative;
      display: flex;
      align-items: center;
    }

    /* Ícones SVG Coloridos */
    .icon {
      position: absolute;
      left: 15px;
      width: 20px;
      height: 20px;
      opacity: 0.8;
    }

    input {
      width: 100%;
      padding: 15px 15px 15px 45px; /* Espaço para o ícone */
      border: 2px solid #eee;
      border-radius: 12px;
      font-size: 16px; /* Tamanho bom para celular não dar zoom */
      background: #f9f9f9;
      transition: all 0.3s ease;
      color: #333;
      font-weight: 600;
    }

    input:focus {
      border-color: #00d2ff;
      background: #fff;
      box-shadow: 0 0 0 4px rgba(0, 210, 255, 0.1);
      outline: none;
    }

    /* Cores específicas para bordas de foco se quiser variar */
    .grp-qtd input:focus { border-color: #28a745; box-shadow: 0 0 0 4px rgba(40, 167, 69, 0.1); }
    .grp-id input:focus { border-color: #ff9900; box-shadow: 0 0 0 4px rgba(255, 153, 0, 0.1); }

    /* --- Botão --- */
    button {
      width: 100%;
      padding: 18px;
      background: linear-gradient(90deg, #003366, #0055aa);
      color: white;
      border: none;
      border-radius: 12px;
      font-size: 18px;
      font-weight: 700;
      cursor: pointer;
      box-shadow: 0 10px 20px rgba(0, 51, 102, 0.2);
      transition: transform 0.2s;
      margin-top: 10px;
    }

    button:active { transform: scale(0.96); }

    /* --- Rodapé --- */
    .footer { text-align: center; margin-top: 25px; font-size: 12px; color: #aaa; }

  </style>
</head>
<body>

  <div class="container">
    <div class="header">
      <div class="logo">MS<span>A</span></div>
      <div class="subtitle">Smart Factory Control</div>
    </div>

    <form action="/salvar" method="GET">
      
      <!-- ID AUTOMATICO (REMOVIDO INPUT) -->

      <div class="form-group">
        <span class="label">Código do Produto</span>
        <div class="input-wrapper">
          <svg class="icon" viewBox="0 0 24 24" fill="none" stroke="#003366" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20.59 13.41l-7.17 7.17a2 2 0 0 1-2.83 0L2 12V2h10l8.59 8.59a2 2 0 0 1 0 2.82z"></path><line x1="7" y1="7" x2="7.01" y2="7"></line></svg>
          <input type="text" name="cod" placeholder="ABC-123" required>
        </div>
      </div>

      <div class="form-group">
        <span class="label">Descrição</span>
        <div class="input-wrapper">
          <svg class="icon" viewBox="0 0 24 24" fill="none" stroke="#6f42c1" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="17" y1="10" x2="3" y2="10"></line><line x1="21" y1="6" x2="3" y2="6"></line><line x1="21" y1="14" x2="3" y2="14"></line><line x1="17" y1="18" x2="3" y2="18"></line></svg>
          <input type="text" name="desc" placeholder="Nome do item" required>
        </div>
      </div>

      <div class="form-group grp-qtd">
        <span class="label">Quantidade na Caixa</span>
        <div class="input-wrapper">
          <svg class="icon" viewBox="0 0 24 24" fill="none" stroke="#28a745" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"></path><polyline points="3.27 6.96 12 12.01 20.73 6.96"></polyline><line x1="12" y1="22.08" x2="12" y2="12"></line></svg>
          <input type="number" name="qtd" placeholder="100" required>
        </div>
      </div>

      <div class="form-group">
        <span class="label">Código de Barras (Opcional)</span>
        <div class="input-wrapper">
          <svg class="icon" viewBox="0 0 24 24" fill="none" stroke="#333" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M3 5v14"/><path d="M8 5v14"/><path d="M12 5v14"/><path d="M17 5v14"/><path d="M21 5v14"/></svg>
          <input type="text" name="bar" placeholder="789...">
        </div>
      </div>

      <button type="submit">SALVAR CADASTRO</button>
    </form>

    <div class="footer">
      <a href="/lista" style="color: #003366; text-decoration: none; font-weight: bold; font-size: 14px;">GERENCIAR PRODUTOS</a><br><br>
      MSA Technology V2.1
    </div>
  </div>

</body>
</html>
)rawliteral";

// --- FUNÇÕES DE REDE (Inalteradas) ---

void enviarReceitaParaSlaves(Receita r) {
    PacoteRede pct;
    pct.tipo = 1; 
    pct.dados = r;
    esp_now_send(broadcastAddr, (uint8_t *) &pct, sizeof(pct));
}

void enviarResetParaSlaves() {
    PacoteRede pct;
    pct.tipo = 2; // TIPO 2 = RESET TOTAL
    // Zeramos os dados só por segurança
    memset(&pct.dados, 0, sizeof(Receita));
    esp_now_send(broadcastAddr, (uint8_t *) &pct, sizeof(pct));
}

void handleRoot() { server.send(200, "text/html", index_html); }

void handleSave() {
    Receita r;
    
    // AUTO ID
    int proximoId = getTotalReceitas() + 1;
    if (proximoId > MAX_RECEITAS) {
        server.send(200, "text/html", "<h1>Erro: Memoria Cheia!</h1><a href='/'>Voltar</a>");
        return;
    }
    r.id = proximoId;

    r.quantidade = server.arg("qtd").toInt();
    strncpy(r.codigo, server.arg("cod").c_str(), 15);
    strncpy(r.descricao, server.arg("desc").c_str(), 31);
    strncpy(r.barcode, server.arg("bar").c_str(), 15);
    r.ativa = true;

    salvarReceitaMemoria(r);
    enviarReceitaParaSlaves(r);
    atualizarListaProdutos(); 

    // Página de Sucesso Bonita
    String sucessoHtml = R"rawliteral(
      <!DOCTYPE html><html><head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        body{background: linear-gradient(135deg, #003366 0%, #00d2ff 100%); font-family:'Segoe UI', sans-serif; height:100vh; display:flex; justify-content:center; align-items:center; margin:0;}
        .card{background:white; padding:40px; border-radius:20px; text-align:center; box-shadow:0 20px 50px rgba(0,0,0,0.2); width:90%; max-width:350px;}
        .icon-check { color: #28a745; font-size: 60px; margin-bottom: 20px; display:block; }
        h1{color:#333; font-size: 24px; margin:0 0 10px 0;}
        p{color:#666; margin-bottom: 30px;}
        a{display:block; width:100%; text-decoration:none; background:#003366; color:white; padding:15px; border-radius:10px; font-weight:bold;}
      </style></head><body>
      <div class="card">
        <span class="icon-check">&#10004;</span>
        <h1>Sucesso!</h1>
        <p>Produto cadastrado.</p>
        <a href='/'>NOVO CADASTRO</a>
      </div></body></html>
    )rawliteral";

    server.send(200, "text/html", sucessoHtml);
}



void handleList() {
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="pt-BR">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Gerenciar Produtos</title>
      <style>
        body { background: linear-gradient(135deg, #003366 0%, #00d2ff 100%); font-family: 'Segoe UI', sans-serif; min-height: 100vh; padding: 20px; }
        .container { max-width: 600px; margin: 0 auto; }
        .card { background: white; border-radius: 15px; padding: 20px; margin-bottom: 15px; display: flex; justify-content: space-between; align-items: center; box-shadow: 0 4px 15px rgba(0,0,0,0.1); }
        .info { flex-grow: 1; }
        .code { font-weight: bold; color: #003366; font-size: 18px; }
        .desc { color: #666; font-size: 14px; }
        .btn-del { background: #dc3545; color: white; border: none; padding: 10px 20px; border-radius: 8px; cursor: pointer; text-decoration: none; font-weight: bold; }
        .header { text-align: center; color: white; margin-bottom: 20px; }
        .btn-back { display: block; width: 100%; text-align: center; background: rgba(255,255,255,0.2); color: white; padding: 15px; border-radius: 10px; text-decoration: none; font-weight: bold; margin-bottom: 20px; }
      </style>
    </head>
    <body>
      <div class="container">
        <div class="header">
            <h1>Produtos Ativos</h1>
        </div>
        <a href="/" class="btn-back">VOLTAR PARA CADASTRO</a>
        
        <a href="/confirmar_reset" class="btn-back" style="background: #dc3545;">LIMPAR TUDO (RESET V3)</a>
    )rawliteral";

    int total = getTotalReceitas();
    bool temItem = false;

    for (int i = 1; i <= total; i++) {
        Receita r = carregarReceitaMemoria(i);
        // Serial.printf("Item %d: Ativa=%d\n", r.id, r.ativa); // Debug
        if (r.ativa) {
            temItem = true;
            char itemBuf[512];
            sprintf(itemBuf, 
                "<div class='card'>"
                "<div class='info'><div class='code'>%s</div><div class='desc'>ID: %d | %s</div></div>"
                "<form action='/deletar' method='POST' style='display:inline; margin:0;'>"
                "<input type='hidden' name='id' value='%d'>"
                "<button type='submit' class='btn-del'>EXCLUIR</button>"
                "</form>"
                "</div>", 
                r.codigo, r.id, r.descricao, r.id);
            html += itemBuf;
        }
    }

    if (!temItem) {
        html += "<div style='text-align:center; color:white;'>Nenhum produto cadastrado.</div>";
    }

    // DEBUG FOOTER
    char debugBuf[128];
    sprintf(debugBuf, "<div style='text-align:center; color:#aaa; font-size:10px; margin-top:20px;'>Total RAM: %d | Heap: %d | V: RAM_V2</div>", 
        getTotalReceitas(), ESP.getFreeHeap());
    html += debugBuf;

    html += "</div></body></html>";
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");
    server.send(200, "text/html", html);
}

void handleDelete() {
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="pt-BR">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Produto Excluido</title>
      <style>
        body { background: linear-gradient(135deg, #003366 0%, #00d2ff 100%); font-family: 'Segoe UI', sans-serif; min-height: 100vh; padding: 20px; display: flex; align-items: center; justify-content: center; }
        .container { max-width: 400px; width: 100%; text-align: center; }
        .card { background: white; border-radius: 15px; padding: 30px; box-shadow: 0 10px 25px rgba(0,0,0,0.2); }
        h1 { color: #003366; margin-bottom: 10px; }
        p { color: #666; font-size: 16px; margin-bottom: 25px; }
        .btn { display: inline-block; width: 100%; padding: 15px; background: #28a745; color: white; border-radius: 10px; text-decoration: none; font-weight: bold; font-size: 16px; box-sizing: border-box; }
        .btn:hover { opacity: 0.9; }
      </style>
    </head>
    <body>
      <div class="container">
        <div class="card">
    )rawliteral";

    if (server.hasArg("id")) {
        int id = server.arg("id").toInt();
        
        // 1. Executa Exclusao
        desativarReceita(id);
        
        // 2. Verifica Status Pós-Exclusão
        Receita r = carregarReceitaMemoria(id);
        
        // Avisa os slaves
        enviarReceitaParaSlaves(r);
        
        // Atualiza a tela do Master
        atualizarListaProdutos();

        html += "<h1>Produto Excluido!</h1>";
        html += "<p>O produto <strong>ID " + String(id) + "</strong> foi removido com sucesso.</p>";
        html += "<a href='/lista' class='btn'>VOLTAR PARA LISTA</a>";
        
    } else {
        html += "<h1 style='color:red'>Erro</h1>";
        html += "<p>Nenhum ID fornecido.</p>";
        html += "<a href='/lista' class='btn' style='background:#666'>VOLTAR</a>";
    }

    html += "</div></div></body></html>";

    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.send(200, "text/html", html);
}

void handleReset() {
    enviarResetParaSlaves(); // AVISA OS SLAVES ANTES DE APAGAR TUDO
    limparMemoria();
    Serial.println(">>> WEB REQUEST: MEMORIA LIMPA! REINICIANDO...");
    server.send(200, "text/html", "<h1>Memoria Limpa! Reiniciando...</h1><p>Aguarde 5 segundos e recarregue a pagina.</p><script>setTimeout(function(){window.location.href='/';}, 5000);</script>");
    delay(1000);
    ESP.restart();
}

void handleConfirmReset() {
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="pt-BR">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Confirmar Reset</title>
      <style>
        body { background: #003366; font-family: sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; padding: 20px; text-align: center; color: white; }
        .card { background: white; color: #333; padding: 30px; border-radius: 15px; width: 100%; max-width: 400px; box-shadow: 0 10px 25px rgba(0,0,0,0.5); }
        h1 { color: #dc3545; margin-top: 0; }
        p { font-size: 18px; margin-bottom: 30px; }
        .btn { display: block; width: 100%; padding: 15px; border-radius: 10px; font-weight: bold; font-size: 18px; cursor: pointer; text-decoration: none; margin-bottom: 10px; border: none; }
        .btn-danger { background: #dc3545; color: white; }
        .btn-secondary { background: #6c757d; color: white; }
      </style>
    </head>
    <body>
      <div class="card">
        <h1>ATENÇÃO!</h1>
        <p>Você tem certeza que deseja APAGAR TODOS os produtos e reiniciar o sistema?</p>
        <p><strong>Essa ação não pode ser desfeita.</strong></p>
        
        <form action="/reset" method="POST">
            <button type="submit" class="btn btn-danger">SIM, APAGAR TUDO</button>
        </form>
        
        <a href="/lista" class="btn btn-secondary">CANCELAR</a>
      </div>
    </body>
    </html>
    )rawliteral";
    server.send(200, "text/html", html);
}

void setupNetwork() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(WIFI_SSID, WIFI_PASS);
    dnsServer.start(53, "*", WiFi.softAPIP());

    if (esp_now_init() != ESP_OK) return;
    
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastAddr, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    // --- OTA CONFIG ---
    ArduinoOTA.setHostname(HOSTNAME);
    
    // Callbacks de progresso/erro (Opcional, mas util para debug Serial)
    ArduinoOTA.onStart([]() { Serial.println("OTA Start"); });
    ArduinoOTA.onEnd([]() { Serial.println("\nOTA End"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();
    Serial.println("OTA Master Iniciado (Porta IP)");

    server.on("/", handleRoot);
    server.on("/salvar", handleSave);
    server.on("/lista", handleList);
    server.on("/deletar", handleDelete);
    server.on("/confirmar_reset", handleConfirmReset);
    server.on("/reset", handleReset);
    server.on("/debug", []() {
        String html = "<h1>Debug NVS</h1>";
        html += "<p>RAM Total: " + String(getTotalReceitas()) + "</p>";
        
        Preferences p; 
        p.begin("msa_v3", true);
        int nvsTotal = p.getInt("total", -1);
        int versao = p.getInt("versao_db", -1);
        p.end();
        
        html += "<p>NVS Total: " + String(nvsTotal) + "</p>";
        html += "<p>NVS Versao: " + String(versao) + "</p>";
        html += "<a href='/'>Voltar</a>";
        server.send(200, "text/html", html);
    });
    server.onNotFound(handleRoot); 
    server.begin();
}

void loopNetwork() {
    dnsServer.processNextRequest();
    server.handleClient();
    ArduinoOTA.handle();
}