#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include "Common.h"

// Apenas dizemos que essas funções existem
void setupStorage();
void salvarReceitaMemoria(Receita r);
Receita carregarReceitaMemoria(int id);
int getTotalReceitas();
void limparMemoria();
void desativarReceita(int id);

#endif