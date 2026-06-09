/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#ifndef INDICE_H
#define INDICE_H

#include <stdio.h>

void remover_do_indice(FILE *f_indice, int cod_alvo);
void inserir_no_indice(FILE *f_indice, int cod_alvo, int rrn_alvo);
int buscar_no_indice(FILE *f_indice, int cod_alvo, int *rrn_encontrado);

#endif