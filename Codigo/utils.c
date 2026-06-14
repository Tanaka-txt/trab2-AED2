#include "utils.h"
#include <stdlib.h>
#include <string.h>

// essa função compara primeiro pelo codigo e, em caso de empate, pelo RRN isso garante uma ordenação consistente e estavel.
int compare_cod(const void *a, const void *b) {
    IndiceEntry *ia = (IndiceEntry*)a;
    IndiceEntry *ib = (IndiceEntry*)b;
    if (ia->cod != ib->cod) return ia->cod - ib->cod;
    return ia->rrn - ib->rrn;
}

// essa função lê o cabeçalho do arquivo binário e preenche a struct reg_cabecalho
int validar_cabecalho(FILE *fdados, reg_cabecalho *cab) {
    fseek(fdados, 0, SEEK_SET);
    fread(&cab->status, 1, 1, fdados);
    fread(&cab->topo, 4, 1, fdados);
    fread(&cab->proxRRN, 4, 1, fdados);
    fread(&cab->nroEstacoes, 4, 1, fdados);
    fread(&cab->nroParesEstacoes, 4, 1, fdados);
    
    return (cab->status == '1') ? 1 : 0;
}

// essa função garante que não haja vazamento de memória
void liberar_strings_registro(reg_dados *reg) {
    if (reg->tamNomeEstacao > 0 && reg->nomeEstacao != NULL) {
        free(reg->nomeEstacao);
        reg->nomeEstacao = NULL;
    }
    if (reg->tamNomeLinha > 0 && reg->nomeLinha != NULL) {
        free(reg->nomeLinha);
        reg->nomeLinha = NULL;
    }
}

// ela verifica se um registro atende a todos os critérios de busca fornecidos pelo usuário
int atende_filtros_geral(reg_dados *reg, int m, char nomesCampos[][50], char valoresStr[][100], int valoresInt[]) {
    for (int j = 0; j < m; j++) {
        if (strcmp(nomesCampos[j], "codEstacao") == 0) {
            if (reg->codEstacao != valoresInt[j]) return 0;
        } else if (strcmp(nomesCampos[j], "codLinha") == 0) {
            if (reg->codLinha != valoresInt[j]) return 0;
        } else if (strcmp(nomesCampos[j], "codProxEstacao") == 0) {
            if (reg->codProxEstacao != valoresInt[j]) return 0;
        } else if (strcmp(nomesCampos[j], "distProxEstacao") == 0) {
            if (reg->distProxEstacao != valoresInt[j]) return 0;
        } else if (strcmp(nomesCampos[j], "codLinhaIntegra") == 0) {
            if (reg->codLinhaIntegra != valoresInt[j]) return 0;
        } else if (strcmp(nomesCampos[j], "codEstIntegra") == 0) {
            if (reg->codEstIntegra != valoresInt[j]) return 0;
        } else if (strcmp(nomesCampos[j], "nomeEstacao") == 0) {
            if (valoresStr[j][0] == '\0') {
                if (reg->nomeEstacao != NULL && reg->tamNomeEstacao > 0) return 0;
            } else {
                if (reg->nomeEstacao == NULL) return 0;
                if (strcmp(reg->nomeEstacao, valoresStr[j]) != 0) return 0;
            }
        } else if (strcmp(nomesCampos[j], "nomeLinha") == 0) {
            if (valoresStr[j][0] == '\0') {
                if (reg->nomeLinha != NULL && reg->tamNomeLinha > 0) return 0;
            } else {
                if (reg->nomeLinha == NULL) return 0;
                if (strcmp(reg->nomeLinha, valoresStr[j]) != 0) return 0;
            }
        }
    }
    return 1;
}

// Função auxiliar para ler campos inteiros sem repetir código (func3.c)
void ler_criterio_int(int *flag_busca, int *valor_busca) {
    char aux[50];
    *flag_busca = 1; // Liga a flag no painel
    scanf("%s", aux);
    if (strcmp(aux, "NULO") == 0) {
        *valor_busca = -1;
    } else {
        *valor_busca = atoi(aux);
    }
}