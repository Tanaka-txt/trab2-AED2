#include "features.h"
#include "registro.h"
#include "leitura.h"
#include "fornecidas.h"
#include <stdlib.h>
#include <string.h>

// Estrutura para armazenar o índice
typedef struct {
    int cod;
    int rrn;
} IndiceEntry;

// Comparador para o qsort (ordenação crescente pelo código)
// O desempate por RRN garante que a ordenação seja perfeitamente estável
static int compare_cod(const void *a, const void *b) {
    IndiceEntry *entryA = (IndiceEntry *)a;
    IndiceEntry *entryB = (IndiceEntry *)b;
    
    if (entryA->cod != entryB->cod) {
        return entryA->cod - entryB->cod;
    } else {
        return entryA->rrn - entryB->rrn;
    }
}

void funcionalidade5(const char *nome_arq_dados, const char *nome_arq_indice) {
    FILE *dados = fopen(nome_arq_dados, "rb");
    if (dados == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // Lê apenas o status do cabeçalho (ignoramos o proxRRN que pode estar mentindo)
    char status;
    fread(&status, sizeof(char), 1, dados);
    if (status == '0') {
        printf("Falha no processamento do arquivo.\n");
        fclose(dados);
        return;
    }
    
    // Pula o restante do cabeçalho indo direto para o byte 17, onde começam os dados
    fseek(dados, 17, SEEK_SET);

    // Abordagem com alocação dinâmica (expansão sob demanda)
    int capacidade = 20; 
    IndiceEntry *entries = malloc(capacidade * sizeof(IndiceEntry));
    if (entries == NULL) {
        printf("Falha no processamento do arquivo.\n");
        fclose(dados);
        return;
    }

    int count = 0; // Quantidade de registros válidos inseridos no índice
    int rrn = 0;   // RRN físico, conta todos os loops (válidos e removidos)
    reg_dados reg;

    // Lemos livremente até a nossa função de leitura retornar 0 (EOF real)
    while (1) {
        memset(&reg, 0, sizeof(reg_dados)); // Limpa a struct a cada volta para não herdar lixo
        int ret = ler_registro(dados, &reg);
        
        if (ret == 0) {
            break; // Fim do arquivo real atingido!
        }

        if (ret == 1) { // 1 = Registro válido na nossa função
            // Se o vetor lotou, dobramos a capacidade dele (mesma proteção da sua colega, feita de forma segura)
            if (count >= capacidade) {
                capacidade *= 2;
                IndiceEntry *temp = realloc(entries, capacidade * sizeof(IndiceEntry));
                if (temp == NULL) {
                    printf("Falha no processamento do arquivo.\n");
                    free(entries);
                    fclose(dados);
                    return;
                }
                entries = temp;
            }

            // Guarda no índice
            entries[count].cod = reg.codEstacao;
            entries[count].rrn = rrn;
            count++;

            // Libera memória das strings alocadas no ler_registro para evitar memory leak
            if (reg.tamNomeEstacao > 0 && reg.nomeEstacao != NULL) {
                free(reg.nomeEstacao);
            }
            if (reg.tamNomeLinha > 0 && reg.nomeLinha != NULL) {
                free(reg.nomeLinha);
            }
        }
        
        // O RRN avança sempre, independente de ser válido (ret==1) ou removido (ret==2)
        rrn++; 
    }
    fclose(dados);

    // Ordena o vetor usando o nosso qsort blindado
    qsort(entries, count, sizeof(IndiceEntry), compare_cod);

    // Grava o arquivo de índice final
    FILE *indice = fopen(nome_arq_indice, "wb");
    if (indice == NULL) {
        printf("Falha no processamento do arquivo.\n");
        free(entries);
        return;
    }

    char status_indice = '0';
    fwrite(&status_indice, sizeof(char), 1, indice);

    for (int i = 0; i < count; i++) {
        fwrite(&entries[i].cod, sizeof(int), 1, indice);
        fwrite(&entries[i].rrn, sizeof(int), 1, indice);
    }

    // Volta e atualiza status para consistente
    fseek(indice, 0, SEEK_SET);
    status_indice = '1';
    fwrite(&status_indice, sizeof(char), 1, indice);

    fclose(indice);
    free(entries);

    BinarioNaTela((char*)nome_arq_indice);
}