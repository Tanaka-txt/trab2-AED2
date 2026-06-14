/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#include "impressao.h"

void imprimir_registro(reg_dados *registro) {
    // 1. codEstacao
    printf("%d ", registro->codEstacao);

    // 2. nomeEstacao (Tratando caso o nome seja vazio)
    if (registro->tamNomeEstacao == 0 || registro->nomeEstacao == NULL) {
        printf("NULO ");
    } else {
        printf("%s ", registro->nomeEstacao);
    }

    // 3. codLinha
    if (registro->codLinha == -1) {
        printf("NULO ");
    } else {
        printf("%d ", registro->codLinha);
    }

    // 4. nomeLinha
    if (registro->tamNomeLinha == 0 || registro->nomeLinha == NULL) {
        printf("NULO ");
    } else {
        printf("%s ", registro->nomeLinha);
    }

    // 5. codProxEstacao
    if (registro->codProxEstacao == -1) {
        printf("NULO ");
    } else {
        printf("%d ", registro->codProxEstacao);
    }

    // 6. distProxEstacao
    if (registro->distProxEstacao == -1) {
        printf("NULO ");
    } else {
        printf("%d ", registro->distProxEstacao);
    }

    // 7. codLinhaIntegra
    if (registro->codLinhaIntegra == -1) {
        printf("NULO ");
    } else {
        printf("%d ", registro->codLinhaIntegra);
    }

    // 8. codEstIntegra (Modificado para \n\n para dar o espaçamento de uma linha em branco ao final)
    if (registro->codEstIntegra == -1) {
        printf("NULO\n\n");
    } else {
        printf("%d\n\n", registro->codEstIntegra);
    }
}