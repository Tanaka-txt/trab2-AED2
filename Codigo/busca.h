/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#ifndef BUSCA_H
#define BUSCA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Estrutura que agrupa as flags e os valores dos critérios de busca
typedef struct {
    int busca_codEstacao; int valor_codEstacao;
    int busca_nomeEstacao; char valor_nomeEstacao[50];
    int busca_codLinha; int valor_codLinha;
    int busca_nomeLinha; char valor_nomeLinha[50];
    int busca_codProxEstacao; int valor_codProxEstacao;
    int busca_distProxEstacao; int valor_distProxEstacao;
    int busca_codLinhaIntegra; int valor_codLinhaIntegra;
    int busca_codEstIntegra; int valor_codEstIntegra;
} Painel_Busca;

// Protótipo da função que vai ler o teclado e montar o painel
Painel_Busca montar_painel(int m);

#endif