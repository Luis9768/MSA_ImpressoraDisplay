#ifndef PRINTER_MANAGER_H
#define PRINTER_MANAGER_H

#include <Arduino.h>
#include "Common.h"

void setupPrinter();
void imprimirEtiqueta(Receita r, int contador);

#endif
