#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include "Common.h"

void setupNetwork();
void loopNetwork();
void enviarReceitaParaSlaves(Receita r); // Manda para os escravos

#endif