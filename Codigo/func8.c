/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#include "features.h"
#include "registro.h"
#include "leitura.h"
#include "fornecidas.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int cod;
    int rrn;
} IndiceEntry;

static int compare_cod(const void *a, const void *b) {
    IndiceEntry *ia = (IndiceEntry*)a;
    IndiceEntry *ib = (IndiceEntry*)b;
    if (ia->cod != ib->cod) return ia->cod - ib->cod;
    return ia->rrn - ib->rrn;
}

// Insere um novo registro ordenado no índice
static void inserir_no_indice(FILE *fidx, int codEstacao, int rrn) {
    fseek(fidx, 0, SEEK_END);
    long tam = ftell(fidx);
    int nReg = (int)((tam - 1) / 8);  // 1 byte header + 8 bytes por registro
    
    // Carrega todos os registros do índice
    IndiceEntry *lista = malloc(sizeof(IndiceEntry) * (nReg + 1));
    
    fseek(fidx, 1, SEEK_SET);
    for (int i = 0; i < nReg; i++) {
        fread(&lista[i].cod, 4, 1, fidx);
        fread(&lista[i].rrn, 4, 1, fidx);
    }
    
    // Adiciona novo registro
    lista[nReg].cod = codEstacao;
    lista[nReg].rrn = rrn;
    nReg++;
    
    // Ordena
    qsort(lista, nReg, sizeof(IndiceEntry), compare_cod);
    
    // Reescreve o índice inteiro
    fseek(fidx, 1, SEEK_SET);
    for (int i = 0; i < nReg; i++) {
        fwrite(&lista[i].cod, 4, 1, fidx);
        fwrite(&lista[i].rrn, 4, 1, fidx);
    }
    
    free(lista);
}

// Escreve um registro no arquivo de dados com preenchimento de lixo
static void escrever_registro_com_lixo(FILE *fdados, reg_dados *reg) {
    // Escreve os campos fixos seguindo exatamente o padrão
    fwrite(&reg->status_removido, 1, 1, fdados);
    fwrite(&reg->prox_queue, sizeof(int), 1, fdados);
    fwrite(&reg->codEstacao, sizeof(int), 1, fdados);
    fwrite(&reg->codLinha, sizeof(int), 1, fdados);
    fwrite(&reg->codProxEstacao, sizeof(int), 1, fdados);
    fwrite(&reg->distProxEstacao, sizeof(int), 1, fdados);
    fwrite(&reg->codLinhaIntegra, sizeof(int), 1, fdados);
    fwrite(&reg->codEstIntegra, sizeof(int), 1, fdados);
    
    // Escreve os campos variáveis (strings)
    fwrite(&reg->tamNomeEstacao, sizeof(int), 1, fdados);
    if (reg->tamNomeEstacao > 0)
        fwrite(reg->nomeEstacao, 1, reg->tamNomeEstacao, fdados);
    
    fwrite(&reg->tamNomeLinha, sizeof(int), 1, fdados);
    if (reg->tamNomeLinha > 0)
        fwrite(reg->nomeLinha, 1, reg->tamNomeLinha, fdados);
    
    // Preenche com lixo ($) explicitamente calculando a diferença
    // 37 = status(1) + prox(4) + 6 ints(24) + tamNome(4) + tamLinha(4)
    int bytesEscritos = 37 + reg->tamNomeEstacao + reg->tamNomeLinha;
    int lixo_bytes = 80 - bytesEscritos;
    char lixo = '$';
    for (int i = 0; i < lixo_bytes; i++) {
        fwrite(&lixo, 1, 1, fdados);
    }
}

void funcionalidade8(const char *arq_dados, const char *arq_indice, int n) {
    FILE *fdados = fopen(arq_dados, "r+b");
    if (!fdados) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // Lê e verifica cabeçalho do arquivo de dados
    reg_cabecalho cab;
    fseek(fdados, 0, SEEK_SET);
    fread(&cab.status, 1, 1, fdados);
    fread(&cab.topo, 4, 1, fdados);
    fread(&cab.proxRRN, 4, 1, fdados);
    fread(&cab.nroEstacoes, 4, 1, fdados);
    fread(&cab.nroParesEstacoes, 4, 1, fdados);
    if (cab.status != '1') {
        printf("Falha no processamento do arquivo.\n");
        fclose(fdados);
        return;
    }

    FILE *fidx = fopen(arq_indice, "r+b");
    if (!fidx) {
        printf("Falha no processamento do arquivo.\n");
        fclose(fdados);
        return;
    }

    // Marca os dois arquivos como inconsistentes
    char status_inconsistente = '0';
    fseek(fdados, 0, SEEK_SET);
    fwrite(&status_inconsistente, 1, 1, fdados);
    fseek(fidx, 0, SEEK_SET);
    fwrite(&status_inconsistente, 1, 1, fidx);
    
    fflush(fdados);
    fflush(fidx);

    // Processa cada inserção
    for (int i = 0; i < n; i++) {
        reg_dados reg;
        memset(&reg, 0, sizeof(reg_dados));
        reg.status_removido = '0';
        reg.prox_queue = -1;

        // Lê os campos de entrada
        char temp[100];
        
        // codEstacao
        scanf("%s", temp);
        reg.codEstacao = (strcmp(temp, "NULO") == 0) ? -1 : atoi(temp);

        // nomeEstacao
        ScanQuoteString(temp);
        reg.tamNomeEstacao = strlen(temp);
        if (reg.tamNomeEstacao > 0) {
            reg.nomeEstacao = malloc(reg.tamNomeEstacao + 1);
            strcpy(reg.nomeEstacao, temp);
        } else {
            reg.nomeEstacao = NULL;
        }

        // codLinha
        scanf("%s", temp);
        reg.codLinha = (strcmp(temp, "NULO") == 0) ? -1 : atoi(temp);

        // nomeLinha
        ScanQuoteString(temp);
        reg.tamNomeLinha = strlen(temp);
        if (reg.tamNomeLinha > 0) {
            reg.nomeLinha = malloc(reg.tamNomeLinha + 1);
            strcpy(reg.nomeLinha, temp);
        } else {
            reg.nomeLinha = NULL;
        }

        // codProxEstacao
        scanf("%s", temp);
        reg.codProxEstacao = (strcmp(temp, "NULO") == 0) ? -1 : atoi(temp);

        // distProxEstacao
        scanf("%s", temp);
        reg.distProxEstacao = (strcmp(temp, "NULO") == 0) ? -1 : atoi(temp);

        // codLinhaIntegra
        scanf("%s", temp);
        reg.codLinhaIntegra = (strcmp(temp, "NULO") == 0) ? -1 : atoi(temp);

        // codEstIntegra
        scanf("%s", temp);
        reg.codEstIntegra = (strcmp(temp, "NULO") == 0) ? -1 : atoi(temp);

        // Verifica se é nova estação e novo par
        int nova_estacao = 0;
        int novo_par = 0;

        if (reg.codEstacao != -1 && reg.nomeEstacao != NULL) {
            nova_estacao = 1;
        }

        if (reg.codEstacao != -1 && reg.codProxEstacao != -1) {
            novo_par = 1;
        }

        // Verifica unicidade percorrendo registros ativos e PISANDO nos removidos
        if (nova_estacao || novo_par) {
            fseek(fdados, 17, SEEK_SET);
            reg_dados aux;
            while (1) {
                int ret = ler_registro(fdados, &aux);
                if (ret == 0) break;      // Fim do arquivo (chegou no fim real)
                if (ret == 2) continue;   // É um registro removido! Pula e continua lendo.

                // Verifica unicidade de estação pelo nome
                if (nova_estacao && aux.nomeEstacao != NULL && reg.nomeEstacao != NULL) {
                    if (strcmp(aux.nomeEstacao, reg.nomeEstacao) == 0) {
                        nova_estacao = 0;
                    }
                }

                // Verifica unicidade de par (bidirecional)
                if (novo_par && aux.codEstacao != -1 && aux.codProxEstacao != -1) {
                    if ((aux.codEstacao == reg.codEstacao && aux.codProxEstacao == reg.codProxEstacao) ||
                        (aux.codEstacao == reg.codProxEstacao && aux.codProxEstacao == reg.codEstacao)) {
                        novo_par = 0;
                    }
                }

                // Limpa alocação pendente do ler_registro
                if (aux.tamNomeEstacao > 0 && aux.nomeEstacao)
                    free(aux.nomeEstacao);
                if (aux.tamNomeLinha > 0 && aux.nomeLinha)
                    free(aux.nomeLinha);

                // Se já invalidou as duas checagens, pode parar de procurar!
                if (!nova_estacao && !novo_par) break;
            }
        }

        if (nova_estacao) cab.nroEstacoes++;
        if (novo_par) cab.nroParesEstacoes++;

        // Determina RRN de destino (pilha ou próximo)
        int rrnDestino;
        if (cab.topo != -1) {
            // Pega do topo da pilha e aplica fflush antes de ler
            fflush(fdados);
            rrnDestino = cab.topo;
            long offset = 17 + (long)rrnDestino * 80;
            fseek(fdados, offset + 1, SEEK_SET);
            fread(&cab.topo, 4, 1, fdados);
        } else {
            // Insere no final
            rrnDestino = cab.proxRRN;
            cab.proxRRN++;
        }

        // Escreve o registro no arquivo
        long offsetGrava = 17 + (long)rrnDestino * 80;
        fseek(fdados, offsetGrava, SEEK_SET);
        escrever_registro_com_lixo(fdados, &reg);
        fflush(fdados); // Garante que a gravação do registro seja concluída

        // Insere no índice se o codEstacao não é nulo
        if (reg.codEstacao != -1) {
            inserir_no_indice(fidx, reg.codEstacao, rrnDestino);
            fflush(fidx); // Garante a consistência do índice a cada loop
        }

        // Libera memória das strings
        if (reg.tamNomeEstacao > 0 && reg.nomeEstacao)
            free(reg.nomeEstacao);
        if (reg.tamNomeLinha > 0 && reg.nomeLinha)
            free(reg.nomeLinha);
    }

    // Escreve cabeçalho final consistente
    cab.status = '1';
    fseek(fdados, 0, SEEK_SET);
    fwrite(&cab.status, 1, 1, fdados);
    fwrite(&cab.topo, 4, 1, fdados);
    fwrite(&cab.proxRRN, 4, 1, fdados);
    fwrite(&cab.nroEstacoes, 4, 1, fdados);
    fwrite(&cab.nroParesEstacoes, 4, 1, fdados);
    fflush(fdados);

    // Marca índice como consistente
    char status_consistente = '1';
    fseek(fidx, 0, SEEK_SET);
    fwrite(&status_consistente, 1, 1, fidx);
    fflush(fidx);

    fclose(fdados);
    fclose(fidx);

    // Exibe os dois arquivos
    BinarioNaTela((char*)arq_dados);
    BinarioNaTela((char*)arq_indice);
}