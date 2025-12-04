#pragma once
#include <Arduino.h>

struct Receita {
  int id;
  String codigo;
  String quantidade;
  String descricao;
  String barcode;
};

extern const int MAX_RECEITAS;
extern Receita receitas[];
extern int totalReceitas;
extern int currentID;

void salvarReceitasEEPROM();
void mostrarReceitaLVGL(int id);
