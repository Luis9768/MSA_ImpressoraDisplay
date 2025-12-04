#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <WebServer.h>
#include "app_api.h" 
#include "espnow_manager.h" // Importante: Permite enviar dados para os escravos

static WebServer* g_server = nullptr;

// O HTML COMPLETO (Sua versão com Excel/LocalStorage)
static const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="pt-BR">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>Cadastro de Peças</title>
  <style>
    body{ font-family:Arial; background:#e6f2ff; margin:0; padding:30px; display:flex; justify-content:center; }
    .container{ width:100%; max-width:900px; }
    h1{ text-align:center; margin-bottom:25px; color:#004c97; }
    .card{ background:#fff; padding:30px; border-radius:20px; box-shadow:0 5px 18px rgba(0,0,0,0.1); margin-bottom:25px; }
    form{ display:grid; grid-template-columns:repeat(2,1fr); gap:20px; }
    label{ display:flex; flex-direction:column; font-weight:600; }
    input, textarea{ margin-top:6px; padding:12px; border:2px solid #bcd6f5; border-radius:10px; background:#f7fbff; }
    textarea{ min-height:85px; resize:vertical; }
    .full{ grid-column:1 / -1; }
    button{ padding:14px; border:0; border-radius:12px; cursor:pointer; background:#0b57d0; color:#fff; font-size:1.1rem; }
    table{ width:100%; border-collapse:collapse; background:#fff; border-radius:12px; overflow:hidden; }
    th{ background:#0b57d0; padding:14px; color:white; }
    td{ padding:12px; border-bottom:1px solid #eee; }
    .actions button{ padding:8px 12px; border-radius:8px; }
    .danger{ background:#e14a4a; color:white; }
    .switch { position: relative; display: inline-block; width: 55px; height: 28px; margin-top: 10px; }
    .switch input { display: none; }
    .slider { position: absolute; cursor: pointer; background-color: #ccc; border-radius: 34px; top: 0; left: 0; right: 0; bottom: 0; transition: .4s; }
    .slider:before { position: absolute; content: ""; height: 22px; width: 22px; left: 3px; bottom: 3px; background-color: white; border-radius: 50%; transition: .4s; }
    input:checked + .slider { background-color: #0b57d0; }
    input:checked + .slider:before { transform: translateX(26px); }
  </style>
</head>
<body>
<div class="container">
  <h1>Cadastro de Peças</h1>
  <div class="card">
    <form id="form" onsubmit="return false;">
      <label>Código (apenas números)<input id="codigo" type="text" pattern="[0-9]+" title="Apenas números" required></label>
      <label>Quantidade de peças<input id="quantidade" type="number" min="1" required></label>
      <label class="full">Descrição<textarea id="descricao"></textarea></label>
      <label>Possui código de barras?<label class="switch"><input type="checkbox" id="temBarcode" onchange="toggleBarcode()"><span class="slider"></span></label></label>
      <label>Código de barras (apenas números)<input id="barcode" type="text" pattern="[0-9]*" title="Apenas números" disabled></label>
      <button id="saveBtn" class="full">Salvar</button>
    </form>
  </div>
  <table>
    <thead><tr><th>ID</th><th>Código</th><th>Quantidade</th><th>Descrição</th><th>Cód. Barras</th><th>Ações</th></tr></thead>
    <tbody id="tableBody"></tbody>
  </table>
</div>
<script>
  let items = JSON.parse(localStorage.getItem('pecas') || '[]');
  function render(){
    const tableBody = document.getElementById('tableBody');
    tableBody.innerHTML = '';
    items.forEach((item, i)=>{
      const tr = document.createElement('tr');
      tr.innerHTML = `<td>${item.id}</td><td>${item.codigo}</td><td>${item.quantidade}</td><td>${item.descricao}</td><td>${item.barcode}</td><td class="actions"><button class="danger" onclick="del(${i})">Apagar</button></td>`;
      tableBody.appendChild(tr);
    });
  }
  function del(i){
    const id = items[i].id; 
    items.splice(i,1);
    items.forEach((item, index) => item.id = index + 1);
    localStorage.setItem('pecas', JSON.stringify(items));
    fetch("/apagar?id=" + id);
    render();
  }
  function toggleBarcode() {
    const chk = document.getElementById('temBarcode');
    const bc  = document.getElementById('barcode');
    bc.disabled = !chk.checked;
    if (!chk.checked) bc.value = "";
  }
  document.getElementById('saveBtn').onclick = ()=>{
    if (!/^[0-9]+$/.test(codigo.value)) { alert('Código deve conter apenas números!'); return; }
    if (temBarcode.checked && barcode.value && !/^[0-9]+$/.test(barcode.value)) { alert('Código de barras deve conter apenas números!'); return; }
    const obj = { codigo: codigo.value, quantidade: quantidade.value, descricao: descricao.value, barcode: temBarcode.checked ? barcode.value : "" };
    obj.id = items.length + 1;
    items.push(obj);
    localStorage.setItem('pecas', JSON.stringify(items));
    fetch("/receita?codigo=" + obj.codigo + "&quantidade=" + obj.quantidade + "&descricao=" + encodeURIComponent(obj.descricao) + "&barcode=" + (temBarcode.checked ? obj.barcode : "") + "&id=" + obj.id);
    document.getElementById('form').reset();
    toggleBarcode();
    render();
  }
  function syncReceitas() {
    console.log("Sincronizando " + items.length + " receitas com ESP32 (resetando antes)...");
    fetch("/limpar").then(()=>{
      items.forEach(item => {
        fetch("/receita?codigo=" + item.codigo + "&quantidade=" + item.quantidade + "&descricao=" + encodeURIComponent(item.descricao) + "&barcode=" + item.barcode + "&id=" + item.id);
      });
    }).catch(err=>console.error('Falha ao limpar no ESP32:', err));
  }
  render();
  syncReceitas();
</script>
</body>
</html>
)rawliteral";

static bool validarNumeros(String str) {
  if (str.length() == 0) return true;
  for (unsigned int i = 0; i < str.length(); i++) {
    if (!isDigit(str.charAt(i))) return false;
  }
  return true;
}

// Handler para Receber Dados (Salvar ou Atualizar)
static void receberDadosHandler() {
  Receita r;
  r.id = g_server->arg("id").toInt();
  r.codigo = g_server->arg("codigo");
  r.quantidade = g_server->arg("quantidade");
  r.descricao = g_server->arg("descricao");
  r.barcode = g_server->arg("barcode");

  if (!validarNumeros(r.codigo)) { g_server->send(400, "text/plain", "Codigo deve conter apenas numeros"); return; }
  if (!validarNumeros(r.quantidade)) { g_server->send(400, "text/plain", "Quantidade deve conter apenas numeros"); return; }
  if (!validarNumeros(r.barcode)) { g_server->send(400, "text/plain", "Codigo de barras deve conter apenas numeros"); return; }

  // 1. Tenta ATUALIZAR se já existe
  for (int i = 0; i < totalReceitas; i++) {
    if (receitas[i].id == r.id) {
      receitas[i] = r;
      salvarReceitasEEPROM();
      enviarDadosEspNow(r, false); // <--- IMPORTANTE: Envia atualização para Escravos (false = Gravar)
      g_server->send(200, "text/plain", "OK (Atualizado)");
      return;
    }
  }

  // 2. Se não existe, CRIA NOVO
  if (totalReceitas < MAX_RECEITAS) {
    receitas[totalReceitas++] = r;
    salvarReceitasEEPROM();
    enviarDadosEspNow(r, false); // <--- IMPORTANTE: Envia nova receita para Escravos (false = Gravar)
    g_server->send(200, "text/plain", "OK (Novo)");
  } else {
    g_server->send(507, "text/plain", "Limite maximo de receitas atingido");
  }
}

// Handler para Apagar Receita
static void apagarReceitaHandler() {
  int id = g_server->arg("id").toInt();
  
  // Encontra e apaga
  for (int i = 0; i < totalReceitas; i++) {
    if (receitas[i].id == id) {
      
      // 1. Avisa os escravos para apagarem
      Receita rTemp;
      rTemp.id = id;
      enviarDadosEspNow(rTemp, true); // <--- IMPORTANTE: true = Apagar
      
      // 2. Remove da lista do Mestre
      for (int j = i; j < totalReceitas - 1; j++) {
        receitas[j] = receitas[j + 1];
      }
      totalReceitas--;
      salvarReceitasEEPROM();
      
      g_server->send(200, "text/plain", "OK (Apagado)");
      return;
    }
  }
  g_server->send(404, "text/plain", "Receita nao encontrada");
}

static void register_web_routes(WebServer &server) {
  g_server = &server;
  server.on("/", HTTP_GET, [](){ g_server->send_P(200, "text/html", index_html); });
  server.on("/receita", HTTP_GET, receberDadosHandler);
  server.on("/apagar", HTTP_GET, apagarReceitaHandler);
  server.on("/limpar", HTTP_GET, [](){ 
    totalReceitas = 0; 
    salvarReceitasEEPROM(); 
    g_server->send(200, "text/plain", "OK"); 
  });
}

#endif