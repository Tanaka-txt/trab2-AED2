/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#ifndef FEATURES_H
#define FEATURES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// func1 - protótipos
void create_regi_bin(char *arq_csv, char *arq_bin);
void create_cabecalho();

// Func2- 
void read_bin(char *arq_bin);

// Func3-
void busca_bin(char *arq_bin);

// Func4-
int busca_por_rrn(char *arquivo, int posi_relativa);


#endif