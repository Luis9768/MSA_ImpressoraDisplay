#include "StorageManager.h"
#include <Preferences.h>
#include "Config.h"

Preferences preferences;
Receita cacheReceitas[MAX_RECEITAS + 1]; // Cache em RAM (Index 1 a 50)
int totalReceitasRAM = 0;

void setupStorage() {
    preferences.begin("dados-prod", false);
    
    // VERIFICACAO DE VERSAO PARA FORMATACAO
    int versao = preferences.getInt("versao_db", 0);
    if (versao != 1) { 
        Serial.println(">>> NOVA VERSAO DETECTADA: FORMATANDO MEMORIA...");
        preferences.clear();
        preferences.putInt("versao_db", 1);
        Serial.println(">>> MEMORIA FORMATADA COM SUCESSO!");
    }

    totalReceitasRAM = preferences.getInt("total", 0); // Carrega para RAM
    Serial.printf(">>> STORAGE INIT: Total Receitas (RAM) = %d\n", totalReceitasRAM);
    
    if (totalReceitasRAM > MAX_RECEITAS || totalReceitasRAM < 0) {
        Serial.println(">>> ERRO: Memoria corrompida detectada! Resetando total para 0.");
        preferences.putInt("total", 0);
        totalReceitasRAM = 0;
    }

    Serial.println(">>> CARREGANDO CACHE PARA RAM...");
    for (int i = 1; i <= MAX_RECEITAS; i++) {
        char chave[10];
        sprintf(chave, "rec_%d", i);
        size_t len = preferences.getBytes(chave, &cacheReceitas[i], sizeof(Receita));
        
        if (len == 0) {
            cacheReceitas[i].id = i;
            cacheReceitas[i].ativa = false;
            strcpy(cacheReceitas[i].descricao, "Vazio");
        }
    }
    Serial.println(">>> CACHE CARREGADO!");
    preferences.end();
}

void salvarReceitaMemoria(Receita r) {
    if (r.id < 1 || r.id > MAX_RECEITAS) return;

    // 1. Atualiza RAM
    cacheReceitas[r.id] = r;

    // 2. Atualiza Flash
    preferences.begin("dados-prod", false);
    char chave[10];
    sprintf(chave, "rec_%d", r.id);
    preferences.putBytes(chave, &r, sizeof(Receita));
    
    if (r.id > totalReceitasRAM) {
        totalReceitasRAM = r.id;
        preferences.putInt("total", totalReceitasRAM);
    }
    preferences.end();
    
    Serial.printf("Receita ID %d salva (RAM+FLASH)!\n", r.id);
}

Receita carregarReceitaMemoria(int id) {
    if (id < 1 || id > MAX_RECEITAS) {
        Receita r; r.id = 0; r.ativa = false; return r;
    }
    // Retorna direto da RAM (Instantaneo)
    return cacheReceitas[id];
}

int getTotalReceitas() {
    return totalReceitasRAM;
}

void limparMemoria() {
    preferences.begin("dados-prod", false);
    preferences.clear();
    preferences.putInt("versao_db", 1);
    preferences.end();

    // Limpa RAM tambem
    for (int i = 1; i <= MAX_RECEITAS; i++) {
        cacheReceitas[i].id = i;
        cacheReceitas[i].ativa = false;
        strcpy(cacheReceitas[i].descricao, "Vazio");
    }
    Serial.println(">>> MEMORIA LIMPA (RAM+FLASH)!");
}

void desativarReceita(int id) {
    if (id < 1 || id > MAX_RECEITAS) return;

    // 1. Atualiza RAM
    cacheReceitas[id].ativa = false;

    // 2. Atualiza Flash
    preferences.begin("dados-prod", false);
    char chave[10];
    sprintf(chave, "rec_%d", id);
    preferences.putBytes(chave, &cacheReceitas[id], sizeof(Receita));
    preferences.end();

    Serial.printf("Receita ID %d desativada (RAM+FLASH).\n", id);
}