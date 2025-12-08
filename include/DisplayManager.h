#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>
#include "Common.h"

void setupDisplay();
void loopDisplay();
void mostrarTelaProducao(Receita r);

#endif