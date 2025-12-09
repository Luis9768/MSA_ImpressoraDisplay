#include "StorageManager.h" // Chama o header da pasta include
#include <Preferences.h>

Preferences preferences;

void setupStorage() {
    preferences.begin("dados-prod", false);
}

void salvarReceitaMemoria(Receita r) {
    char chave[10];
    sprintf(chave, "rec_%d", r.id);
    preferences.putBytes(chave, &r, sizeof(Receita));
    
    int total = preferences.getInt("total", 0);
    if (r.id > total) {
        preferences.putInt("total", r.id);
    }
    Serial.printf("Receita ID %d salva!\n", r.id);
}

Receita carregarReceitaMemoria(int id) {
    Receita r;
    char chave[10];
    sprintf(chave, "rec_%d", id);
    
    size_t len = preferences.getBytes(chave, &r, sizeof(Receita));
    if (len == 0) {
        r.id = id;
        r.ativa = false;
        strcpy(r.descricao, "Vazio"); 
    }
    return r;
}

int getTotalReceitas() {
    return preferences.getInt("total", 0);
}

void limparMemoria() {
    preferences.clear();
}

void desativarReceita(int id) {
    Receita r = carregarReceitaMemoria(id);
    if (r.id != 0) { // Se existe
        r.ativa = false;
        salvarReceitaMemoria(r);
        Serial.printf("Receita ID %d desativada.\n", id);
    }
}