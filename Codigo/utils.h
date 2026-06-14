#ifndef UTILS_H
#define UTILS_H

#include "registro.h"
#include <stdio.h>

// 1. PADRONIZAÇÃO DA STRUCT DE ÍNDICE
// Única declaração para o projeto inteiro. 'IndiceEntry9' deixa de existir.
typedef struct {
    int cod;
    int rrn;
} IndiceEntry;

// 2. PADRONIZAÇÃO DAS ASSINATURAS (Protótipos)
int compare_cod(const void *a, const void *b);
int validar_cabecalho(FILE *fdados, reg_cabecalho *cab);
void liberar_strings_registro(reg_dados *reg);

// Verifica se um registro atende aos critérios de busca
int atende_filtros_geral(reg_dados *reg, int m, char nomesCampos[][50], char valoresStr[][100], int valoresInt[]);

void ler_criterio_int(int *flag_busca, int *valor_busca);

#endif