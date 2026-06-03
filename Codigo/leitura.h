/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#ifndef LEITURA_H
#define LEITURA_H

#include "features.h"
#include "registro.h"

int ler_registro(FILE *binario, reg_dados *registro);

void le_linha_csv(char *linha, reg_dados *registro, int topo);

#endif