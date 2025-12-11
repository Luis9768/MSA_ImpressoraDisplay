#include "NetworkSlave.h"
#include "Config.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ArduinoOTA.h>
#include <Preferences.h>

const char* SSID_MASTER = "MASTER_PRODUCAO";
std::vector<Receita> listaReceitas;
volatile bool listaAtualizada = false;
Preferences preferences;

// Salva uma receita na NVS
void salvarReceitaNVS(Receita r) {
    if (r.id <= 0 || r.id > MAX_RECEITAS) return;
    char key[16];
    sprintf(key, "rec_%d", r.id);
    preferences.putBytes(key, &r, sizeof(Receita));
}

// Carrega todas as receitas da NVS
void carregarReceitasNVS() {
    listaReceitas.clear();
    for (int i = 1; i <= MAX_RECEITAS; i++) {
        char key[16];
        sprintf(key, "rec_%d", i);
        if (preferences.isKey(key)) {
            Receita r;
            preferences.getBytes(key, &r, sizeof(Receita));
            if (r.ativa) {
                listaReceitas.push_back(r);
            }
        }
    }
    listaAtualizada = true;
    Serial.printf("Carregadas %d receitas da NVS\n", listaReceitas.size());
}

// Callback quando recebe dados
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    Serial.printf(">>> RECVD: %d bytes <<<\n", len);
    if (len != sizeof(PacoteRede)) return;
    
    PacoteRede pacote;
    memcpy(&pacote, incomingData, sizeof(pacote));

    Serial.printf(">>> PKT TIPO: %d <<<\n", pacote.tipo);

    if (pacote.tipo == 1) { // Receita Individual
        Receita r = pacote.dados;
        
        // Salva na NVS imediatamente
        salvarReceitaNVS(r);

        // Verifica se já existe na lista
        bool encontrado = false;
        for (auto &item : listaReceitas) {
            if (item.id == r.id) {
                if (r.ativa) {
                    item = r; // Atualiza
                } else {
                    item.ativa = false; 
                }
                encontrado = true;
                break;
            }
        }

        if (!encontrado && r.ativa) {
            listaReceitas.push_back(r);
        }

        // Limpeza de inativos
        for (auto it = listaReceitas.begin(); it != listaReceitas.end(); ) {
            if (!it->ativa) {
                it = listaReceitas.erase(it);
            } else {
                ++it;
            }
        }

        listaAtualizada = true;
    } else if (pacote.tipo == 2) { // RESET TOTAL (Vindo do Master)
        Serial.println(">>> COMANDO DE RESET TOTAL RECEBIDO <<<");
        listaReceitas.clear();
        preferences.clear(); // Limpa NVS
        preferences.putInt("reset_done", 1); // Mantém flag de reset inicial
        listaAtualizada = true;
    }
}

int32_t getWiFiChannel(const char *ssid) {
    if (int32_t n = WiFi.scanNetworks()) {
        for (uint8_t i = 0; i < n; i++) {
            if (!strcmp(ssid, WiFi.SSID(i).c_str())) return WiFi.channel(i);
        }
    }
    return 0;
}

void setupNetworkSlave() {
    // Inicia NVS
    preferences.begin("slave_db_v5", false);
    
    // Reset Forcado na primeira vez
    if (preferences.getInt("reset_done", 0) == 0) {
        preferences.clear();
        preferences.putInt("reset_done", 1);
        Serial.println(">>> SLAVE MEMORY RESET (V5) <<<");
    }

    carregarReceitasNVS();

    // --- INÍCIO CONFIGURAÇÃO OTA ---
    // Tenta conectar ao WiFi do Master para permitir OTA
    Serial.printf("Tentando conectar ao WiFi: %s\n", SSID_MASTER);
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID_MASTER, OTA_WIFI_PASS);
    
    // Aguarda conexão por alguns segundos (não bloqueante eternamente)
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
        delay(100);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("WiFi Conectado! IP: %s\n", WiFi.localIP().toString().c_str());
        
        // Configura Hostname com parte do MAC para ser único
        String hostname = OTA_HOSTNAME_PREFIX;
        hostname += "_";
        byte mac[6];
        WiFi.macAddress(mac);
        hostname += String(mac[5], HEX); // Usa ultimo byte do MAC
        ArduinoOTA.setHostname(hostname.c_str());
        
        ArduinoOTA
            .onStart([]() {
                String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
                Serial.println("Start updating " + type);
            })
            .onEnd([]() {
                Serial.println("\nEnd");
            })
            .onProgress([](unsigned int progress, unsigned int total) {
                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
            })
            .onError([](ota_error_t error) {
                Serial.printf("Error[%u]: ", error);
                if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
                else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
                else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
                else if (error == OTA_END_ERROR) Serial.println("End Failed");
            });

        ArduinoOTA.begin();
        Serial.println("OTA Iniciado e Pronto.");
        
        // Se conectou, o canal já está configurado pelo WiFi.begin
        // Mas para garantir o ESP-NOW, vamos verificar o canal
        int32_t channel = WiFi.channel();
         Serial.printf("Canal WiFi Atual: %d\n", channel);

    } else {
        Serial.println("Falha ao conectar WiFi. Modo Offline (apenas ESP-NOW via Scan).");
        // Fallback: Tenta encontrar o canal manual se não conectou
        int32_t channel = getWiFiChannel(SSID_MASTER);
        if (channel > 0) {
            esp_wifi_set_promiscuous(true);
            esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
            esp_wifi_set_promiscuous(false);
            Serial.printf("Canal forçado para %d (Sem conexão WiFi)\n", channel);
        } else {
            Serial.println("Master nao encontrado no Scan! Usando canal padrao (1).");
        }
    }
    // --- FIM CONFIGURAÇÃO OTA ---

    if (esp_now_init() != ESP_OK) {
        Serial.println("Erro ao iniciar ESP-NOW");
        return;
    }
    
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    Serial.println("Slave ESP-NOW Ativo.");
}

void loopNetworkSlave() {
    ArduinoOTA.handle();
}

std::vector<Receita> getListaReceitas() {
    return listaReceitas;
}

bool novaListaDisponivel() {
    return listaAtualizada;
}

void confirmarAtualizacaoLista() {
    listaAtualizada = false;
}
