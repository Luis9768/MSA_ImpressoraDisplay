#ifndef COMMON_H   // <--- Verifique se escreveu #ifndef e não #ifdef
#define COMMON_H

#include <Arduino.h> // Importante para entender tipos como 'bool'

// Estrutura da Receita
struct Receita {
    int id;               
    char codigo[16];      
    int quantidade;       
    char descricao[32];   
    char barcode[16];     
    bool ativa;           
};

// Pacote de Rede
struct PacoteRede {
    int tipo; 
    Receita dados;
};

#endif