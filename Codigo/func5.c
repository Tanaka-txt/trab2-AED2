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

// Estrutura para armazenar um par (codEstacao, RRN) durante a construção do índice
typedef struct {
    int cod;
    int rrn;
} IndiceEntry;

// Função de comparação para ordenar os pares por código crescente
static int compare_cod(const void *a, const void *b) {
    return ((IndiceEntry*)a)->cod - ((IndiceEntry*)b)->cod;
}

void funcionalidade5(const char *nome_arq_dados, const char *nome_arq_indice) {
    // Abre o arquivo de dados para leitura binária
    FILE *dados = fopen(nome_arq_dados, "rb");
    if (dados == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // Leitura do cabeçalho do arquivo de dados
    char status;
    int topo, proxRRN, nroEstacoes, nroParesEstacoes;
    fread(&status, sizeof(char), 1, dados);
    fread(&topo, sizeof(int), 1, dados);
    fread(&proxRRN, sizeof(int), 1, dados);
    fread(&nroEstacoes, sizeof(int), 1, dados);
    fread(&nroParesEstacoes, sizeof(int), 1, dados);

    // Verifica consistência do arquivo de dados
    if (status == '0') {
        printf("Falha no processamento do arquivo.\n");
        fclose(dados);
        return;
    }

    // Posiciona o ponteiro no início do primeiro registro (após os 17 bytes do cabeçalho)
    fseek(dados, 17, SEEK_SET);

    IndiceEntry *entries = NULL;   // Vetor dinâmico de entradas do índice
    int count = 0;                 // Número de registros válidos encontrados
    int rrn = 0;                   // RRN do registro atual (sequencial, mesmo se removido)
    reg_dados reg;

    // Percorre todos os registros do arquivo de dados
    while (1) {
        // Lê o próximo registro (inclui campos fixos e variáveis)
        int ret = ler_registro(dados, &reg);
        if (ret == 0) break;       // Fim do arquivo

        if (ret == 1) {
            // Registro válido (não removido)
            // Expande o vetor dinâmico e armazena o par (codEstacao, RRN)
            IndiceEntry *temp = realloc(entries, (count + 1) * sizeof(IndiceEntry));
            if (temp == NULL) {
                printf("Falha no processamento do arquivo.\n");
                free(entries);
                fclose(dados);
                return;
            }
            entries = temp;
            entries[count].cod = reg.codEstacao;
            entries[count].rrn = rrn;
            count++;
        }

        // Libera a memória alocada para as strings (se houver)
        if (reg.tamNomeEstacao > 0) free(reg.nomeEstacao);
        if (reg.tamNomeLinha > 0) free(reg.nomeLinha);

        rrn++;   // Avança para o próximo RRN (mesmo que o registro esteja removido)
    }
    fclose(dados);

    // Ordena as entradas do índice por código da estação (crescente)
    qsort(entries, count, sizeof(IndiceEntry), compare_cod);

    // Cria o arquivo de índice primário (escrita binária)
    FILE *indice = fopen(nome_arq_indice, "wb+");
    if (indice == NULL) {
        printf("Falha no processamento do arquivo.\n");
        free(entries);
        return;
    }

    // Escreve o cabeçalho do índice com status inconsistente ('0')
    char status_indice = '0';
    fwrite(&status_indice, sizeof(char), 1, indice);

    // Escreve os registros de índice (8 bytes cada: codEstacao + RRN)
    for (int i = 0; i < count; i++) {
        fwrite(&entries[i].cod, sizeof(int), 1, indice);
        fwrite(&entries[i].rrn, sizeof(int), 1, indice);
    }

    // Volta ao início do arquivo de índice e atualiza o status para consistente ('1')
    fseek(indice, 0, SEEK_SET);
    status_indice = '1';
    fwrite(&status_indice, sizeof(char), 1, indice);

    fclose(indice);
    free(entries);

    // Exibe o conteúdo do arquivo de índice usando a função fornecida
    BinarioNaTela((char*)nome_arq_indice);
    BinarioNaTela((char*)"indice_pos_5.bin");
}