/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

/*
 * FUNCIONALIDADE 9 - Atualização de registros (UPDATE) com índice primário
 *
 * Formato de entrada (após "9 arq_dados arq_indice n"):
 *   Para cada uma das n atualizações:
 *     m campo1 val1 ... campoM valM   <- filtros WHERE (m pares)
 *     p campo1 val1 ... campoP valP   <- campos SET    (p pares)
 *
 * BUG CORRIGIDO: ler_registro faz fseek(+4) no prox_queue sem lê-lo,
 * então reg.prox_queue fica 0 após memset. Ao reescrever, gravávamos 0
 * em vez do valor original (-1), corrompendo o checksum do arquivo de dados.
 * SOLUÇÃO: reescrever_registro9 lê o prox_queue original do disco
 * (offset +1 dentro do registro) antes de sobrescrever.
 */

#include "features.h"
#include "registro.h"
#include "leitura.h"
#include "fornecidas.h"
#include <stdlib.h>
#include <string.h>

#define TAM_CABECALHO  17
#define TAM_REGISTRO   80
#define VALOR_NULO     -1

/* =====================================================================
 * Espelho em memória do índice primário
 * ===================================================================== */
typedef struct {
    int cod;
    int rrn;
} IndiceEntry9;

static int comparar_indice9(const void *a, const void *b) {
    const IndiceEntry9 *ia = (const IndiceEntry9 *)a;
    const IndiceEntry9 *ib = (const IndiceEntry9 *)b;
    if (ia->cod != ib->cod) return ia->cod - ib->cod;
    return ia->rrn - ib->rrn;
}

/* =====================================================================
 * Atualiza a chave no índice quando codEstacao muda.
 * Localiza (codAntigo, rrn), substitui pelo codNovo, re-ordena e regrava.
 * ===================================================================== */
static void atualizar_chave_indice9(FILE *fIndice, int codAntigo, int codNovo, int rrn) {
    fseek(fIndice, 0, SEEK_END);
    long tam  = ftell(fIndice);
    int  nReg = (int)((tam - 1) / 8);
    if (nReg <= 0) return;

    IndiceEntry9 *lista = malloc(sizeof(IndiceEntry9) * nReg);
    if (!lista) return;

    fseek(fIndice, 1, SEEK_SET);
    for (int i = 0; i < nReg; i++) {
        fread(&lista[i].cod, sizeof(int), 1, fIndice);
        fread(&lista[i].rrn, sizeof(int), 1, fIndice);
    }

    for (int i = 0; i < nReg; i++) {
        if (lista[i].cod == codAntigo && lista[i].rrn == rrn) {
            lista[i].cod = codNovo;
            break;
        }
    }

    qsort(lista, nReg, sizeof(IndiceEntry9), comparar_indice9);

    fseek(fIndice, 1, SEEK_SET);
    for (int i = 0; i < nReg; i++) {
        fwrite(&lista[i].cod, sizeof(int), 1, fIndice);
        fwrite(&lista[i].rrn, sizeof(int), 1, fIndice);
    }

    free(lista);
}

/* =====================================================================
 * Reescreve um registro no disco (80 bytes fixos).
 *
 * PONTO CRÍTICO: ler_registro pula o prox_queue com fseek(+4) sem lê-lo,
 * então reg->prox_queue = 0 após memset. Aqui lemos o valor ORIGINAL
 * do disco antes de sobrescrever para não corromper esse campo.
 * ===================================================================== */
static void reescrever_registro9(FILE *fDados, reg_dados *reg, int rrn) {
    long offset = TAM_CABECALHO + (long)rrn * TAM_REGISTRO;

    /* Lê o prox_queue original (posição offset+1, após o byte de status) */
    int prox_queue_original;
    fseek(fDados, offset + 1, SEEK_SET);
    fread(&prox_queue_original, sizeof(int), 1, fDados);

    /* Reescreve o registro a partir do início */
    fseek(fDados, offset, SEEK_SET);

    fwrite(&reg->status_removido,  1,           1, fDados);
    fwrite(&prox_queue_original,   sizeof(int), 1, fDados);  /* valor original preservado */
    fwrite(&reg->codEstacao,       sizeof(int), 1, fDados);
    fwrite(&reg->codLinha,         sizeof(int), 1, fDados);
    fwrite(&reg->codProxEstacao,   sizeof(int), 1, fDados);
    fwrite(&reg->distProxEstacao,  sizeof(int), 1, fDados);
    fwrite(&reg->codLinhaIntegra,  sizeof(int), 1, fDados);
    fwrite(&reg->codEstIntegra,    sizeof(int), 1, fDados);

    fwrite(&reg->tamNomeEstacao, sizeof(int), 1, fDados);
    if (reg->tamNomeEstacao > 0 && reg->nomeEstacao)
        fwrite(reg->nomeEstacao, 1, reg->tamNomeEstacao, fDados);

    fwrite(&reg->tamNomeLinha, sizeof(int), 1, fDados);
    if (reg->tamNomeLinha > 0 && reg->nomeLinha)
        fwrite(reg->nomeLinha, 1, reg->tamNomeLinha, fDados);

    /* Padding '$' para completar os 80 bytes fixos */
    int bytesEscritos = 37 + reg->tamNomeEstacao + reg->tamNomeLinha;
    int lixo = TAM_REGISTRO - bytesEscritos;
    char c = '$';
    for (int i = 0; i < lixo; i++)
        fwrite(&c, 1, 1, fDados);
}

/* =====================================================================
 * Verifica se um registro atende a TODOS os filtros WHERE.
 * ===================================================================== */
static int atende_filtros9(reg_dados *reg,
                            int m,
                            char nomesCampos[][50],
                            char valoresStr[][100],
                            int  valoresInt[]) {
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

/* =====================================================================
 * Aplica as modificações SET em um registro carregado na memória.
 * Para strings, verifica se o novo valor cabe nos 80 bytes fixos.
 * ===================================================================== */
static void aplicar_atualizacao9(reg_dados *reg,
                                  int p,
                                  char nomesCamposA[][50],
                                  char valoresStrA[][100],
                                  int  valoresIntA[],
                                  int *mudouCodEstacao,
                                  int *codAntigo) {
    for (int i = 0; i < p; i++) {
        if (strcmp(nomesCamposA[i], "codEstacao") == 0) {
            *mudouCodEstacao = 1;
            *codAntigo       = reg->codEstacao;
            reg->codEstacao  = valoresIntA[i];

        } else if (strcmp(nomesCamposA[i], "codLinha") == 0) {
            reg->codLinha = valoresIntA[i];

        } else if (strcmp(nomesCamposA[i], "codProxEstacao") == 0) {
            reg->codProxEstacao = valoresIntA[i];

        } else if (strcmp(nomesCamposA[i], "distProxEstacao") == 0) {
            reg->distProxEstacao = valoresIntA[i];

        } else if (strcmp(nomesCamposA[i], "codLinhaIntegra") == 0) {
            reg->codLinhaIntegra = valoresIntA[i];

        } else if (strcmp(nomesCamposA[i], "codEstIntegra") == 0) {
            reg->codEstIntegra = valoresIntA[i];

        } else if (strcmp(nomesCamposA[i], "nomeEstacao") == 0) {
            int novoTam = (valoresStrA[i][0] == '\0') ? 0 : (int)strlen(valoresStrA[i]);
            if (37 + novoTam + reg->tamNomeLinha > TAM_REGISTRO) continue; /* não cabe */
            if (reg->nomeEstacao) free(reg->nomeEstacao);
            if (novoTam == 0) {
                reg->nomeEstacao    = NULL;
                reg->tamNomeEstacao = 0;
            } else {
                reg->nomeEstacao    = malloc(novoTam + 1);
                strcpy(reg->nomeEstacao, valoresStrA[i]);
                reg->tamNomeEstacao = novoTam;
            }

        } else if (strcmp(nomesCamposA[i], "nomeLinha") == 0) {
            int novoTam = (valoresStrA[i][0] == '\0') ? 0 : (int)strlen(valoresStrA[i]);
            if (37 + reg->tamNomeEstacao + novoTam > TAM_REGISTRO) continue; /* não cabe */
            if (reg->nomeLinha) free(reg->nomeLinha);
            if (novoTam == 0) {
                reg->nomeLinha    = NULL;
                reg->tamNomeLinha = 0;
            } else {
                reg->nomeLinha    = malloc(novoTam + 1);
                strcpy(reg->nomeLinha, valoresStrA[i]);
                reg->tamNomeLinha = novoTam;
            }
        }
    }
}

/* =====================================================================
 * Lê qtd pares (campo, valor) da entrada padrão.
 * Strings são lidas com ScanQuoteString; inteiros com scanf+atoi.
 * ===================================================================== */
static void ler_clausula9(int qtd,
                           char nomesCampos[][50],
                           char valoresStr[][100],
                           int  valoresInt[]) {
    for (int j = 0; j < qtd; j++) {
        valoresStr[j][0] = '\0';
        valoresInt[j]    = VALOR_NULO;

        scanf("%s", nomesCampos[j]);

        if (strcmp(nomesCampos[j], "nomeEstacao") == 0 ||
            strcmp(nomesCampos[j], "nomeLinha")   == 0) {
            ScanQuoteString(valoresStr[j]);
        } else {
            char temp[50];
            scanf("%s", temp);
            valoresInt[j] = (strcmp(temp, "NULO") == 0) ? VALOR_NULO : atoi(temp);
        }
    }
}

/* =====================================================================
 * FUNCIONALIDADE 9 - ponto de entrada público
 *
 * Chamado em Main.c após:
 *   scanf("%s %s %d", arq_bin, arq_indice, &n);
 *   funcionalidade9(arq_bin, arq_indice, n);
 * ===================================================================== */
void funcionalidade9(const char *arq_dados, const char *arq_indice, int n) {
    FILE *fDados = fopen(arq_dados, "r+b");
    if (!fDados) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    reg_cabecalho cab;
    fseek(fDados, 0, SEEK_SET);
    fread(&cab.status,            sizeof(char), 1, fDados);
    fread(&cab.topo,              sizeof(int),  1, fDados);
    fread(&cab.proxRRN,           sizeof(int),  1, fDados);
    fread(&cab.nroEstacoes,       sizeof(int),  1, fDados);
    fread(&cab.nroParesEstacoes,  sizeof(int),  1, fDados);

    if (cab.status != '1') {
        printf("Falha no processamento do arquivo.\n");
        fclose(fDados);
        return;
    }

    FILE *fIndice = fopen(arq_indice, "r+b");
    if (!fIndice) {
        printf("Falha no processamento do arquivo.\n");
        fclose(fDados);
        return;
    }

    char statusIndice;
    fread(&statusIndice, sizeof(char), 1, fIndice);
    if (statusIndice != '1') {
        printf("Falha no processamento do arquivo.\n");
        fclose(fDados);
        fclose(fIndice);
        return;
    }

    /* Marca ambos como inconsistentes durante a operação */
    char inconsistente = '0';
    fseek(fDados,  0, SEEK_SET); fwrite(&inconsistente, 1, 1, fDados);
    fseek(fIndice, 0, SEEK_SET); fwrite(&inconsistente, 1, 1, fIndice);
    fflush(fDados);
    fflush(fIndice);

    /* ----------------------------------------------------------------
     * Loop principal: processa cada uma das n atualizações
     * ---------------------------------------------------------------- */
    for (int i = 0; i < n; i++) {
        /* Lê filtros WHERE */
        int mFiltros;
        scanf("%d", &mFiltros);
        char nomesCamposB[10][50];
        char valoresStrB[10][100];
        int  valoresIntB[10];
        ler_clausula9(mFiltros, nomesCamposB, valoresStrB, valoresIntB);

        /* Lê campos SET */
        int pCampos;
        scanf("%d", &pCampos);
        char nomesCamposA[10][50];
        char valoresStrA[10][100];
        int  valoresIntA[10];
        ler_clausula9(pCampos, nomesCamposA, valoresStrA, valoresIntA);

        /* Varredura sequencial */
        int rrnAtual = 0;
        fseek(fDados, TAM_CABECALHO, SEEK_SET);

        while (1) {
            reg_dados reg;
            memset(&reg, 0, sizeof(reg_dados));

            int ret = ler_registro(fDados, &reg);
            if (ret == 0) break;   /* EOF */
            if (ret == 2) {        /* removido */
                rrnAtual++;
                continue;
            }

            /* ret == 1: registro válido */
            if (atende_filtros9(&reg, mFiltros, nomesCamposB, valoresStrB, valoresIntB)) {
                int mudouCodEstacao = 0;
                int codAntigo       = reg.codEstacao;

                aplicar_atualizacao9(&reg, pCampos,
                                     nomesCamposA, valoresStrA, valoresIntA,
                                     &mudouCodEstacao, &codAntigo);

                /* Reescreve preservando o prox_queue original do disco */
                reescrever_registro9(fDados, &reg, rrnAtual);

                /* Reposiciona cursor para o próximo registro */
                fseek(fDados, TAM_CABECALHO + (long)(rrnAtual + 1) * TAM_REGISTRO, SEEK_SET);

                /* Atualiza chave no índice se codEstacao mudou */
                if (mudouCodEstacao && codAntigo != VALOR_NULO) {
                    atualizar_chave_indice9(fIndice, codAntigo, reg.codEstacao, rrnAtual);
                    fflush(fIndice);
                }
            }

            if (reg.tamNomeEstacao > 0 && reg.nomeEstacao) free(reg.nomeEstacao);
            if (reg.tamNomeLinha   > 0 && reg.nomeLinha)   free(reg.nomeLinha);

            rrnAtual++;
        }
    }

    /* Restaura consistência */
    char consistente = '1';
    fseek(fDados,  0, SEEK_SET); fwrite(&consistente, 1, 1, fDados);
    fseek(fIndice, 0, SEEK_SET); fwrite(&consistente, 1, 1, fIndice);
    fflush(fDados);
    fflush(fIndice);

    fclose(fDados);
    fclose(fIndice);

    BinarioNaTela((char *)arq_dados);
    BinarioNaTela((char *)arq_indice);
}