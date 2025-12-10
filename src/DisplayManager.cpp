#include "DisplayManager.h"
#include "Config.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>

TFT_eSPI tft = TFT_eSPI(); 
SPIClass touchSpi(VSPI);
XPT2046_Touchscreen touch(TOUCH_XPT_CS, TOUCH_XPT_IRQ);

// Variável de controle de seleção
// 0 = Nenhuma seleção (Lista)
// >0 = ID do produto selecionado
// -1 = Solicitou voltar
volatile int produtoSelecionado = 0;

void event_handler_item(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        int id = (int)(intptr_t)lv_event_get_user_data(e);
        produtoSelecionado = id;
    }
}

void event_handler_voltar(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        Serial.println(">>> CLIQUE NO BOTAO VOLTAR DETECTADO <<<");
        produtoSelecionado = -1;
    }
}

int verificarToque() {
    int ret = produtoSelecionado;
    if (ret != 0) {
        produtoSelecionado = 0; // Consome o evento para não repetir
    }
    return ret;
}

// Globais do Carousel
std::vector<Receita> currentLista;
int currentIndex = 0;

void event_handler_proximo(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        proximoProdutoCarousel();
    }
}

void event_handler_entrar(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        entrarProdutoCarousel();
    }
}

void proximoProdutoCarousel() {
    if (currentLista.empty()) return;
    currentIndex++;
    if (currentIndex >= currentLista.size()) currentIndex = 0;
    mostrarCarouselSlave(currentLista, currentIndex);
}

void entrarProdutoCarousel() {
    if (currentLista.empty()) return;
    produtoSelecionado = currentLista[currentIndex].id;
}

// (Função mostrarCarouselSlave antiga removida)

// Handler para o botão "Próximo" na tela de produção
// (Handler duplicado removido)

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[240 * 20]; // Buffer vertical

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

// Objeto cursor para debug visual
lv_obj_t * cursor_obj = NULL;

void my_touch_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        
        // Calibração Normal de p.y->x (Vertical)
        int16_t x = map(p.y, 250, 3750, 0, 240); 
        int16_t y = map(p.x, 200, 3900, 0, 320);
        
        if(x < 0) x = 0; if(x >= 240) x = 239;
        if(y < 0) y = 0; if(y >= 320) y = 319;
        
        data->state = LV_INDEV_STATE_PR;
        data->point.x = x; data->point.y = y;
        
        // Atualiza posição do cursor visual
        if (cursor_obj) {
            lv_obj_set_pos(cursor_obj, x - 5, y - 5); // Centraliza a bolinha
            lv_obj_clear_flag(cursor_obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(cursor_obj); // Garante que está no topo
        }

        static unsigned long lastDebug = 0;
        if (millis() - lastDebug > 200) {
            Serial.printf("TOUCH: Raw(%d,%d) -> Screen(%d,%d)\n", p.x, p.y, x, y);
            lastDebug = millis();
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// ... (rest of code) ...

void mostrarCarouselSlave(std::vector<Receita> lista, int indice) {
    // Atualiza globais para navegação
    currentLista = lista;
    if (indice >= lista.size()) indice = 0;
    currentIndex = indice;
    
    lv_obj_clean(lv_scr_act());

    // Recria cursor se foi limpo
    cursor_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cursor_obj, 10, 10);
    lv_obj_set_style_bg_color(cursor_obj, lv_palette_main(LV_PALETTE_BLUE), 0); // BLUE aparece RED
    lv_obj_set_style_radius(cursor_obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(cursor_obj, LV_OBJ_FLAG_CLICKABLE); // Não bloqueia toques
    lv_obj_add_flag(cursor_obj, LV_OBJ_FLAG_HIDDEN); // Escondido até tocar
    
    // Fundo
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);

    // LOGO MSA
    lv_obj_t * lblLogo = lv_label_create(lv_scr_act());
    lv_label_set_text(lblLogo, "MSA");
    lv_obj_center(lblLogo);
    lv_obj_set_style_text_font(lblLogo, &lv_font_montserrat_32, 0); 
    // CORRIGIDO: Amarelo Ouro (RGB: FF D7 00). No BGR pode precisar inverter RB se driver nao fizer.
    // O usuário disse que "Logo MSA é Amarelo"
    lv_obj_set_style_text_color(lblLogo, lv_color_hex(0xFFD700), 0); 

    // Subtitulo
    lv_obj_t * lblSub = lv_label_create(lv_scr_act());
    lv_label_set_text(lblSub, "SISTEMA DE PRODUCAO");
    lv_obj_align(lblSub, LV_ALIGN_CENTER, 0, 30); 
    lv_obj_set_style_text_color(lblSub, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_text_font(lblSub, &lv_font_montserrat_14, 0);

    if (lista.empty()) {
        // TELA DE LISTA VAZIA
        lv_obj_t * lblVazio = lv_label_create(lv_scr_act());
        lv_label_set_text(lblVazio, "SEM PRODUTOS\nCADASTRE PELO MASTER");
        lv_obj_set_style_text_align(lblVazio, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lblVazio, LV_ALIGN_BOTTOM_MID, 0, -40);
        lv_obj_set_style_text_color(lblVazio, lv_palette_main(LV_PALETTE_RED), 0); // RED aparece BLUE (Aviso)
    } else {
        // Botão INICIAR (Use Palette Green - geralmente ok)
        lv_obj_t * btnEntrar = lv_btn_create(lv_scr_act());
        lv_obj_set_size(btnEntrar, 120, 80); 
        lv_obj_align(btnEntrar, LV_ALIGN_BOTTOM_RIGHT, -10, -10); 
        lv_obj_set_style_bg_color(btnEntrar, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_obj_set_style_radius(btnEntrar, 15, 0); 
        lv_obj_add_event_cb(btnEntrar, event_handler_entrar, LV_EVENT_CLICKED, NULL);

        lv_obj_t * lblEntrar = lv_label_create(btnEntrar);
        lv_label_set_text(lblEntrar, "INICIAR");
        lv_obj_center(lblEntrar);
        lv_obj_set_style_text_font(lblEntrar, &lv_font_montserrat_14, 0);
    }
}

// Handler para o botão "Próximo" na tela de produção
void event_handler_proximo_producao(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
       produtoSelecionado = -2; // CÓDIGO ESPECIAL: PRÓXIMO PRODUTO
    }
}

void mostrarTelaProducao(Receita r) {
    lv_obj_clean(lv_scr_act());

    // Recria cursor
    cursor_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cursor_obj, 10, 10);
    lv_obj_set_style_bg_color(cursor_obj, lv_palette_main(LV_PALETTE_BLUE), 0); // BLUE aparece RED
    lv_obj_set_style_radius(cursor_obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(cursor_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(cursor_obj, LV_OBJ_FLAG_HIDDEN);

    // Fundo Geral
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xF0F0F0), 0);

    // Cabeçalho - CORRIGIDO: RED aparece BLUE
    lv_obj_t *header = lv_obj_create(lv_scr_act());
    lv_obj_set_size(header, 240, 50); 
    lv_obj_set_style_bg_color(header, lv_palette_main(LV_PALETTE_RED), 0); 
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    // Label Código (#ID)
    lv_obj_t *lblCod = lv_label_create(header);
    lv_label_set_text_fmt(lblCod, "PRODUTO #%d", r.id);
    lv_obj_center(lblCod);
    lv_obj_set_style_text_color(lblCod, lv_color_white(), 0);
    lv_obj_set_style_text_font(lblCod, &lv_font_montserrat_22, 0); 

    // Descrição
    lv_obj_t *lblDesc = lv_label_create(lv_scr_act());
    lv_label_set_long_mode(lblDesc, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lblDesc, 220); 
    lv_label_set_text(lblDesc, r.descricao);
    lv_obj_align(lblDesc, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_text_align(lblDesc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lblDesc, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_color(lblDesc, lv_color_black(), 0);

    // Card Contador
    lv_obj_t * cardContador = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cardContador, 220, 100); 
    lv_obj_align(cardContador, LV_ALIGN_CENTER, 0, 40); 
    lv_obj_set_style_bg_color(cardContador, lv_color_white(), 0);
    lv_obj_set_style_radius(cardContador, 10, 0);
    lv_obj_clear_flag(cardContador, LV_OBJ_FLAG_SCROLLABLE);

    // Labels do Contador
    lv_obj_t * lblTitulo = lv_label_create(cardContador);
    lv_label_set_text(lblTitulo, "CONTAGEM");
    lv_obj_align(lblTitulo, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_color(lblTitulo, lv_palette_main(LV_PALETTE_GREY), 0);

    char metaTxt[32];
    sprintf(metaTxt, "META: %d", r.quantidade);
    lv_obj_t *lblMeta = lv_label_create(cardContador);
    lv_label_set_text(lblMeta, metaTxt);
    lv_obj_align(lblMeta, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_text_color(lblMeta, lv_palette_main(LV_PALETTE_GREEN), 0);

    // Contador Gigante SEM ZOOM (Use fonte 32)
    static lv_obj_t * lblContador = NULL; 
    lblContador = lv_label_create(cardContador);
    lv_label_set_text(lblContador, "0");
    lv_obj_align(lblContador, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_text_font(lblContador, &lv_font_montserrat_32, 0);
    // CORRIGIDO: RED aparece BLUE (Dark Blue)
    lv_obj_set_style_text_color(lblContador, lv_palette_main(LV_PALETTE_RED), 0); 
    
    lv_obj_set_user_data(lblContador, (void*)999); 

    // Botão PROXIMO (CORRIGIDO: RED aparece BLUE)
    lv_obj_t * btnProximo = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btnProximo, 120, 80); 
    lv_obj_align(btnProximo, LV_ALIGN_BOTTOM_RIGHT, -10, -10); 
    lv_obj_set_style_bg_color(btnProximo, lv_palette_main(LV_PALETTE_RED), 0); 
    lv_obj_set_style_radius(btnProximo, 15, 0); 
    lv_obj_set_style_shadow_width(btnProximo, 10, 0);
    lv_obj_add_event_cb(btnProximo, event_handler_proximo_producao, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lblProx = lv_label_create(btnProximo);
    lv_label_set_text(lblProx, ">");
    lv_obj_center(lblProx);
    lv_obj_set_style_text_font(lblProx, &lv_font_montserrat_32, 0);
}

void atualizarContador(int qtd) {
    // Busca o objeto na tela ativa (meio feio, mas evita variavel global solta)
    lv_obj_t * screen = lv_scr_act();
    uint32_t count = lv_obj_get_child_cnt(screen);
    for(uint32_t i=0; i<count; i++) {
        lv_obj_t * child = lv_obj_get_child(screen, i);
        if ((int)(intptr_t)lv_obj_get_user_data(child) == 999) {
            lv_label_set_text_fmt(child, "%d", qtd);
            return;
        }
    }
}

void setupDisplay() {
    pinMode(21, OUTPUT); digitalWrite(21, HIGH);
    
    tft.begin(); 
    tft.setRotation(0); // VERTICAL
    tft.fillScreen(TFT_BLACK);
    
    touchSpi.begin(TOUCH_XPT_CLK, TOUCH_XPT_MISO, TOUCH_XPT_MOSI, TOUCH_XPT_CS);
    touch.begin(touchSpi); touch.setRotation(0);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 240 * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 240; disp_drv.ver_res = 320;
    disp_drv.flush_cb = my_disp_flush; 
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touch_read;
    lv_indev_drv_register(&indev_drv);

    // Tela Inicial
    mostrarCarouselSlave({}, 0);
}

void loopDisplay() {
    lv_timer_handler();
}