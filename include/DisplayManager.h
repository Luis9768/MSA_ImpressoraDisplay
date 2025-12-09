#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "Common.h"
#include <vector>

void setupDisplay();
void loopDisplay();

// Mostra a lista de produtos recebidos do Master
void mostrarListaSlave(std::vector<Receita> lista);

// Mostra a tela de contagem/produção
void mostrarTelaProducao(Receita r);

// Atualiza o número no contador
void atualizarContador(int qtd);

// Verifica toques na tela e retorna o ID do produto clicado (ou -1 se nenhum)
// Se estiver na tela de produção, retorna -2 para "Sair"
int verificarToque();

#endif