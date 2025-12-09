#ifndef CONFIG_H
#define CONFIG_H

// --- Wi-Fi & Rede ---
#define WIFI_SSID "MASTER_PRODUCAO" // Nome da rede que vai aparecer
#define WIFI_PASS "12345678"        // Senha
#define HOSTNAME  "master-cyd"

// --- Hardware do CYD (ESP Amarelo) ---
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

// Pinos do Touch (XPT2046) - O CYD usa um barramento SPI separado para o touch
#define TOUCH_XPT_CS   33
#define TOUCH_XPT_IRQ  36
#define TOUCH_XPT_MOSI 32
#define TOUCH_XPT_MISO 39
#define TOUCH_XPT_CLK  25

// LED RGB (Que fica atrás da placa) - ATENÇÃO: Eles ligam com LOW (Invertido)
#define CYD_LED_RED    4
#define CYD_LED_GREEN  16
#define CYD_LED_BLUE   17

// Conector de Expansão (CN1 - Lateral)
// Use estes pinos se precisar ligar sensores extras no Master
#define PINO_EXTRA_1   22
#define PINO_EXTRA_2   27 

// --- Limites ---
#define MAX_RECEITAS 50

#endif