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

void mostrarListaSlave(std::vector<Receita> lista) {
    lv_obj_clean(lv_scr_act());
    produtoSelecionado = 0;

    // Cabeçalho
    lv_obj_t * header = lv_obj_create(lv_scr_act());
    lv_obj_set_size(header, 240, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x003366), 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    
    lv_obj_t * title = lv_label_create(header);
    lv_label_set_text(title, "SELECIONE O PRODUTO");
    lv_obj_center(title);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);

    // Lista (Container com Scroll)
    lv_obj_t * list = lv_obj_create(lv_scr_act());
    lv_obj_set_size(list, 240, 270);
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(list, 10, 0);
    lv_obj_set_style_pad_row(list, 25, 0); // Mais espaco entre itens

    if (lista.empty()) {
        lv_obj_t * lbl = lv_label_create(list);
        lv_label_set_text(lbl, "Aguardando dados...\n\nSalve um produto no\nMaster para aparecer aqui.");
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(lbl);
        return;
    }

    for (const auto& r : lista) {
        // Cria um botão para cada item
        lv_obj_t * btn = lv_btn_create(list);
        lv_obj_set_width(btn, lv_pct(100));
        lv_obj_set_height(btn, 85); // Botao mais alto para facilitar o toque
        lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xEEEEEE), LV_STATE_PRESSED);
        lv_obj_set_style_shadow_width(btn, 10, 0);
        lv_obj_set_style_shadow_color(btn, lv_palette_main(LV_PALETTE_GREY), 0);
        lv_obj_set_style_shadow_opa(btn, 30, 0);
        
        // Callback
        lv_obj_add_event_cb(btn, event_handler_item, LV_EVENT_CLICKED, (void*)(intptr_t)r.id);

        // Texto Código
        lv_obj_t * lblCod = lv_label_create(btn);
        lv_label_set_text(lblCod, r.codigo);
        lv_obj_align(lblCod, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_set_style_text_color(lblCod, lv_color_hex(0x003366), 0);
        lv_obj_set_style_text_font(lblCod, &lv_font_montserrat_14, 0);

        // Texto Descrição
        lv_obj_t * lblDesc = lv_label_create(btn);
        lv_label_set_text(lblDesc, r.descricao);
        lv_obj_align(lblDesc, LV_ALIGN_BOTTOM_LEFT, 0, 0);
        lv_obj_set_style_text_color(lblDesc, lv_color_hex(0x666666), 0);
        lv_label_set_long_mode(lblDesc, LV_LABEL_LONG_DOT);
        lv_obj_set_width(lblDesc, 150);

        // Badge Quantidade
        lv_obj_t * badge = lv_obj_create(btn);
        lv_obj_set_size(badge, 50, 25);
        lv_obj_align(badge, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_style_bg_color(badge, lv_color_hex(0x28a745), 0);
        lv_obj_set_style_radius(badge, 10, 0);
        lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t * lblQtd = lv_label_create(badge);
        lv_label_set_text_fmt(lblQtd, "%d", r.quantidade);
        lv_obj_center(lblQtd);
        lv_obj_set_style_text_color(lblQtd, lv_color_white(), 0);
    }
}

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

void my_touch_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    if (touch.touched()) {
        TS_Point p = touch.getPoint();
        // Calibração Vertical (X vira Y) - Ajustada para melhor sensibilidade nas bordas
        int16_t x = map(p.y, 200, 3900, 0, 240);
        int16_t y = map(p.x, 200, 3900, 0, 320);
        if(x < 0) x = 0; if(x >= 240) x = 239;
        if(y < 0) y = 0; if(y >= 320) y = 319;
        data->state = LV_INDEV_STATE_PR;
        data->point.x = x; data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void mostrarTelaProducao(Receita r) {
    lv_obj_clean(lv_scr_act());

    // Fundo Geral
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xF0F0F0), 0);

    // Cabeçalho Azul (Menor)
    lv_obj_t *header = lv_obj_create(lv_scr_act());
    lv_obj_set_size(header, 240, 60);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x003366), 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    // Nome do Produto (Truncado se for longo)
    lv_obj_t *lbl = lv_label_create(header);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lbl, 200);
    lv_label_set_text(lbl, r.descricao);
    lv_obj_center(lbl);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_22, 0); // Usando 22

    // Container do Contador (Card Branco no Meio)
    lv_obj_t * cardContador = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cardContador, 220, 160);
    lv_obj_center(cardContador);
    lv_obj_set_style_bg_color(cardContador, lv_color_white(), 0);
    lv_obj_set_style_radius(cardContador, 15, 0);
    lv_obj_set_style_shadow_width(cardContador, 20, 0);
    lv_obj_set_style_shadow_color(cardContador, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_shadow_opa(cardContador, 50, 0);
    lv_obj_clear_flag(cardContador, LV_OBJ_FLAG_SCROLLABLE);

    // Label "PRODUZIDO"
    lv_obj_t * lblTitulo = lv_label_create(cardContador);
    lv_label_set_text(lblTitulo, "PRODUZIDO");
    lv_obj_align(lblTitulo, LV_ALIGN_TOP_MID, 0, -5);
    lv_obj_set_style_text_color(lblTitulo, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_font(lblTitulo, &lv_font_montserrat_14, 0);

    // Contador Gigante
    static lv_obj_t * lblContador = NULL; 
    lblContador = lv_label_create(cardContador);
    lv_label_set_text(lblContador, "0");
    lv_obj_center(lblContador);
    lv_obj_set_style_text_font(lblContador, &lv_font_montserrat_32, 0);
    lv_obj_set_style_transform_zoom(lblContador, 800, 0); // Zoom 3x (Gigante)
    lv_obj_set_style_text_color(lblContador, lv_color_hex(0x003366), 0); // Azul Escuro
    
    lv_obj_set_user_data(lblContador, (void*)999); // Marcador para update

    // Meta (Pequeno abaixo do contador)
    char metaTxt[32];
    sprintf(metaTxt, "META: %d", r.quantidade);
    lv_obj_t *lblMeta = lv_label_create(cardContador);
    lv_label_set_text(lblMeta, metaTxt);
    lv_obj_align(lblMeta, LV_ALIGN_BOTTOM_MID, 0, 5);
    lv_obj_set_style_text_color(lblMeta, lv_color_hex(0x28a745), 0); // Verde
    lv_obj_set_style_text_font(lblMeta, &lv_font_montserrat_14, 0); // Usando 14


    // Botão Voltar - DIREITA E VERMELHO
    lv_obj_t * btnVoltar = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btnVoltar, 120, 60); 
    lv_obj_align(btnVoltar, LV_ALIGN_BOTTOM_RIGHT, -10, -10); // Canto Inferior Direito
    lv_obj_set_style_bg_color(btnVoltar, lv_color_hex(0xFF0000), 0); // Vermelho Vivo
    lv_obj_set_style_radius(btnVoltar, 10, 0);
    lv_obj_set_style_shadow_width(btnVoltar, 10, 0);
    lv_obj_set_style_shadow_opa(btnVoltar, 50, 0);
    lv_obj_add_event_cb(btnVoltar, event_handler_voltar, LV_EVENT_CLICKED, NULL);

    lv_obj_t * lblVoltar = lv_label_create(btnVoltar);
    lv_label_set_text(lblVoltar, "SAIR");
    lv_obj_center(lblVoltar);
    lv_obj_set_style_text_font(lblVoltar, &lv_font_montserrat_14, 0); // Usando 14
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
    mostrarListaSlave({});
}

void loopDisplay() {
    lv_timer_handler();
}