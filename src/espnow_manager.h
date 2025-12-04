#ifndef ESPNOW_MANAGER_H
#define ESPNOW_MANAGER_H

#include <esp_now.h>
#include <WiFi.h>
#include "app_api.h"

// Endereço de Broadcast (Envia para todo mundo)
static uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Inicia o ESP-NOW
void setupEspNowMaster() {
  // Importante: Modo AP_STA (AP para o celular, STA para o ESP-NOW)
  WiFi.mode(WIFI_AP_STA); 
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao inicializar ESP-NOW");
    return;
  }

  // Registra o peer de Broadcast
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Erro ao adicionar Peer Broadcast");
  } else {
    Serial.println("ESP-NOW Mestre Pronto!");
  }
}

// Converte String para Char e envia
void enviarDadosEspNow(Receita r, bool apagar) {
  struct_mensagem msg;
  
  msg.id = r.id;
  msg.comandoApagar = apagar;
  
  // Conversão segura de String para Char Array
  r.codigo.toCharArray(msg.codigo, 32);
  r.quantidade.toCharArray(msg.quantidade, 10);
  r.descricao.toCharArray(msg.descricao, 64);
  r.barcode.toCharArray(msg.barcode, 32);

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &msg, sizeof(msg));
   
  if (result == ESP_OK) {
    Serial.printf("Enviado ID %d via ESP-NOW (Apagar=%d)\n", r.id, apagar);
  } else {
    Serial.println("Falha no envio ESP-NOW");
  }
}

#endif