#include "DisplayManager.h"
#include "Config.h"
#include "StorageManager.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// --- Hardware ---
TFT_eSPI tft = TFT_eSPI(); 
SPIClass touchSpi(VSPI);
XPT2046_Touchscreen touch(TOUCH_XPT_CS, TOUCH_XPT_IRQ);

// --- LVGL ---
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * 10]; // Reduzido para economizar RAM (foi o que causou overflow)
lv_obj_t *mainContainer = NULL; // Onde ficam os cards

// Funções de Hardware (Flush/Touch)
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
        int16_t x = map(p.x, 200, 3700, 0, SCREEN_WIDTH);
        int16_t y = map(p.y, 240, 3800, 0, SCREEN_HEIGHT);
        if(x < 0) x = 0; if(x >= SCREEN_WIDTH) x = SCREEN_WIDTH - 1;
        if(y < 0) y = 0; if(y >= SCREEN_HEIGHT) y = SCREEN_HEIGHT - 1;
        data->state = LV_INDEV_STATE_PR;
        data->point.x = x; data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// === FUNÇÃO AUXILIAR: CRIA UM CARD DE PRODUTO ===
void criarCardProduto(Receita r) {
    // 1. O Cartão Branco (Base)
    lv_obj_t * card = lv_obj_create(mainContainer);
    lv_obj_set_size(card, lv_pct(100), 75); // Largura total, 75px altura
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_radius(card, 12, 0); // Bordas arredondadas
    lv_obj_set_style_border_width(card, 0, 0); // Sem borda preta
    // Sombra suave
    lv_obj_set_style_shadow_width(card, 20, 0);
    lv_obj_set_style_shadow_color(card, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_shadow_opa(card, 30, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE); // O cartão não rola, só a lista

    // 2. Faixa Colorida Lateral (Estilo MSA)
    lv_obj_t * stripe = lv_obj_create(card);
    lv_obj_set_size(stripe, 6, lv_pct(100));
    lv_obj_align(stripe, LV_ALIGN_LEFT_MID, -15, 0); // Cola na esquerda (compensa o padding do card)
    lv_obj_set_style_bg_color(stripe, lv_color_hex(0x003366), 0); // Azul MSA
    lv_obj_set_style_border_width(stripe, 0, 0);

    // 3. Código do Produto (Título)
    lv_obj_t * lblCodigo = lv_label_create(card);
    lv_label_set_text(lblCodigo, r.codigo);
    lv_obj_align(lblCodigo, LV_ALIGN_TOP_LEFT, 10, -5);
    lv_obj_set_style_text_color(lblCodigo, lv_color_hex(0x003366), 0);
    
    // 4. Descrição e ID (Subtítulo)
    lv_obj_t * lblDesc = lv_label_create(card);
    char bufDesc[64];
    sprintf(bufDesc, "ID: %d  |  %s", r.id, r.descricao);
    lv_label_set_text(lblDesc, bufDesc);
    lv_obj_align(lblDesc, LV_ALIGN_BOTTOM_LEFT, 10, 5);
    lv_obj_set_style_text_color(lblDesc, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_text_font(lblDesc, lv_font_default(), 0); // Usa fonte padrão

    // 5. Badge de Quantidade (Pílula Verde)
    lv_obj_t * badge = lv_obj_create(card);
    lv_obj_set_size(badge, 70, 26);
    lv_obj_align(badge, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(badge, lv_color_hex(0x28a745), 0); // Verde Sucesso
    lv_obj_set_style_radius(badge, 13, 0); // Formato Pílula
    lv_obj_set_style_border_width(badge, 0, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * lblQtd = lv_label_create(badge);
    char bufQtd[10];
    sprintf(bufQtd, "%d", r.quantidade);
    lv_label_set_text(lblQtd, bufQtd);
    lv_obj_center(lblQtd);
    lv_obj_set_style_text_color(lblQtd, lv_color_white(), 0);
}

// === ATUALIZA A TELA INTEIRA ===
void atualizarListaProdutos() {
    // Se o container principal não existe, cria (na primeira vez)
    if (mainContainer == NULL) {
        // Cabeçalho Fixo Azul
        lv_obj_t * header = lv_obj_create(lv_scr_act());
        lv_obj_set_size(header, lv_pct(100), 50);
        lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(header, lv_color_hex(0x003366), 0);
        lv_obj_set_style_radius(header, 0, 0);
        lv_obj_set_style_border_width(header, 0, 0);
        
        lv_obj_t * title = lv_label_create(header);
        lv_label_set_text(title, "MSA PRODUCTION");
        lv_obj_center(title);
        lv_obj_set_style_text_color(title, lv_color_white(), 0);

        // Container de Rolagem (Onde ficam os cards)
        mainContainer = lv_obj_create(lv_scr_act());
        lv_obj_set_size(mainContainer, lv_pct(100), SCREEN_HEIGHT - 50);
        lv_obj_align(mainContainer, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(mainContainer, lv_color_hex(0xF0F2F5), 0); // Fundo Cinza Claro
        lv_obj_set_style_border_width(mainContainer, 0, 0);
        lv_obj_set_style_pad_all(mainContainer, 10, 0); // Margem geral
        
        // Define que os itens ficam um embaixo do outro (Flex Layout)
        lv_obj_set_flex_flow(mainContainer, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(mainContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        // Espaço entre os cards:
        lv_obj_set_style_pad_row(mainContainer, 15, 0); 
    } else {
        // Se já existe, limpa os cards antigos
        lv_obj_clean(mainContainer);
    }

    int total = getTotalReceitas();
    bool temItem = false;

    for (int i = 1; i <= total; i++) {
        Receita r = carregarReceitaMemoria(i);
        if (r.ativa) {
            temItem = true;
            criarCardProduto(r);
        }
    }

    if (!temItem) {
        lv_obj_t *aviso = lv_label_create(mainContainer);
        lv_label_set_text(aviso, "Nenhum produto.\nAcesse o Wi-Fi para criar.");
        lv_obj_set_style_text_align(aviso, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(aviso, lv_palette_main(LV_PALETTE_GREY), 0);
    }
}

void setupDisplay() {
    pinMode(21, OUTPUT); digitalWrite(21, HIGH); // Luz ON
    
    tft.begin(); tft.setRotation(0); tft.fillScreen(TFT_BLACK);
    touchSpi.begin(TOUCH_XPT_CLK, TOUCH_XPT_MISO, TOUCH_XPT_MOSI, TOUCH_XPT_CS);
    touch.begin(touchSpi); touch.setRotation(0);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH; disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush; disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touch_read;
    lv_indev_drv_register(&indev_drv);

    // Inicia a Interface
    atualizarListaProdutos();
}

void loopDisplay() {
    lv_timer_handler();
}