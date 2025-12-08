#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "Common.h"
#include "DisplayManager.h"

const char* SSID_MASTER = "MASTER_PRODUCAO"; // Nome da rede do Master

Receita receitaAtual;
volatile bool novaMensagem = false;

// Acha o canal do Master
int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
    for (uint8_t i = 0; i < n; i++) {
      if (!strcmp(ssid, WiFi.SSID(i).c_str())) return WiFi.channel(i);
    }
  }
  return 0;
}

// Callback (Recebeu dados)
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len != sizeof(PacoteRede)) return;
  PacoteRede pacote;
  memcpy(&pacote, incomingData, sizeof(pacote));

  if (pacote.tipo == 1) {
    receitaAtual = pacote.dados;
    novaMensagem = true;
  }
}

void setup() {
  Serial.begin(115200);
  setupDisplay(); // Liga a tela primeiro
  
  WiFi.mode(WIFI_STA);
  // Procura e conecta no canal certo
  int32_t channel = getWiFiChannel(SSID_MASTER);
  if (channel > 0) {
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
  }

  if (esp_now_init() != ESP_OK) ESP.restart();
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}

void loop() {
  loopDisplay(); // Mantém a tela viva

  if (novaMensagem) {
    novaMensagem = false;
    mostrarTelaProducao(receitaAtual); // Atualiza o desenho
  }
}