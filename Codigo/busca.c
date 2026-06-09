/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#include "busca.h"
#include "fornecidas.h" // Para usar o ScanQuoteString

Painel_Busca montar_painel(int m) {
    Painel_Busca painel = {0}; // Inicializa tudo zerado para evitar lixo de memória
    char palavra[30], aux[50];

    for (int j = 0; j < m; j++) {
        scanf("%s", palavra);

        if (strcmp(palavra, "codEstacao") == 0) {
            painel.busca_codEstacao = 1;
            scanf("%s", aux);
            if (strcmp(aux, "NULO") == 0) painel.valor_codEstacao = -1;
            else painel.valor_codEstacao = atoi(aux);
        } else if (strcmp(palavra, "nomeEstacao") == 0) {
            painel.busca_nomeEstacao = 1;
            ScanQuoteString(painel.valor_nomeEstacao);
        } else if (strcmp(palavra, "codLinha") == 0) {
            painel.busca_codLinha = 1;
            scanf("%s", aux);
            if (strcmp(aux, "NULO") == 0) painel.valor_codLinha = -1;
            else painel.valor_codLinha = atoi(aux);
        } else if (strcmp(palavra, "nomeLinha") == 0) {
            painel.busca_nomeLinha = 1;
            ScanQuoteString(painel.valor_nomeLinha);
        } else if (strcmp(palavra, "codProxEstacao") == 0) {
            painel.busca_codProxEstacao = 1;
            scanf("%s", aux);
            if (strcmp(aux, "NULO") == 0) painel.valor_codProxEstacao = -1;
            else painel.valor_codProxEstacao = atoi(aux);
        } else if (strcmp(palavra, "distProxEstacao") == 0) {
            painel.busca_distProxEstacao = 1;
            scanf("%s", aux);
            if (strcmp(aux, "NULO") == 0) painel.valor_distProxEstacao = -1;
            else painel.valor_distProxEstacao = atoi(aux);
        } else if (strcmp(palavra, "codLinhaIntegra") == 0) {
            painel.busca_codLinhaIntegra = 1;
            scanf("%s", aux);
            if (strcmp(aux, "NULO") == 0) painel.valor_codLinhaIntegra = -1;
            else painel.valor_codLinhaIntegra = atoi(aux);
        } else if (strcmp(palavra, "codEstIntegra") == 0) {
            painel.busca_codEstIntegra = 1;
            scanf("%s", aux);
            if (strcmp(aux, "NULO") == 0) painel.valor_codEstIntegra = -1;
            else painel.valor_codEstIntegra = atoi(aux);
        }
    }
    return painel;
}