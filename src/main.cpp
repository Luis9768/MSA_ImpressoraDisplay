#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>
#include <EEPROM.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <WebServer.h>
#include <nvs_flash.h>
#include <time.h>
#include "pt_font.h"

// ==== PINOS DE HARDWARE ====
constexpr int PIN_TOUCH_CS   = 33;
constexpr int PIN_TOUCH_IRQ  = 36;
constexpr int PIN_TOUCH_CLK  = 25;
constexpr int PIN_TOUCH_MISO = 39;
constexpr int PIN_TOUCH_MOSI = 32;

constexpr int PIN_LED_R     = 4;
constexpr int PIN_LED_G     = 16;
constexpr int PIN_LED_B     = 17;

constexpr int SCREEN_WIDTH  = 240;
constexpr int SCREEN_HEIGHT = 320;
constexpr int LVGL_BUF_LINES = 10;

// Pinos ajustados para a placa informada
// Disponiveis: GPIO22 (sensor). Sem reset de hardware.
constexpr int SENSOR_PIN = 22;       // entrada do sensor (usa pull-up interno)
constexpr int PRINTER_TX_PIN = 1;    // TX da impressora (GPIO1)
constexpr int PRINTER_RX_PIN = 3;    // RX da impressora (GPIO3)
constexpr unsigned long PRINTER_BAUD_RATE = 9600;
// Sem temporizador de reset por hardware

constexpr int TOUCH_MIN_X = 200;
constexpr int TOUCH_MAX_X = 3800;
constexpr int TOUCH_MIN_Y = 200;
constexpr int TOUCH_MAX_Y = 3800;

// ==== OBJETOS GLOBAIS ====
SPIClass mySPI(VSPI);
XPT2046_Touchscreen ts(PIN_TOUCH_CS, PIN_TOUCH_IRQ);
TFT_eSPI tft = TFT_eSPI();
static lv_obj_t *labelCodigo = nullptr;
static lv_obj_t *labelPrompt = nullptr;
static lv_obj_t *textareaAtiva = nullptr;
static lv_obj_t *kbGlobal = nullptr;
static int etapa = 0; // 0=codigo, 1=RE, 2=data
static lv_obj_t *telaProducao = nullptr;
static lv_obj_t *lblResumo = nullptr;
static lv_obj_t *lblContador = nullptr;
static lv_obj_t *arcProgresso = nullptr;
static lv_obj_t *lblLimite = nullptr;
static String codigoProduto;
static String reOperador;
static String dataProducao;
static uint32_t contadorSequencial = 0;
static uint32_t limiteProducao = 100; // configuravel
static bool producaoAtiva = false;
static bool sensorEstadoAnterior = false;
// Sem reset por hardware

// ==== RECEITAS WIFI ====
#include "app_api.h"

const int MAX_RECEITAS = 20;
Receita receitas[MAX_RECEITAS];
int totalReceitas = 0;
int currentID = 1;
Receita receitaSelecionada;
bool receitaConfirmada = false;
int contadorProducao = 0;
int quantidadeAlvo = 0;

const char* ssid = "ESP_RECEITAS";
const char* password = "12345678";
WebServer server(80);

// Objetos LVGL para tela de receitas
static lv_obj_t *telaReceitas = nullptr;
static lv_obj_t *lblReceitaCodigo = nullptr;
static lv_obj_t *lblReceitaQtd = nullptr;
static lv_obj_t *lblReceitaDesc = nullptr;
static lv_obj_t *lblReceitaBarras = nullptr;
static lv_obj_t *lblReceitaID = nullptr;

// Objetos para tela de RE
static lv_obj_t *telaRE = nullptr;
static lv_obj_t *txtRE = nullptr;
static String reDigitado = "";

// Objetos para tela de data
static lv_obj_t *telaData = nullptr;
static lv_obj_t *txtData = nullptr;
static String dataDigitada = "";

// Botão reset com temporizador
static lv_obj_t *btnReset = nullptr;
static uint32_t resetPressStart = 0;
static bool resetPressed = false;

// Controle de impressão
static bool serialImpressoraInicializada = false;

// ==== IMPRESSAO/EEPROM ====
struct LabelConfig {
  uint32_t codigoProdutoNum;
  uint16_t reValor;
  uint8_t fabDia;
  uint8_t fabMes;
  uint8_t fabAno; // dois ultimos digitos
};

constexpr uint32_t EEPROM_MAGIC = 0x52435032UL; // "RCP2" - Magic number para receitas
struct ReceitaPersistida {
  int id;
  char codigo[32];
  char quantidade[16];
  char descricao[128];
  char barcode[64];
};

struct PersistedData {
  uint32_t magic;
  uint32_t contador;
  LabelConfig config;
  int totalReceitas;
  ReceitaPersistida receitas[MAX_RECEITAS];
};

// Tamanho da EEPROM emulada deve caber na partição NVS (~20KB total).
// Com MAX_RECEITAS=20, PersistedData ~ 4.9KB. Usamos 8KB para folga.
constexpr size_t EEPROM_SIZE = 8192; // 8KB
constexpr int EEPROM_COUNTER_ADDR = 0;
constexpr uint32_t CONTADOR_MAX = 1000; // ajuste conforme necessario

LabelConfig etiquetaConfig{999999, 0, 1, 1, 24};
PersistedData dadosPersistidos{};

HardwareSerial printerSerial(2);
static lv_color_t draw_buf_memory[SCREEN_WIDTH * LVGL_BUF_LINES];
static bool lvglAtivo = true; // mantido ligado para usar apenas LVGL

// ==== FUNCOES ====
void setupHardware();
void setLED(bool r, bool g, bool b);
void displayMensagem(const String &texto, int y, int tamanhoFonte);
void atualizarTouch();
void setupLVGL();
void loopLVGLOnly();
void lvglLoop();
void flushDisplay(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
void lerTouch(lv_indev_t *indev, lv_indev_data_t *data);
void criarUIComTeclado();
void eventoTextarea(lv_event_t *e);
void avancarEtapa();
void atualizarPromptETamanho();
void criarTelaProducaoSimples();
void atualizarTelaProducao();
void processarSensorProducao();
bool dataValida(const char *txt);
void persistirEstado();
void carregarPersistido();
void salvarContador();
void salvarConfigEtiqueta();
void iniciarSerialImpressora();
void imprimirContador(uint32_t valor);
bool atualizarLabelConfigFromUI();
void voltarTelaInicial(lv_event_t *e);
void zerarContadorEvent(lv_event_t *e);
// Handlers web movidos para módulo separado
#include "web_handlers.h"
void mostrarReceitaLVGL(int id);
void criarTelaReceitas();
void atualizarIDReceita(int delta);
void confirmarReceita(lv_event_t *e);
void criarTelaRE();
void criarTelaData();
void criarTelaProducao();
void atualizarTelaProducaoReceita();
void processarSensorProducaoReceita();
void imprimirEtiqueta();
void salvarReceitasEEPROM();
void carregarReceitasEEPROM();

// HTML e handlers movidos para src/web_handlers.cpp

void mostrarReceitaLVGL(int id) {
  if (totalReceitas == 0 || id == 0) {
    if (lblReceitaCodigo) lv_label_set_text(lblReceitaCodigo, "Nenhuma receita cadastrada");
    if (lblReceitaQtd) lv_label_set_text(lblReceitaQtd, "Use o WiFi para cadastrar");
    if (lblReceitaDesc) lv_label_set_text(lblReceitaDesc, "SSID: ESP_RECEITAS");
    if (lblReceitaBarras) lv_label_set_text(lblReceitaBarras, "IP: 192.168.4.1");
    return;
  }
  
  for (int i = 0; i < totalReceitas; i++) {
    if (receitas[i].id == id) {
      if (lblReceitaCodigo) lv_label_set_text(lblReceitaCodigo, ("Codigo: " + receitas[i].codigo).c_str());
      if (lblReceitaQtd) lv_label_set_text(lblReceitaQtd, ("Quantidade: " + receitas[i].quantidade).c_str());
        if (lblReceitaDesc) lv_label_set_text(lblReceitaDesc, ("Descrição: " + receitas[i].descricao).c_str());
      if (lblReceitaBarras) lv_label_set_text(lblReceitaBarras, ("Cod. Barras: " + receitas[i].barcode).c_str());
      Serial.printf("Mostrando receita ID %d: %s\n", id, receitas[i].codigo.c_str());
      return;
    }
  }
  if (lblReceitaCodigo) lv_label_set_text(lblReceitaCodigo, "Codigo: -");
  if (lblReceitaQtd) lv_label_set_text(lblReceitaQtd, "Quantidade: -");
    if (lblReceitaDesc) lv_label_set_text(lblReceitaDesc, "Descrição: -");
  if (lblReceitaBarras) lv_label_set_text(lblReceitaBarras, "Cod. Barras: -");
}

void atualizarIDReceita(int delta) {
  currentID += delta;
  
  // Limitar navegação para a quantidade de receitas salvas
  int maxID = totalReceitas;
  if (maxID < 1) maxID = 1; // garante pelo menos 1 para exibir placeholder
  if (currentID < 1) currentID = 1;
  if (currentID > maxID) currentID = maxID;
  
  if (lblReceitaID) {
    char buf[32];
    sprintf(buf, "ID: %d", currentID);
    lv_label_set_text(lblReceitaID, buf);
  }
  mostrarReceitaLVGL(currentID);
}

void criarTelaReceitas() {
  telaReceitas = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(telaReceitas, lv_color_hex(0xFFFFFF), 0);

  lv_obj_t *topBar = lv_obj_create(telaReceitas);
  lv_obj_set_size(topBar, LV_PCT(100), 60);
  lv_obj_align(topBar, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_flex_flow(topBar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(topBar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_bg_opa(topBar, LV_OPA_TRANSP, 0);

  lv_obj_t *btnMinus = lv_button_create(topBar);
  lv_obj_set_size(btnMinus, 70, 50);
  lv_obj_t *lblMinus = lv_label_create(btnMinus);
  lv_label_set_text(lblMinus, "-");
  lv_obj_set_style_text_font(lblMinus, pt_font_22(), 0);
  lv_obj_center(lblMinus);
  lv_obj_add_event_cb(btnMinus, [](lv_event_t *e) { atualizarIDReceita(-1); }, LV_EVENT_CLICKED, NULL);

  lblReceitaID = lv_label_create(topBar);
  lv_obj_set_style_text_font(lblReceitaID, &lv_font_montserrat_22, 0);
  char buf[32];
  sprintf(buf, "ID: %d", currentID);
  lv_label_set_text(lblReceitaID, buf);

  lv_obj_t *btnPlus = lv_button_create(topBar);
  lv_obj_set_size(btnPlus, 70, 50);
  lv_obj_t *lblPlus = lv_label_create(btnPlus);
  lv_label_set_text(lblPlus, "+");
  lv_obj_set_style_text_font(lblPlus, pt_font_22(), 0);
  lv_obj_center(lblPlus);
  lv_obj_add_event_cb(btnPlus, [](lv_event_t *e) { atualizarIDReceita(1); }, LV_EVENT_CLICKED, NULL);

  lv_obj_t *card = lv_obj_create(telaReceitas);
  lv_obj_set_size(card, LV_PCT(90), LV_PCT(55));
  lv_obj_align(card, LV_ALIGN_CENTER, 0, 10);
  lv_obj_set_style_pad_all(card, 10, 0);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);

  lblReceitaCodigo = lv_label_create(card);
    lv_label_set_text(lblReceitaCodigo, "Código: -");
  lv_obj_set_style_text_font(lblReceitaCodigo, pt_font_18(), 0);
  lv_obj_set_width(lblReceitaCodigo, LV_PCT(100));

  lblReceitaQtd = lv_label_create(card);
  lv_label_set_text(lblReceitaQtd, "Quantidade: -");
  lv_obj_set_style_text_font(lblReceitaQtd, pt_font_18(), 0);
  lv_obj_set_width(lblReceitaQtd, LV_PCT(100));

  lblReceitaDesc = lv_label_create(card);
    lv_label_set_text(lblReceitaDesc, "Descrição: -");
  lv_obj_set_style_text_font(lblReceitaDesc, pt_font_18(), 0);
  lv_obj_set_width(lblReceitaDesc, LV_PCT(100));

  lblReceitaBarras = lv_label_create(card);
  lv_label_set_text(lblReceitaBarras, "Cod. Barras: -");
  lv_obj_set_style_text_font(lblReceitaBarras, pt_font_18(), 0);
  lv_obj_set_width(lblReceitaBarras, LV_PCT(100));

  lv_obj_t *btnConfirmar = lv_button_create(telaReceitas);
  lv_obj_set_size(btnConfirmar, LV_PCT(100), 60);
  lv_obj_set_style_bg_color(btnConfirmar, lv_color_hex(0x00FF00), 0); // Verde
  lv_obj_align(btnConfirmar, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_t *lblOk = lv_label_create(btnConfirmar);
  lv_label_set_text(lblOk, "CONFIRMA");
  lv_obj_set_style_text_font(lblOk, pt_font_22(), 0);
  lv_obj_center(lblOk);
  lv_obj_add_event_cb(btnConfirmar, confirmarReceita, LV_EVENT_CLICKED, NULL);

  mostrarReceitaLVGL(currentID);
  
  // Carregar a tela de receitas
  lv_screen_load(telaReceitas);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Sistema de Receitas WiFi ===");
  
  setupHardware();
  carregarPersistido();
  
  // WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP iniciado");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // Rotas WebServer (registradas no módulo web)
  register_web_routes(server);
  server.begin();
  Serial.println("Servidor HTTP iniciado");
  
  // Inicializar LVGL e tela de receitas
  if (lvglAtivo) {
    setupLVGL();
    criarTelaReceitas();
    Serial.println("Tela de receitas criada");
  }
  
  // Iniciar serial da impressora
  iniciarSerialImpressora();
}

void loop() {
  // Executa somente o LVGL e a lógica de produção
  loopLVGLOnly();
}

void loopLVGLOnly() {
  // Processar requisições HTTP
  server.handleClient();
  
  // Verificar reset de 1 segundo
  if (resetPressed && (millis() - resetPressStart >= 1000)) {
    resetPressed = false;
    contadorProducao = 0;
    Serial.println("RESET: Contador zerado!");
    atualizarTelaProducaoReceita();
  }
  
  // Roda LVGL e lógica de produção
  if (receitaConfirmada) {
    processarSensorProducaoReceita();
  } else {
    processarSensorProducao();
  }
  lvglLoop();
  delay(10);
}

void setupHardware() {
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  setLED(true, true, true);

  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(false); // evita dupla inversão de bytes nas cores
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  SPI.begin(PIN_TOUCH_CLK, PIN_TOUCH_MISO, PIN_TOUCH_MOSI, PIN_TOUCH_CS);
  ts.begin();
  ts.setRotation(1);
}

void setLED(bool r, bool g, bool b) {
  digitalWrite(PIN_LED_R, !r);
  digitalWrite(PIN_LED_G, !g);
  digitalWrite(PIN_LED_B, !b);
}

void displayMensagem(const String &texto, int y, int tamanhoFonte) {
  tft.setTextFont(tamanhoFonte);
  tft.setCursor(0, y);
  tft.print(texto);
}

void atualizarTouch() {
  if (!ts.tirqTouched() || !ts.touched()) {
    return;
  }

  TS_Point p = ts.getPoint();
  tft.fillRect(0, 200, 240, 40, TFT_BLACK);
  tft.setTextFont(2);
  tft.setCursor(0, 200);
  tft.printf("Touch: x=%d y=%d z=%d", p.x, p.y, p.z);
}

// ==== LVGL (DESLIGADO POR PADRAO) ====
void setupLVGL() {
  lv_init();

  lv_display_t *disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_default(disp);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_flush_cb(disp, flushDisplay);
  lv_display_set_buffers(disp, draw_buf_memory, nullptr, sizeof(draw_buf_memory),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_0);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, lerTouch);
  lv_indev_set_display(indev, disp);

  Serial.println("LVGL inicializado");
}

void lvglLoop() {
  static uint32_t ultimaLeitura = millis();
  uint32_t agora = millis();
  lv_tick_inc(agora - ultimaLeitura);
  ultimaLeitura = agora;
  lv_timer_handler();
}

void flushDisplay(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint16_t w = area->x2 - area->x1 + 1;
  uint16_t h = area->y2 - area->y1 + 1;
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)px_map, w * h, false); // LV_COLOR_16_SWAP ja entrega ordem correta
  tft.endWrite();
  lv_display_flush_ready(disp);
}

void lerTouch(lv_indev_t *indev, lv_indev_data_t *data) {
  (void)indev;
  if (!ts.tirqTouched() || !ts.touched()) {
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

  TS_Point p = ts.getPoint();
  int x = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, SCREEN_WIDTH);
  int y = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, SCREEN_HEIGHT);
  x = (SCREEN_WIDTH - 1) - x; // inverte eixo X (esq <-> dir)
  x = constrain(x, 0, SCREEN_WIDTH - 1);
  y = constrain(y, 0, SCREEN_HEIGHT - 1);

  data->state = LV_INDEV_STATE_PRESSED;
  data->point.x = x;
  data->point.y = y;
}

void criarUIComTeclado() {
  lv_obj_t *cont = lv_obj_create(lv_screen_active());
  lv_obj_set_size(cont, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_center(cont);
  lv_obj_set_style_bg_color(cont, lv_palette_main(LV_PALETTE_GREY), 0);

  labelPrompt = lv_label_create(cont);
  lv_obj_align(labelPrompt, LV_ALIGN_TOP_MID, 0, 4);
  lv_obj_set_style_text_font(labelPrompt, &lv_font_montserrat_22, 0);

  textareaAtiva = lv_textarea_create(cont);
  lv_obj_set_size(textareaAtiva, SCREEN_WIDTH - 16, 110);
  lv_obj_align(textareaAtiva, LV_ALIGN_TOP_MID, 0, 30);
  lv_textarea_set_one_line(textareaAtiva, true);
  lv_obj_set_style_text_font(textareaAtiva, &lv_font_montserrat_22, 0);
  lv_obj_set_style_text_align(textareaAtiva, LV_TEXT_ALIGN_LEFT, 0);

  kbGlobal = lv_keyboard_create(cont);
  lv_obj_set_size(kbGlobal, SCREEN_WIDTH, 150);
  lv_obj_align(kbGlobal, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_keyboard_set_textarea(kbGlobal, textareaAtiva);
  lv_keyboard_set_mode(kbGlobal, LV_KEYBOARD_MODE_NUMBER);
  lv_obj_clear_flag(kbGlobal, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_state(textareaAtiva, LV_STATE_FOCUSED);
  lv_obj_add_event_cb(textareaAtiva, eventoTextarea, LV_EVENT_ALL, nullptr);

  labelCodigo = lv_label_create(cont);
  lv_label_set_text(labelCodigo, "");
  lv_obj_align(labelCodigo, LV_ALIGN_TOP_MID, 0, 150);
  lv_obj_set_style_text_font(labelCodigo, &lv_font_montserrat_22, 0);

  etapa = 0;
  atualizarPromptETamanho();
}

void eventoTextarea(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *ta = (lv_obj_t *)lv_event_get_target(e);
  (void)e;

  if (code == LV_EVENT_VALUE_CHANGED && labelCodigo) {
    const char *txt = lv_textarea_get_text(ta);
    lv_label_set_text(labelCodigo, txt);
    if (strlen(txt) >= lv_textarea_get_max_length(ta)) {
      lv_obj_send_event(ta, LV_EVENT_READY, NULL);
    }
  } else if (code == LV_EVENT_READY) {
    const char *texto = lv_textarea_get_text(ta);
    if (etapa == 0) {
      codigoProduto = texto;
    } else if (etapa == 1) {
      reOperador = texto;
    } else if (etapa == 2) {
      if (!dataValida(texto)) {
        lv_label_set_text(labelPrompt, "Data invalida! DDMMAA");
        lv_textarea_set_text(ta, "");
        return;
      }
      dataProducao = texto;
    }
    avancarEtapa();
  }
}

void avancarEtapa() {
  etapa++;
  if (etapa > 2) {
    producaoAtiva = true;
    sensorEstadoAnterior = false;
    atualizarLabelConfigFromUI();
    criarTelaProducaoSimples();
  } else {
    atualizarPromptETamanho();
  }
}

void atualizarPromptETamanho() {
  if (!textareaAtiva || !labelPrompt || !kbGlobal) return;

  switch (etapa) {
    case 0:
      lv_label_set_text(labelPrompt, "Codigo do produto");
      lv_textarea_set_max_length(textareaAtiva, 6);
      lv_textarea_set_text(textareaAtiva, "");
      lv_textarea_set_placeholder_text(textareaAtiva, "Ex: 123456");
      lv_keyboard_set_mode(kbGlobal, LV_KEYBOARD_MODE_NUMBER);
      break;
    case 1:
      lv_label_set_text(labelPrompt, "RE do operador");
      lv_textarea_set_max_length(textareaAtiva, 4);
      lv_textarea_set_text(textareaAtiva, "");
      lv_textarea_set_placeholder_text(textareaAtiva, "Ex: 1234");
      lv_keyboard_set_mode(kbGlobal, LV_KEYBOARD_MODE_NUMBER);
      break;
    case 2:
      lv_label_set_text(labelPrompt, "Digite a data (DDMMAA)");
      lv_textarea_set_max_length(textareaAtiva, 6);
      lv_textarea_set_text(textareaAtiva, "");
      lv_textarea_set_placeholder_text(textareaAtiva, "Ex: 310125");
      lv_keyboard_set_mode(kbGlobal, LV_KEYBOARD_MODE_NUMBER);
      break;
    default:
      break;
  }

  lv_obj_clear_flag(kbGlobal, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_state(textareaAtiva, LV_STATE_FOCUSED);
  lv_keyboard_set_textarea(kbGlobal, textareaAtiva);
}

void criarTelaProducaoSimples() {
  telaProducao = lv_obj_create(NULL);
  lv_screen_load(telaProducao);

  lblResumo = lv_label_create(telaProducao);
  lv_label_set_text(lblResumo, "Resumo");
  lv_obj_set_width(lblResumo, SCREEN_WIDTH - 12);
  lv_obj_align(lblResumo, LV_ALIGN_TOP_MID, 0, 6);
  lv_obj_set_style_text_font(lblResumo, &lv_font_montserrat_22, 0);

  lblContador = lv_label_create(telaProducao);
  lv_label_set_text_fmt(lblContador, "Pecas: %lu", (unsigned long)contadorSequencial);
  lv_obj_set_style_text_font(lblContador, &lv_font_montserrat_22, 0);

  arcProgresso = lv_arc_create(telaProducao);
  lv_obj_set_size(arcProgresso, 200, 200);
  lv_obj_align(arcProgresso, LV_ALIGN_CENTER, 0, 40);
  lv_arc_set_rotation(arcProgresso, 270);
  lv_arc_set_bg_angles(arcProgresso, 0, 360);
  lv_arc_set_range(arcProgresso, 0, limiteProducao);
  lv_obj_clear_flag(arcProgresso, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_text_font(arcProgresso, &lv_font_montserrat_22, 0);
  lv_obj_set_style_text_color(arcProgresso, lv_color_black(), 0);

  lv_obj_t *btnVoltar = lv_btn_create(telaProducao);
  lv_obj_set_size(btnVoltar, 90, 32);
  lv_obj_align(btnVoltar, LV_ALIGN_BOTTOM_LEFT, 10, -8);
  lv_obj_add_event_cb(btnVoltar, voltarTelaInicial, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lblVoltar = lv_label_create(btnVoltar);
  lv_label_set_text(lblVoltar, "Voltar");
  lv_obj_center(lblVoltar);

  lv_obj_t *btnZerar = lv_btn_create(telaProducao);
  lv_obj_set_size(btnZerar, 90, 32);
  lv_obj_align(btnZerar, LV_ALIGN_BOTTOM_RIGHT, -10, -8);
  lv_obj_add_event_cb(btnZerar, zerarContadorEvent, LV_EVENT_ALL, NULL);
  lv_obj_t *lblZerar = lv_label_create(btnZerar);
  lv_label_set_text(lblZerar, "Zerar");
  lv_obj_center(lblZerar);

  atualizarTelaProducao();
}

void atualizarTelaProducao() {
  if (!telaProducao || !lblResumo || !lblContador) return;
  lv_label_set_text_fmt(lblResumo,
                        "Codigo: %s\nRE: %s\nData: %s",
                        codigoProduto.c_str(),
                        reOperador.c_str(),
                        dataProducao.c_str());
  lv_label_set_text_fmt(lblContador, "Pecas: %lu", (unsigned long)contadorSequencial);
  if (arcProgresso) {
    uint32_t valor = contadorSequencial;
    if (valor > limiteProducao) valor = limiteProducao;
    lv_arc_set_range(arcProgresso, 0, limiteProducao);
    lv_arc_set_value(arcProgresso, valor);
    lv_obj_align(lblContador, LV_ALIGN_CENTER, 0, 40);
  }
  if (lblLimite) {
    lv_label_set_text_fmt(lblLimite, "Limite: %lu", (unsigned long)limiteProducao);
  }
}

void processarSensorProducao() {
  bool sensorAtivo = (digitalRead(SENSOR_PIN) == LOW);

  // Feedback LED: sensor = verde, inativo = apagado (sempre ativo)
  setLED(true, true, true); // apaga por padrao
  if (sensorAtivo) setLED(true, false, true); // verde

  // Log de mudança de estado no terminal
  if (sensorAtivo != sensorEstadoAnterior) {
    Serial.printf("SENSOR: %s\n", sensorAtivo ? "ATIVO" : "---");
  }

  // Incrementa contador em borda de ativacao (sempre)
  if (sensorAtivo && !sensorEstadoAnterior) {
    contadorSequencial++;
    Serial.printf("CONTADOR: %lu\n", (unsigned long)contadorSequencial);
    atualizarTelaProducao();
    salvarContador();
    imprimirContador(contadorSequencial);
  }

  sensorEstadoAnterior = sensorAtivo;
}

void voltarTelaInicial(lv_event_t *e) {
  (void)e;
  producaoAtiva = false;
  etapa = 0;
  contadorSequencial = 0;
  codigoProduto = "";
  reOperador = "";
  dataProducao = "";
  labelCodigo = nullptr;
  labelPrompt = nullptr;
  textareaAtiva = nullptr;
  kbGlobal = nullptr;
  telaProducao = nullptr;
  lblResumo = nullptr;
  lblContador = nullptr;
  arcProgresso = nullptr;
  lblLimite = nullptr;
  lv_obj_t *nova = lv_obj_create(NULL);
  lv_screen_load(nova);
  criarUIComTeclado();
  salvarContador();
}

void zerarContadorEvent(lv_event_t *e) {
  static uint32_t pressStart = 0;
  static bool jaZerou = false;
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    pressStart = lv_tick_get();
    jaZerou = false;
  } else if (code == LV_EVENT_PRESSING) {
    if (!jaZerou && lv_tick_elaps(pressStart) >= 2000) {
      contadorSequencial = 0;
      jaZerou = true;
      atualizarTelaProducao();
    }
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    pressStart = 0;
    jaZerou = false;
  }
}

bool dataValida(const char *txt) {
  if (!txt) return false;
  size_t len = strlen(txt);
  if (len != 6) return false;
  int d = (txt[0] - '0') * 10 + (txt[1] - '0');
  int m = (txt[2] - '0') * 10 + (txt[3] - '0');
  int a = (txt[4] - '0') * 10 + (txt[5] - '0'); // aa
  if (d <= 0 || m <= 0) return false;
  if (m > 12) return false;

  int diasMes = 31;
  if (m == 4 || m == 6 || m == 9 || m == 11) diasMes = 30;
  else if (m == 2) {
    bool bissexto = (a % 4 == 0); // aproxima para aa
    diasMes = bissexto ? 29 : 28;
  }
  return d <= diasMes;
}

void persistirEstado() {
  dadosPersistidos.magic = EEPROM_MAGIC;
  dadosPersistidos.contador = contadorSequencial;
  dadosPersistidos.config = etiquetaConfig;
  dadosPersistidos.totalReceitas = totalReceitas;
  EEPROM.put(EEPROM_COUNTER_ADDR, dadosPersistidos);
  EEPROM.commit();
}

void carregarPersistido() {
  Serial.printf("Tentando inicializar EEPROM com %d bytes...\n", EEPROM_SIZE);
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Falha ao iniciar EEPROM! Apagando NVS e tentando novamente...");
    nvs_flash_erase(); // Apaga toda a NVS
    nvs_flash_init();  // Reinicializa
    delay(100);
    if (!EEPROM.begin(EEPROM_SIZE)) {
      Serial.println("Falha critica ao iniciar EEPROM!");
      contadorSequencial = 0;
      totalReceitas = 0;
      return;
    }
  }
  Serial.println("EEPROM inicializada com sucesso!");
  EEPROM.get(EEPROM_COUNTER_ADDR, dadosPersistidos);
  if (dadosPersistidos.magic != EEPROM_MAGIC) {
    Serial.println("EEPROM vazia ou magic invalido, inicializando...");
    dadosPersistidos.magic = EEPROM_MAGIC;
    dadosPersistidos.contador = 0;
    dadosPersistidos.config = etiquetaConfig;
    dadosPersistidos.totalReceitas = 0;
    totalReceitas = 0;
    persistirEstado();
  } else {
    Serial.println("Dados validos encontrados na EEPROM");
    contadorSequencial = dadosPersistidos.contador;
    etiquetaConfig = dadosPersistidos.config;
    if (contadorSequencial > CONTADOR_MAX) {
      contadorSequencial = 0;
      persistirEstado();
    }
    Serial.printf("Contador armazenado: %lu\n", (unsigned long)contadorSequencial);
    
    // Carregar receitas
    carregarReceitasEEPROM();
  }
}

void salvarContador() {
  persistirEstado();
  Serial.printf("Contador atualizado: %lu\n", (unsigned long)contadorSequencial);
}

void salvarConfigEtiqueta() {
  persistirEstado();
  Serial.printf("Etiqueta salva: COD=%lu RE=%u DATA=%02u/%02u/%02u\n",
                (unsigned long)etiquetaConfig.codigoProdutoNum,
                (unsigned int)etiquetaConfig.reValor,
                etiquetaConfig.fabDia,
                etiquetaConfig.fabMes,
                etiquetaConfig.fabAno);
}

void iniciarSerialImpressora() {
  printerSerial.begin(PRINTER_BAUD_RATE, SERIAL_8N1, PRINTER_RX_PIN, PRINTER_TX_PIN);
  serialImpressoraInicializada = true;
}

void imprimirContador(uint32_t valor) {
  if (!serialImpressoraInicializada) return;
  constexpr size_t ZPL_BUFFER_SIZE = 1024;
  char zpl[ZPL_BUFFER_SIZE];
  int escrito = snprintf(
      zpl, ZPL_BUFFER_SIZE,
      "^XA^FO10,10^A0N,30,30^FDCOD:%06lu RE:%04u DATA:%02u/%02u/%02u^FS"
      "^FO10,60^A0N,60,60^FDSEQ:%03lu^FS^XZ",
      (unsigned long)etiquetaConfig.codigoProdutoNum,
      (unsigned int)etiquetaConfig.reValor,
      etiquetaConfig.fabDia, etiquetaConfig.fabMes, etiquetaConfig.fabAno,
      (unsigned long)valor);
  if (escrito <= 0 || escrito >= (int)ZPL_BUFFER_SIZE) {
    Serial.println("Erro ao montar ZPL.");
    return;
  }
  printerSerial.write(reinterpret_cast<const uint8_t *>(zpl), (size_t)escrito);
  printerSerial.flush();
}

bool atualizarLabelConfigFromUI() {
  if (codigoProduto.length() == 0 || reOperador.length() == 0 || dataProducao.length() != 6) {
    return false;
  }
  etiquetaConfig.codigoProdutoNum = (uint32_t)codigoProduto.toInt();
  etiquetaConfig.reValor = (uint16_t)reOperador.toInt();
  etiquetaConfig.fabDia = (uint8_t)((dataProducao.substring(0, 2)).toInt());
  etiquetaConfig.fabMes = (uint8_t)((dataProducao.substring(2, 4)).toInt());
  etiquetaConfig.fabAno = (uint8_t)((dataProducao.substring(4, 6)).toInt());
  salvarConfigEtiqueta();
  return true;
}

// ==== NOVAS FUNÇÕES PARA RECEITA ====
void confirmarReceita(lv_event_t *e) {
  // Buscar receita selecionada
  for (int i = 0; i < totalReceitas; i++) {
    if (receitas[i].id == currentID) {
      receitaSelecionada = receitas[i];
      quantidadeAlvo = receitaSelecionada.quantidade.toInt();
      
      Serial.printf("Receita selecionada: ID=%d Codigo=%s Qtd=%d\n", 
                    receitaSelecionada.id, 
                    receitaSelecionada.codigo.c_str(), 
                    quantidadeAlvo);
      
      // Ir para tela de RE
      criarTelaRE();
      return;
    }
  }
  
  Serial.println("Receita não encontrada!");
}

void criarTelaRE() {
  telaRE = lv_obj_create(NULL);
  lv_screen_load(telaRE);
  lv_obj_set_style_bg_color(telaRE, lv_color_hex(0xFFFFFF), 0);
  
  // Botão voltar (canto superior esquerdo)
  lv_obj_t *btnVoltar = lv_btn_create(telaRE);
  lv_obj_set_size(btnVoltar, 80, 40);
  lv_obj_align(btnVoltar, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_add_event_cb(btnVoltar, [](lv_event_t *e) {
    lv_screen_load(telaReceitas);
  }, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lblVoltar = lv_label_create(btnVoltar);
  lv_label_set_text(lblVoltar, "Voltar");
  lv_obj_center(lblVoltar);
  
  // Título (abaixo do botão)
  lv_obj_t *lblTitulo = lv_label_create(telaRE);
  lv_label_set_text(lblTitulo, "RE Operador");
  lv_obj_set_style_text_font(lblTitulo, &lv_font_montserrat_22, 0);
  lv_obj_align(lblTitulo, LV_ALIGN_TOP_MID, 0, 60);
  
  // Campo de texto
  txtRE = lv_textarea_create(telaRE);
  lv_obj_set_size(txtRE, 200, 50);
  lv_obj_align(txtRE, LV_ALIGN_CENTER, 0, -20);
  lv_textarea_set_max_length(txtRE, 4);
  lv_textarea_set_one_line(txtRE, true);
  lv_textarea_set_text(txtRE, "");
  lv_textarea_set_accepted_chars(txtRE, "0123456789");
  
  // Teclado numérico com evento de aceite
  lv_obj_t *kb = lv_keyboard_create(telaRE);
  lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
  lv_keyboard_set_textarea(kb, txtRE);
  lv_obj_set_size(kb, SCREEN_WIDTH, 140);
  lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  
  // Evento de aceite do teclado
  lv_obj_add_event_cb(kb, [](lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY) {
      reDigitado = String(lv_textarea_get_text(txtRE));
      if (reDigitado.length() == 4) {
        reOperador = reDigitado;
        Serial.printf("RE digitado: %s\n", reOperador.c_str());
        criarTelaData();
      }
    }
  }, LV_EVENT_READY, NULL);
  
  // Evento para avançar automaticamente quando atingir 4 dígitos
  lv_obj_add_event_cb(txtRE, [](lv_event_t *e) {
    const char *texto = lv_textarea_get_text(txtRE);
    if (strlen(texto) == 4) {
      reDigitado = String(texto);
      reOperador = reDigitado;
      Serial.printf("RE digitado: %s\n", reOperador.c_str());
      criarTelaData();
    }
  }, LV_EVENT_VALUE_CHANGED, NULL);
}

void criarTelaData() {
  telaData = lv_obj_create(NULL);
  lv_screen_load(telaData);
  lv_obj_set_style_bg_color(telaData, lv_color_hex(0xFFFFFF), 0);
  
  // Botão voltar (canto superior esquerdo)
  lv_obj_t *btnVoltar = lv_btn_create(telaData);
  lv_obj_set_size(btnVoltar, 80, 40);
  lv_obj_align(btnVoltar, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_add_event_cb(btnVoltar, [](lv_event_t *e) {
    criarTelaRE();
  }, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lblVoltar = lv_label_create(btnVoltar);
  lv_label_set_text(lblVoltar, "Voltar");
  lv_obj_center(lblVoltar);
  
  // Título (abaixo do botão)
  lv_obj_t *lblTitulo = lv_label_create(telaData);
  lv_label_set_text(lblTitulo, "Data: DDMMAA");
  lv_obj_set_style_text_font(lblTitulo, &lv_font_montserrat_22, 0);
  lv_obj_align(lblTitulo, LV_ALIGN_TOP_MID, 0, 60);
  
  // Campo de texto
  txtData = lv_textarea_create(telaData);
  lv_obj_set_size(txtData, 200, 50);
  lv_obj_align(txtData, LV_ALIGN_CENTER, 0, -20);
  lv_textarea_set_max_length(txtData, 6);
  lv_textarea_set_one_line(txtData, true);
  lv_textarea_set_text(txtData, "");
  lv_textarea_set_placeholder_text(txtData, "DDMMAA");
  lv_textarea_set_accepted_chars(txtData, "0123456789");
  
  // Teclado numérico com evento de aceite
  lv_obj_t *kb = lv_keyboard_create(telaData);
  lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
  lv_keyboard_set_textarea(kb, txtData);
  lv_obj_set_size(kb, SCREEN_WIDTH, 140);
  lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
  
  // Evento de aceite do teclado
  lv_obj_add_event_cb(kb, [](lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY) {
      dataDigitada = String(lv_textarea_get_text(txtData));
      if (dataDigitada.length() == 6) {
        // Validar data DDMMAA
        int dia = dataDigitada.substring(0, 2).toInt();
        int mes = dataDigitada.substring(2, 4).toInt();
        int ano = dataDigitada.substring(4, 6).toInt();
        
        if (mes >= 1 && mes <= 12 && dia >= 1 && dia <= 31) {
          // Validação específica por mês
          int diasNoMes[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
          if (dia <= diasNoMes[mes - 1]) {
            dataProducao = dataDigitada;
            Serial.printf("Data digitada: %s\n", dataProducao.c_str());
            receitaConfirmada = true;
            contadorProducao = 0;
            criarTelaProducao();
          } else {
            Serial.println("Data invalida: dia nao existe no mes");
            lv_textarea_set_text(txtData, "");
          }
        } else {
          Serial.println("Data invalida: mes ou dia fora do intervalo");
          lv_textarea_set_text(txtData, "");
        }
      }
    }
  }, LV_EVENT_READY, NULL);
  
  // Evento para avançar automaticamente quando atingir 6 dígitos
  lv_obj_add_event_cb(txtData, [](lv_event_t *e) {
    const char *texto = lv_textarea_get_text(txtData);
    if (strlen(texto) == 6) {
      dataDigitada = String(texto);
      
      // Validar data DDMMAA
      int dia = dataDigitada.substring(0, 2).toInt();
      int mes = dataDigitada.substring(2, 4).toInt();
      int ano = dataDigitada.substring(4, 6).toInt();
      
      if (mes >= 1 && mes <= 12 && dia >= 1 && dia <= 31) {
        // Validação específica por mês
        int diasNoMes[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (dia <= diasNoMes[mes - 1]) {
          dataProducao = dataDigitada;
          Serial.printf("Data digitada: %s\n", dataProducao.c_str());
          receitaConfirmada = true;
          contadorProducao = 0;
          criarTelaProducao();
        } else {
          Serial.println("Data invalida: dia nao existe no mes");
          lv_textarea_set_text(txtData, "");
        }
      } else {
        Serial.println("Data invalida: mes ou dia fora do intervalo");
        lv_textarea_set_text(txtData, "");
      }
    }
  }, LV_EVENT_VALUE_CHANGED, NULL);
}

void criarTelaProducao() {
  telaProducao = lv_obj_create(NULL);
  lv_screen_load(telaProducao);
  lv_obj_set_style_bg_color(telaProducao, lv_color_hex(0xFFFFFF), 0);
  
  // Título com código do produto
  lv_obj_t *lblTitulo = lv_label_create(telaProducao);
  lv_label_set_text_fmt(lblTitulo, "Codigo: %s", receitaSelecionada.codigo.c_str());
  lv_obj_set_style_text_font(lblTitulo, &lv_font_montserrat_22, 0);
  lv_obj_align(lblTitulo, LV_ALIGN_TOP_MID, 0, 10);
  
  // RE e Data
  lv_obj_t *lblInfo = lv_label_create(telaProducao);
  lv_label_set_text_fmt(lblInfo, "RE: %s | Data: %s", reOperador.c_str(), dataProducao.c_str());
  lv_obj_align(lblInfo, LV_ALIGN_TOP_MID, 0, 35);
  
  // Arc de progresso
  arcProgresso = lv_arc_create(telaProducao);
  lv_obj_set_size(arcProgresso, 160, 160);
  lv_obj_align(arcProgresso, LV_ALIGN_CENTER, 0, 0);
  lv_arc_set_rotation(arcProgresso, 270);
  lv_arc_set_bg_angles(arcProgresso, 0, 360);
  lv_arc_set_range(arcProgresso, 0, quantidadeAlvo);
  lv_arc_set_value(arcProgresso, 0);
  lv_obj_remove_flag(arcProgresso, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_color(arcProgresso, lv_color_hex(0x0b57d0), LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(arcProgresso, 15, LV_PART_INDICATOR);
  
  // Label do contador dentro do arco
  lblContador = lv_label_create(telaProducao);
  lv_label_set_text_fmt(lblContador, "%d / %d", contadorProducao, quantidadeAlvo);
  lv_obj_set_style_text_font(lblContador, &lv_font_montserrat_22, 0);
  lv_obj_align(lblContador, LV_ALIGN_CENTER, 0, 0);
  
  // Botão voltar (inferior esquerdo)
  lv_obj_t *btnVoltar = lv_btn_create(telaProducao);
  lv_obj_set_size(btnVoltar, 100, 50);
  lv_obj_align(btnVoltar, LV_ALIGN_BOTTOM_LEFT, 10, -10);
  lv_obj_add_event_cb(btnVoltar, [](lv_event_t *e) {
    receitaConfirmada = false;
    contadorProducao = 0;
    producaoAtiva = false;
    lv_screen_load(telaReceitas);
  }, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lblVoltar = lv_label_create(btnVoltar);
  lv_label_set_text(lblVoltar, "Voltar");
  lv_obj_center(lblVoltar);
  
  // Botão RESET (inferior direito, mantenha pressionado por 1s)
  btnReset = lv_btn_create(telaProducao);
  lv_obj_set_size(btnReset, 100, 50);
  lv_obj_align(btnReset, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
  lv_obj_set_style_bg_color(btnReset, lv_color_hex(0x0000ff), 0); // Vermelho em BGR
  
  lv_obj_add_event_cb(btnReset, [](lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
      resetPressed = true;
      resetPressStart = millis();
      Serial.println("Reset button PRESSED");
    } else if (code == LV_EVENT_RELEASED) {
      resetPressed = false;
      Serial.println("Reset button RELEASED");
    }
  }, LV_EVENT_ALL, NULL);
  
  lv_obj_t *lblReset = lv_label_create(btnReset);
  lv_label_set_text(lblReset, "RESET\n(1s)");
  lv_obj_set_style_text_align(lblReset, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(lblReset);
  
  producaoAtiva = true;
}



void imprimirEtiqueta() {
  if (!serialImpressoraInicializada) {
    Serial.println("Serial da impressora nao inicializado.");
    return;
  }

  // Extrair dados da data DDMMAA
  int dia = dataProducao.substring(0, 2).toInt();
  int mes = dataProducao.substring(2, 4).toInt();
  int ano = dataProducao.substring(4, 6).toInt();
  
  // Extrair RE
  int reValor = reOperador.toInt();
  
  // Código do produto (pegar da receita)
  unsigned long codigoProd = receitaSelecionada.codigo.toInt();

  constexpr size_t ZPL_BUFFER_SIZE = 2048;
  char zplBuffer[ZPL_BUFFER_SIZE];
  int escrito = snprintf(
      zplBuffer,
      ZPL_BUFFER_SIZE,
      "CT~~CD,~CC^~CT~\r\n"
      "^XA~TA000~JSN^LT0^MNW^MTT^PON^PMN^LH0,0^JMA^PR2,2~SD25^JUS^LRN^CI0^XZ\r\n"
      "^XA\r\n"
      "^CI28\r\n"  // Habilita UTF-8 para acentos nas fontes
      "^MMT\r\n"
      "^POI\r\n"
      "^PW400\r\n"
      "^LL0599\r\n"
      "^LS0\r\n"
      "^FO0,0^GFA,01920,01920,00020,:Z64:"
      "eJzt1LFrU0EcB/Df9TT3moY8M4S0EH0hASkO+opLQeRlEVcHsykUFFxTirSD5Z0VNEMHR0ELBRfJX+B4oliHDi7iovSmztepb3jN+fvde6+0qYPQ0skfgVy++by7e793CcD/Ol1dcq/j1QLw3GATynnEFEAzHzTzjEuAaTfI37EE0JgxGsw5D1DyYPPjANZk1ZttfZgte3idaN7Z23t3d99caT5YfJY2pj8BTHlf23OD3VZYrpQ6A8nevAAIZn4uz13+1Q8bzcbShqzXPpNbv9rxvrXCtne9MymnXq8B1Grbj5dmtvvhcuXLkxn1tLYF0Hk72H1/a12HHW/QDnX7JrrFjZ2tpZ0t1X+0erBv+vHtV3i7njcBA4AF3LmWoTf58rATFxVmXegDa3SLzJeYSa6BR7LI6thFoVgXeP30j+Rf6uHv8VJgT5SE+ER29m5EUldxVxWwCbcK+xJTZuikNMGmDDOcz7lp56KE5Q5n/J65mLIYncRMzecOP5JTlK04F5nCXRsOJbkqRLpwDA/fiqCNHTpFHVvxXXboXOZcoLuxc5qyhJyO9EIkY3QGI5aQM4Hqx3LM+UqPOx1gRgtYUx4OeYJf3NC+REf9M8KmLmuY4LkJGLmkyDi5AMilmIkUM4HIBHQT1NukcMz4hUvEAS3uHCOXovNHlOkqT6qZY9b49kfvPl7IU9+5kcusHXGDruScxSyg54pOpCJzeMvOMVPh6UTmMAuy7IJIS85RFrn5Ek+MJljmevfqqz1aF8QIMidxb/OuYeBTRk4dyeyY487xYw6bE1jMyKkjzh5xdF5854RzOnd4qiIr+Tn9Pv7meieqe9Z/EedRfwAl7WoT:0C11\r\n"
      "^FT375,547^A0I,32,31^FH\\^FDCODIGO: %06lu^FS\r\n"
      "^FT375,523^A0I,20,19^FH\\^FD%s^FS\r\n"
      "^BY3,2,85^FT329,423^BEI,,Y,N\r\n"
      "^FD7899619611676^FS\r\n"
      "^FO13,385^GB371,0,8^FS\r\n"
      "^FT356,330^A0I,31,31^FH\\^FDFAB:^FS\r\n"
      "^SL0\r\n"
      "^FT287,330^A0I,31,31\r\n"
      "^FC%%,{,#\r\n"
      "^FD%02u/%02u/%02u^FS\r\n"
      "^FT341,287^A0I,31,31^FH\\^FDRE:^FS\r\n"
      "^FT287,287^A0I,31,31^FH\\^FD%04u^FS\r\n"
      "^FT372,233^A0I,31,31^FH\\^FDSEQUENCIAL:^FS\r\n"
      "^FT188,233^A0I,31,31^FH\\^FD%03lu^FS\r\n"
      "^PQ1,0,1,Y^XZ\r\n",
      static_cast<unsigned long>(codigoProd),
      receitaSelecionada.descricao.c_str(),
      static_cast<unsigned int>(dia),
      static_cast<unsigned int>(mes),
      static_cast<unsigned int>(ano),
      reValor,
      static_cast<unsigned long>(contadorProducao));

  if (escrito <= 0 || static_cast<size_t>(escrito) >= ZPL_BUFFER_SIZE) {
    Serial.println("Falha ao montar o comando ZPL.");
    return;
  }

  printerSerial.write(reinterpret_cast<const uint8_t *>(zplBuffer), static_cast<size_t>(escrito));
  printerSerial.flush();
  Serial.printf("Etiqueta impressa: Codigo=%lu, RE=%d, Data=%02d/%02d/%02d, Seq=%lu\n",
                static_cast<unsigned long>(codigoProd), reValor, dia, mes, ano,
                static_cast<unsigned long>(contadorProducao));
}

void atualizarTelaProducaoReceita() {
  if (!lblContador || !arcProgresso) return;
  
  lv_label_set_text_fmt(lblContador, "%d / %d", contadorProducao, quantidadeAlvo);
  lv_arc_set_value(arcProgresso, contadorProducao);
  
  // Verificar se atingiu a quantidade
  if (contadorProducao >= quantidadeAlvo) {
    setLED(false, true, false); // Verde - produção completa
    producaoAtiva = false;
    
    Serial.println("PRODUÇÃO COMPLETA!");
    
    // Mostrar tela amarela de conclusão
    lv_obj_t *telaConclusao = lv_obj_create(NULL);
    lv_screen_load(telaConclusao);
    lv_obj_set_style_bg_color(telaConclusao, lv_color_hex(0x00FFFF), 0); // Amarelo (BGR: 00FFFF)
    
    lv_obj_t *lblMsg = lv_label_create(telaConclusao);
    lv_label_set_text(lblMsg, "PRODUCAO\nCONCLUIDA");
    lv_obj_set_width(lblMsg, SCREEN_WIDTH - 20);
    lv_obj_set_style_text_font(lblMsg, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_align(lblMsg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lblMsg, lv_color_hex(0x000000), 0);
    lv_obj_center(lblMsg);
    
    lv_timer_handler();
    
    // Aguardar 5 segundos
    delay(5000);
    
    // Zerar contador e voltar para tela de produção
    contadorProducao = 0;
    producaoAtiva = true;
    lv_screen_load(telaProducao);
    lv_label_set_text_fmt(lblContador, "%d / %d", contadorProducao, quantidadeAlvo);
    lv_arc_set_value(arcProgresso, 0);
  }
}

void processarSensorProducaoReceita() {
  if (!receitaConfirmada || !producaoAtiva) return;
  
  bool sensorAtual = (digitalRead(SENSOR_PIN) == LOW);
  
  if (sensorAtual && !sensorEstadoAnterior) {
    // Detecção de borda de subida (sensor ativado)
    contadorProducao++;
    Serial.printf("SENSOR ATIVO - Contador: %d / %d\n", contadorProducao, quantidadeAlvo);
    
    setLED(false, true, false); // LED Verde
    
    // Imprimir etiqueta a cada peça
    imprimirEtiqueta();
    
    atualizarTelaProducaoReceita();
    delay(50);
  } else if (!sensorAtual) {
    setLED(false, false, false); // LED apagado
  }
  
  sensorEstadoAnterior = sensorAtual;
}

// ==== FUNÇÕES DE PERSISTÊNCIA DE RECEITAS ====
void salvarReceitasEEPROM() {
  Serial.printf("[DEBUG] Salvando %d receitas na EEPROM...\n", totalReceitas);
  dadosPersistidos.magic = EEPROM_MAGIC;
  dadosPersistidos.totalReceitas = totalReceitas;
  
  // Copiar receitas para estrutura persistida
  for (int i = 0; i < totalReceitas; i++) {
    dadosPersistidos.receitas[i].id = receitas[i].id;
    receitas[i].codigo.toCharArray(dadosPersistidos.receitas[i].codigo, 32);
    receitas[i].quantidade.toCharArray(dadosPersistidos.receitas[i].quantidade, 16);
    receitas[i].descricao.toCharArray(dadosPersistidos.receitas[i].descricao, 128);
    receitas[i].barcode.toCharArray(dadosPersistidos.receitas[i].barcode, 64);
  }
  
  EEPROM.put(EEPROM_COUNTER_ADDR, dadosPersistidos);
  EEPROM.commit();
  Serial.printf("[OK] Receitas salvas na EEPROM: %d receitas (magic=0x%08X)\n", totalReceitas, dadosPersistidos.magic);
}

void carregarReceitasEEPROM() {
  Serial.printf("[DEBUG] dadosPersistidos.totalReceitas = %d\n", dadosPersistidos.totalReceitas);
  totalReceitas = dadosPersistidos.totalReceitas;
  
  if (totalReceitas < 0 || totalReceitas > MAX_RECEITAS) {
    totalReceitas = 0;
    Serial.println("Número de receitas inválido, resetando...");
    return;
  }
  
  // Copiar receitas da estrutura persistida
  for (int i = 0; i < totalReceitas; i++) {
    receitas[i].id = dadosPersistidos.receitas[i].id;
    receitas[i].codigo = String(dadosPersistidos.receitas[i].codigo);
    receitas[i].quantidade = String(dadosPersistidos.receitas[i].quantidade);
    receitas[i].descricao = String(dadosPersistidos.receitas[i].descricao);
    receitas[i].barcode = String(dadosPersistidos.receitas[i].barcode);
  }
  
  Serial.printf("Receitas carregadas da EEPROM: %d receitas\n", totalReceitas);
  for (int i = 0; i < totalReceitas; i++) {
    Serial.printf("  ID %d: %s - Qtd: %s\n", receitas[i].id, receitas[i].codigo.c_str(), receitas[i].quantidade.c_str());
  }
}
