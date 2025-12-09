#ifndef NETWORK_SLAVE_H
#define NETWORK_SLAVE_H

#include <Arduino.h>
#include <vector>
#include "Common.h"

// Inicializa a rede (ESP-NOW Slave)
void setupNetworkSlave();

// Loop da rede (se precisar)
void loopNetworkSlave();

// Retorna a lista de receitas disponíveis
std::vector<Receita> getListaReceitas();

// Retorna se houve atualização na lista (para a tela redesenhar)
bool novaListaDisponivel();
void confirmarAtualizacaoLista();

#endif
