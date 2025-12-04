#ifndef APP_API_H
#define APP_API_H

#include <Arduino.h>

// --- Estrutura para TRANSPORTE via ESP-NOW (Tamanho Fixo) ---
typedef struct struct_mensagem {
  int id;
  char codigo[32];
  char quantidade[10];
  char descricao[64];
  char barcode[32];
  bool comandoApagar; 
} struct_mensagem;

// Definição da estrutura da Receita
// Isso ensina para o sistema o que compõe uma "Receita" (ID, código, quantidade...)
struct Receita {
  int id;
  String codigo;
  String quantidade;
  String descricao;
  String barcode;
};

// Declaração das variáveis globais
// O comando 'extern' significa: "Isso existe em outro lugar, não crie de novo, apenas use".
extern Receita receitas[];
extern int totalReceitas;
extern int currentID;
extern const int MAX_RECEITAS;

// Funções que o WebServer precisa chamar no Main
void salvarReceitasEEPROM();
void mostrarReceitaLVGL(int id);

#endif