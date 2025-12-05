#include "NetworkManager.h"
#include "Config.h"
#include "StorageManager.h"
#include "DisplayManager.h" 
#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <DNSServer.h>

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
      
      <div class="form-group grp-id">
        <span class="label">ID do Sistema</span>
        <div class="input-wrapper">
          <svg class="icon" viewBox="0 0 24 24" fill="none" stroke="#ff9900" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="4" y1="9" x2="20" y2="9"></line><line x1="4" y1="15" x2="20" y2="15"></line><line x1="10" y1="3" x2="8" y2="21"></line><line x1="16" y1="3" x2="14" y2="21"></line></svg>
          <input type="number" name="id" placeholder="1" required>
        </div>
      </div>

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
      MSA Technology V2.0
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

void handleRoot() { server.send(200, "text/html", index_html); }

void handleSave() {
    Receita r;
    r.id = server.arg("id").toInt();
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
        <span class="icon-check">✔</span>
        <h1>Sucesso!</h1>
        <p>Produto cadastrado.</p>
        <a href='/'>NOVO CADASTRO</a>
      </div></body></html>
    )rawliteral";

    server.send(200, "text/html", sucessoHtml);
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

    server.on("/", handleRoot);
    server.on("/salvar", handleSave);
    server.onNotFound(handleRoot); 
    server.begin();
}

void loopNetwork() {
    dnsServer.processNextRequest();
    server.handleClient();
}