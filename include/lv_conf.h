#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

// --- Cores ---
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0 

// --- Memória (Isso corrige 90% dos erros de ESP32) ---
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (48U * 1024U) // 48KB de RAM para o LVGL

// --- Tempo (Usa o millis do Arduino) ---
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

// --- Fontes (Desative as que não usa para evitar erro de compilação) ---
// Vamos habilitar apenas a padrão por enquanto
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

// --- Performance ---
#define LV_SHADOW_CACHE_SIZE 0
#define LV_IMG_CACHE_DEF_SIZE 0

// --- Log (Ajuda a ver erros no Serial) ---
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_INFO
#define LV_LOG_PRINTF 1

#endif