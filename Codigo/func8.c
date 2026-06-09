#include "features.h"
#include "registro.h"
#include "leitura.h"
#include "fornecidas.h"
#include <stdlib.h>
#include <string.h>

// Estrutura para espelhar o arquivo de índice primário na memória
typedef struct {
    int cod;
    int rrn;
} IndiceEntry;

// Estrutura para controlar pares únicos
typedef struct {
    int origem;
    int destino;
} Par;

// Função de comparação para o qsort (ordenação crescente pelo código)
static int compare_cod(const void *a, const void *b) {
    IndiceEntry *entryA = (IndiceEntry *)a;
    IndiceEntry *entryB = (IndiceEntry *)b;
    
    if (entryA->cod != entryB->cod) {
        return entryA->cod - entryB->cod;
    } else {
        return entryA->rrn - entryB->rrn;
    }
}

// Função auxiliar para ler um inteiro ou capturar "NULO" (retornando -1)
static int ler_inteiro_ou_nulo() {
    char temp[50];
    scanf("%s", temp);
    if (strcmp(temp, "NULO") == 0) {
        return -1;
    }
    return atoi(temp);
}

// Escreve um novo registro no arquivo de dados na posição (RRN) exata e preenche com lixo '$'
static void escrever_novo_registro(FILE *fdados, int rrn, reg_dados *dados) {
    long offset = 17 + (rrn * 80);
    fseek(fdados, offset, SEEK_SET);

    char status_removido = '0';
    int prox_queue = -1;

    fwrite(&status_removido, sizeof(char), 1, fdados); 
    fwrite(&prox_queue, sizeof(int), 1, fdados);  
    fwrite(&dados->codEstacao, sizeof(int), 1, fdados); 
    fwrite(&dados->codLinha, sizeof(int), 1, fdados); 
    fwrite(&dados->codProxEstacao, sizeof(int), 1, fdados); 
    fwrite(&dados->distProxEstacao, sizeof(int), 1, fdados); 
    fwrite(&dados->codLinhaIntegra, sizeof(int), 1, fdados); 
    fwrite(&dados->codEstIntegra, sizeof(int), 1, fdados); 

    fwrite(&dados->tamNomeEstacao, sizeof(int), 1, fdados); 
    if (dados->tamNomeEstacao > 0) {
        fwrite(dados->nomeEstacao, sizeof(char), dados->tamNomeEstacao, fdados);
    }

    fwrite(&dados->tamNomeLinha, sizeof(int), 1, fdados);  
    if (dados->tamNomeLinha > 0) {
        fwrite(dados->nomeLinha, sizeof(char), dados->tamNomeLinha, fdados); 
    }

    // Calcula o lixo e preenche com '$'
    int tamanho_variados = 37 + dados->tamNomeEstacao + dados->tamNomeLinha;
    int lixo = 80 - tamanho_variados;
    char caracter_lixo = '$';
    for(int i = 0; i < lixo; i++){
        fwrite(&caracter_lixo, sizeof(char), 1, fdados);
    }
}

void funcionalidade8(const char *arq_dados, const char *arq_indice, int n) {
    // Abre para leitura e escrita binária (r+b)
    FILE *fdados = fopen(arq_dados, "r+b");
    FILE *findice = fopen(arq_indice, "rb");

    if (fdados == NULL || findice == NULL) {
        printf("Falha no processamento do arquivo.\n");
        if (fdados) fclose(fdados);
        if (findice) fclose(findice);
        return;
    }

    // Lendo o cabeçalho completo
    char status_dados;
    int topo, proxRRN, nroEstacoes, nroParesEstacoes;
    fread(&status_dados, sizeof(char), 1, fdados);
    fread(&topo, sizeof(int), 1, fdados);
    fread(&proxRRN, sizeof(int), 1, fdados);
    fread(&nroEstacoes, sizeof(int), 1, fdados);
    fread(&nroParesEstacoes, sizeof(int), 1, fdados);

    char status_indice;
    fread(&status_indice, sizeof(char), 1, findice);

    if (status_dados == '0' || status_indice == '0') {
        printf("Falha no processamento do arquivo.\n");
        fclose(fdados);
        fclose(findice);
        return;
    }

    status_dados = '0';
    fseek(fdados, 0, SEEK_SET);
    fwrite(&status_dados, sizeof(char), 1, fdados);

    // ====================================================================
    // CARREGAR ESTAÇÕES E PARES ÚNICOS NA MEMÓRIA
    // ====================================================================
    char **estacoes = NULL;
    int total_estacoes = 0;
    Par *pares = NULL;
    int total_pares = 0;

    fseek(fdados, 17, SEEK_SET);
    reg_dados reg_load;
    for (int i = 0; i < proxRRN; i++) {
        memset(&reg_load, 0, sizeof(reg_dados));
        fseek(fdados, 17 + (i * 80), SEEK_SET);
        int ret = ler_registro(fdados, &reg_load);
        
        if (ret == 1) { // Apenas registros válidos
            // Mapeando nomes de estações únicas
            if (reg_load.tamNomeEstacao > 0 && reg_load.nomeEstacao != NULL) {
                int existe = 0;
                for(int k = 0; k < total_estacoes; k++) {
                    if (strcmp(estacoes[k], reg_load.nomeEstacao) == 0) {
                        existe = 1; break;
                    }
                }
                if (!existe) {
                    estacoes = realloc(estacoes, (total_estacoes + 1) * sizeof(char*));
                    estacoes[total_estacoes] = strdup(reg_load.nomeEstacao);
                    total_estacoes++;
                }
            }

            // Mapeando pares de conexão únicos
            if (reg_load.codProxEstacao != -1) {
                int existe = 0;
                for (int k = 0; k < total_pares; k++) {
                    if (pares[k].origem == reg_load.codEstacao && pares[k].destino == reg_load.codProxEstacao) {
                        existe = 1; break;
                    }
                }
                if (!existe) {
                    pares = realloc(pares, (total_pares + 1) * sizeof(Par));
                    pares[total_pares].origem = reg_load.codEstacao;
                    pares[total_pares].destino = reg_load.codProxEstacao;
                    total_pares++;
                }
            }

            if (reg_load.tamNomeEstacao > 0 && reg_load.nomeEstacao) free(reg_load.nomeEstacao);
            if (reg_load.tamNomeLinha > 0 && reg_load.nomeLinha) free(reg_load.nomeLinha);
        }
    }
    // ====================================================================

    // Carregando índice primário
    fseek(findice, 0, SEEK_END);
    long tam_indice = ftell(findice);
    int qtd_indice = (tam_indice - 1) / 8;
    
    int capacidade_indice = qtd_indice + n;
    IndiceEntry *indice_array = malloc(capacidade_indice * sizeof(IndiceEntry));
    
    if (qtd_indice > 0) {
        fseek(findice, 1, SEEK_SET);
        for (int i = 0; i < qtd_indice; i++) {
            fread(&indice_array[i].cod, sizeof(int), 1, findice);
            fread(&indice_array[i].rrn, sizeof(int), 1, findice);
        }
    }
    fclose(findice);

    // ====================================================================
    // INSERÇÕES SOLICITADAS
    // ====================================================================
    for (int i = 0; i < n; i++) {
        reg_dados reg;
        memset(&reg, 0, sizeof(reg_dados));
        char bufferNomeEstacao[256];
        char bufferNomeLinha[256];

        reg.codEstacao = ler_inteiro_ou_nulo();

        ScanQuoteString(bufferNomeEstacao);
        reg.tamNomeEstacao = strlen(bufferNomeEstacao);
        if (reg.tamNomeEstacao > 0) reg.nomeEstacao = bufferNomeEstacao;
        else reg.nomeEstacao = NULL;

        reg.codLinha = ler_inteiro_ou_nulo();

        ScanQuoteString(bufferNomeLinha);
        reg.tamNomeLinha = strlen(bufferNomeLinha);
        if (reg.tamNomeLinha > 0) reg.nomeLinha = bufferNomeLinha;
        else reg.nomeLinha = NULL;

        reg.codProxEstacao = ler_inteiro_ou_nulo();
        reg.distProxEstacao = ler_inteiro_ou_nulo();
        reg.codLinhaIntegra = ler_inteiro_ou_nulo();
        reg.codEstIntegra = ler_inteiro_ou_nulo();

        // ATUALIZAÇÃO DOS CAMPOS LÓGICOS DO CABEÇALHO (Estações e Pares)
        if (reg.tamNomeEstacao > 0 && reg.nomeEstacao != NULL) {
            int existe = 0;
            for(int k = 0; k < total_estacoes; k++) {
                if (strcmp(estacoes[k], reg.nomeEstacao) == 0) {
                    existe = 1; 
                    break;
                }
            }
            if (!existe) {
                estacoes = realloc(estacoes, (total_estacoes + 1) * sizeof(char*));
                estacoes[total_estacoes] = strdup(reg.nomeEstacao);
                total_estacoes++; // Contabilizando de forma isolada!
            }
        }

        if (reg.codProxEstacao != -1) {
            int existe = 0;
            for (int k = 0; k < total_pares; k++) {
                if (pares[k].origem == reg.codEstacao && pares[k].destino == reg.codProxEstacao) {
                    existe = 1; 
                    break;
                }
            }
            if (!existe) {
                pares = realloc(pares, (total_pares + 1) * sizeof(Par));
                pares[total_pares].origem = reg.codEstacao;
                pares[total_pares].destino = reg.codProxEstacao;
                total_pares++; // Contabilizando de forma isolada!
            }
        }

        int rrn_insercao;
        if (topo != -1) {
            rrn_insercao = topo;
            long offset_prox_queue = 17 + (topo * 80) + 1;
            fseek(fdados, offset_prox_queue, SEEK_SET);
            int proximo_topo;
            fread(&proximo_topo, sizeof(int), 1, fdados);
            topo = proximo_topo; // Desempilhando
        } else {
            rrn_insercao = proxRRN;
            proxRRN++;
        }

        escrever_novo_registro(fdados, rrn_insercao, &reg);

        indice_array[qtd_indice].cod = reg.codEstacao;
        indice_array[qtd_indice].rrn = rrn_insercao;
        qtd_indice++;
    }

    // REGRAVANDO O CABEÇALHO DO ARQUIVO DE DADOS COM TUDO ATUALIZADO
    status_dados = '1';
    nroEstacoes = total_estacoes;
    nroParesEstacoes = total_pares;

    fseek(fdados, 0, SEEK_SET);
    fwrite(&status_dados, sizeof(char), 1, fdados);
    fwrite(&topo, sizeof(int), 1, fdados);
    fwrite(&proxRRN, sizeof(int), 1, fdados);
    fwrite(&nroEstacoes, sizeof(int), 1, fdados);
    fwrite(&nroParesEstacoes, sizeof(int), 1, fdados);
    fclose(fdados);

    // ORDENANDO E REGRAVANDO O ARQUIVO DE ÍNDICE
    qsort(indice_array, qtd_indice, sizeof(IndiceEntry), compare_cod);

    findice = fopen(arq_indice, "wb");
    if (findice != NULL) {
        status_indice = '0';
        fwrite(&status_indice, sizeof(char), 1, findice);
        
        for (int i = 0; i < qtd_indice; i++) {
            fwrite(&indice_array[i].cod, sizeof(int), 1, findice);
            fwrite(&indice_array[i].rrn, sizeof(int), 1, findice);
        }
        
        status_indice = '1';
        fseek(findice, 0, SEEK_SET);
        fwrite(&status_indice, sizeof(char), 1, findice);
        fclose(findice);
    }

    // Limpeza de memória
    free(indice_array);
    if (pares != NULL) free(pares);
    for(int i = 0; i < total_estacoes; i++) {
        free(estacoes[i]);
    }
    if (estacoes != NULL) free(estacoes);

    // Listar ambos usando a função fornecida
    BinarioNaTela((char*)arq_dados);
    BinarioNaTela((char*)arq_indice);
}