#ifndef LEITURA_H
#define LEITURA_H

#include "features.h"
#include "registro.h"

int ler_registro(FILE *binario, reg_dados *registro);

void le_linha_csv(char *linha, reg_dados *registro, int topo);

#endif