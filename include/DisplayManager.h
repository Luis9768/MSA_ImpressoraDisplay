#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "Common.h"
#include <vector>

void setupDisplay();
void loopDisplay();

// Mostra tela de seleção em Carousel (Card por Card)
void mostrarCarouselSlave(std::vector<Receita> lista, int indice);

// Funções de navegação do Carousel
void proximoProdutoCarousel();
void entrarProdutoCarousel();

// Mostra a tela de contagem/produção
void mostrarTelaProducao(Receita r);

// Atualiza o número no contador
void atualizarContador(int qtd);

// Mostra alerta de caixa cheia
void mostrarAlertaCaixaCheia();

// Verifica toques na tela e retorna o ID do produto clicado (ou -1 se nenhum)
// Se estiver na tela de produção, retorna -2 para "Sair"
int verificarToque();

#endif