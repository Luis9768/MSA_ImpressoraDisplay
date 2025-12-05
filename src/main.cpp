#include <Arduino.h>
#include "Config.h"
#include "Common.h" // Busca de include/
#include "StorageManager.h" 
#include "NetworkManager.h"
#include "DisplayManager.h" // Confirme se o DisplayManager.h está na pasta include!

void setup() {
    Serial.begin(115200);
    
    // 1. Storage
    setupStorage();
    
    // 2. Display (Liga o Backlight pino 21)
    setupDisplay(); 
    
    // 3. Rede
    setupNetwork();
    
    Serial.println(">>> MASTER INICIADO <<<");
}

void loop() {
    // Processos contínuos (SEM DELAY)
    loopNetwork();
    loopDisplay();
    
    // Exemplo: piscar um LED ou imprimir status a cada 1 segundo (sem travar)
    static unsigned long ultimoTempo = 0;
    if (millis() - ultimoTempo > 1000) {
        ultimoTempo = millis();
        // Aqui você pode atualizar o relógio na tela, etc.
        // Serial.println("Sistema rodando...");
    }
}