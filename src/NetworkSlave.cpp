#include "NetworkSlave.h"
#include "Config.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
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
    if (len != sizeof(PacoteRede)) return;
    
    PacoteRede pacote;
    memcpy(&pacote, incomingData, sizeof(pacote));

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
    preferences.begin("slave_db", false);
    carregarReceitasNVS();

    WiFi.mode(WIFI_STA);
    
    int32_t channel = getWiFiChannel(SSID_MASTER);
    if (channel > 0) {
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        esp_wifi_set_promiscuous(false);
        Serial.printf("Conectado ao canal %d do Master\n", channel);
    } else {
        Serial.println("Master nao encontrado! Usando canal 1.");
    }

    if (esp_now_init() != ESP_OK) {
        Serial.println("Erro ao iniciar ESP-NOW");
        return;
    }
    
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
    Serial.println("Slave Iniciado e Ouvindo...");
}

void loopNetworkSlave() {
    // Nada por enquanto
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
