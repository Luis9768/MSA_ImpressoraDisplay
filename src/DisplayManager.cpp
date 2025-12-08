#include "DisplayManager.h"
#include "Config.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

TFT_eSPI tft = TFT_eSPI(); 
SPIClass touchSpi(VSPI);
XPT2046_Touchscreen touch(TOUCH_XPT_CS, TOUCH_XPT_IRQ);

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
        // Calibração Vertical (X vira Y)
        int16_t x = map(p.y, 240, 3800, 0, 240);
        int16_t y = map(p.x, 200, 3700, 0, 320);
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

    // Cabeçalho Azul
    lv_obj_t *header = lv_obj_create(lv_scr_act());
    lv_obj_set_size(header, 240, 90);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x003366), 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    
    // Nome do Produto
    lv_obj_t *lbl = lv_label_create(header);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lbl, 210);
    lv_label_set_text(lbl, r.descricao);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lbl);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_22, 0);

    // Meta
    char metaTxt[32];
    sprintf(metaTxt, "META: %d CX", r.quantidade);
    lv_obj_t *lblMeta = lv_label_create(lv_scr_act());
    lv_label_set_text(lblMeta, metaTxt);
    lv_obj_align(lblMeta, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_set_style_text_color(lblMeta, lv_palette_main(LV_PALETTE_GREY), 0);

    // Contador Gigante
    lv_obj_t *lblContador = lv_label_create(lv_scr_act());
    lv_label_set_text(lblContador, "0");
    lv_obj_center(lblContador);
    lv_obj_set_style_text_font(lblContador, &lv_font_montserrat_32, 0);
    lv_obj_set_style_transform_zoom(lblContador, 512, 0); // Zoom 2x
    lv_obj_set_style_text_color(lblContador, lv_color_hex(0x28a745), 0);

    // Rodapé
    lv_obj_t *footer = lv_label_create(lv_scr_act());
    lv_label_set_text(footer, "STATUS: OK");
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_color(footer, lv_color_hex(0xFF9900), 0);
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

    // Tela de Espera
    lv_obj_t *lbl = lv_label_create(lv_scr_act());
    lv_label_set_text(lbl, "SLAVE\nVERTICAL\n\nAguardando...");
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lbl);
}

void loopDisplay() {
    lv_timer_handler();
}