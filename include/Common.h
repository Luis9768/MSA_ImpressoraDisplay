#ifndef COMMON_H
#define COMMON_H
#include <Arduino.h>

struct Receita {
    int id;               
    char codigo[16];      
    int quantidade;       
    char descricao[32];   
    char barcode[16];     
    bool ativa;           
};

struct PacoteRede {
    int tipo; 
    Receita dados;
};
#endif