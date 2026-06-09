#include "features.h"
#include "registro.h"
#include "leitura.h"
#include "fornecidas.h"
#include <stdlib.h>
#include <string.h>

// Estruturas auxiliares (mesmas usadas na busca)
typedef struct {
    int cod;
    int rrn;
    int removido; // Flag auxiliar: 0 = válido, 1 = removido do índice
} IndiceEntry;

typedef struct {
    char nomeCampo[50];
    char valorStr[100];
    int valorInt;
    int isNulo;
} CriterioBusca;

// Função de Busca Binária no Índice Primário
static int busca_binaria(IndiceEntry *indice, int n, int cod_alvo) {
    int esq = 0, dir = n - 1;
    while (esq <= dir) {
        int meio = esq + (dir - esq) / 2;
        if (indice[meio].cod == cod_alvo) {
            if (indice[meio].removido == 1) return -1; // Já foi removido nesta execução
            return indice[meio].rrn;
        }
        else if (indice[meio].cod < cod_alvo) esq = meio + 1;
        else dir = meio - 1;
    }
    return -1;
}

// Verifica se um registro lido bate com TODOS os critérios
static int match_registro(reg_dados *reg, CriterioBusca *criterios, int m) {
    for (int i = 0; i < m; i++) {
        if (strcmp(criterios[i].nomeCampo, "codEstacao") == 0) {
            if (criterios[i].isNulo) { if (reg->codEstacao != -1) return 0; }
            else if (reg->codEstacao != criterios[i].valorInt) return 0;
            
        } else if (strcmp(criterios[i].nomeCampo, "nomeEstacao") == 0) {
            if (criterios[i].isNulo) { if (reg->tamNomeEstacao > 0) return 0; }
            else if (reg->tamNomeEstacao == 0 || strcmp(reg->nomeEstacao, criterios[i].valorStr) != 0) return 0;
            
        } else if (strcmp(criterios[i].nomeCampo, "codLinha") == 0) {
            if (criterios[i].isNulo) { if (reg->codLinha != -1) return 0; }
            else if (reg->codLinha != criterios[i].valorInt) return 0;
            
        } else if (strcmp(criterios[i].nomeCampo, "nomeLinha") == 0) {
            if (criterios[i].isNulo) { if (reg->tamNomeLinha > 0) return 0; }
            else if (reg->tamNomeLinha == 0 || strcmp(reg->nomeLinha, criterios[i].valorStr) != 0) return 0;
            
        } else if (strcmp(criterios[i].nomeCampo, "codProxEstacao") == 0) {
            if (criterios[i].isNulo) { if (reg->codProxEstacao != -1) return 0; }
            else if (reg->codProxEstacao != criterios[i].valorInt) return 0;
            
        } else if (strcmp(criterios[i].nomeCampo, "distProxEstacao") == 0) {
            if (criterios[i].isNulo) { if (reg->distProxEstacao != -1) return 0; }
            else if (reg->distProxEstacao != criterios[i].valorInt) return 0;
            
        } else if (strcmp(criterios[i].nomeCampo, "codLinhaIntegra") == 0) {
            if (criterios[i].isNulo) { if (reg->codLinhaIntegra != -1) return 0; }
            else if (reg->codLinhaIntegra != criterios[i].valorInt) return 0;
            
        } else if (strcmp(criterios[i].nomeCampo, "codEstIntegra") == 0) {
            if (criterios[i].isNulo) { if (reg->codEstIntegra != -1) return 0; }
            else if (reg->codEstIntegra != criterios[i].valorInt) return 0;
        }
    }
    return 1;
}

// Remove um registro no arquivo de dados e atualiza a pilha
static void remover_registro_fisico(FILE *fdados, int rrn_alvo, int *topo_atual) {
    long offset = 17 + (rrn_alvo * 80);
    fseek(fdados, offset, SEEK_SET);

    char removido = '1';
    fwrite(&removido, sizeof(char), 1, fdados);
    fwrite(topo_atual, sizeof(int), 1, fdados); // O prox_queue passa a apontar pro topo antigo

    *topo_atual = rrn_alvo; // O novo topo passa a ser este RRN
}

// Marca o registro como removido no array de índice na memória
static void remover_do_indice(IndiceEntry *indice, int qtd_indice, int cod_alvo) {
    int esq = 0, dir = qtd_indice - 1;
    while (esq <= dir) {
        int meio = esq + (dir - esq) / 2;
        if (indice[meio].cod == cod_alvo) {
            indice[meio].removido = 1;
            break;
        }
        else if (indice[meio].cod < cod_alvo) esq = meio + 1;
        else dir = meio - 1;
    }
}

void funcionalidade7(const char *arq_dados, const char *arq_indice, int n) {
    // Abre para leitura e escrita (r+b)
    FILE *fdados = fopen(arq_dados, "r+b");
    FILE *findice = fopen(arq_indice, "rb");

    if (fdados == NULL || findice == NULL) {
        printf("Falha no processamento do arquivo.\n");
        if (fdados) fclose(fdados);
        if (findice) fclose(findice);
        return;
    }

    char status_dados, status_indice;
    fread(&status_dados, sizeof(char), 1, fdados);
    fread(&status_indice, sizeof(char), 1, findice);

    if (status_dados == '0' || status_indice == '0') {
        printf("Falha no processamento do arquivo.\n");
        fclose(fdados);
        fclose(findice);
        return;
    }

    // Setando status como inconsistente durante as operações
    status_dados = '0';
    fseek(fdados, 0, SEEK_SET);
    fwrite(&status_dados, sizeof(char), 1, fdados);

    // Lendo o topo atual da pilha de removidos
    int topo_atual;
    fseek(fdados, 1, SEEK_SET);
    fread(&topo_atual, sizeof(int), 1, fdados);

    // Carregando o índice na memória
    fseek(findice, 0, SEEK_END);
    long tam_indice = ftell(findice);
    int qtd_indice = (tam_indice - 1) / 8;
    
    IndiceEntry *indice_array = NULL;
    if (qtd_indice > 0) {
        indice_array = malloc(qtd_indice * sizeof(IndiceEntry));
        fseek(findice, 1, SEEK_SET);
        for (int i = 0; i < qtd_indice; i++) {
            fread(&indice_array[i].cod, sizeof(int), 1, findice);
            fread(&indice_array[i].rrn, sizeof(int), 1, findice);
            indice_array[i].removido = 0;
        }
    }
    fclose(findice); // Fechamos o índice pois vamos regravá-lo do zero no final

    // Pegando o tamanho real do arquivo de dados para varredura
    fseek(fdados, 0, SEEK_END);
    long tamanho_arquivo = ftell(fdados);
    int max_registros = (tamanho_arquivo - 17) / 80;

    for (int i = 0; i < n; i++) {
        int m;
        if (scanf("%d", &m) != 1) break;

        CriterioBusca criterios[m];
        int tem_codEstacao = 0;
        int cod_buscado = -1;

        for (int j = 0; j < m; j++) {
            scanf("%s", criterios[j].nomeCampo);
            ScanQuoteString(criterios[j].valorStr);

            criterios[j].isNulo = (strlen(criterios[j].valorStr) == 0) ? 1 : 0;
            if (!criterios[j].isNulo) criterios[j].valorInt = atoi(criterios[j].valorStr);
            else criterios[j].valorInt = -1;

            if (strcmp(criterios[j].nomeCampo, "codEstacao") == 0 && !criterios[j].isNulo) {
                tem_codEstacao = 1;
                cod_buscado = criterios[j].valorInt;
            }
        }

        // BUSCA E REMOÇÃO INDEXADA
        if (tem_codEstacao && indice_array != NULL) {
            int rrn = busca_binaria(indice_array, qtd_indice, cod_buscado);
            
            if (rrn != -1) {
                fseek(fdados, 17 + (rrn * 80), SEEK_SET);
                reg_dados reg;
                memset(&reg, 0, sizeof(reg_dados));
                
                if (ler_registro(fdados, &reg) == 1) { 
                    if (match_registro(&reg, criterios, m)) {
                        remover_registro_fisico(fdados, rrn, &topo_atual);
                        remover_do_indice(indice_array, qtd_indice, reg.codEstacao);
                    }
                    if (reg.tamNomeEstacao > 0 && reg.nomeEstacao) free(reg.nomeEstacao);
                    if (reg.tamNomeLinha > 0 && reg.nomeLinha) free(reg.nomeLinha);
                }
            }
        } 
        // BUSCA E REMOÇÃO SEQUENCIAL
        else {
            for (int rrn = 0; rrn < max_registros; rrn++) {
                fseek(fdados, 17 + (rrn * 80), SEEK_SET);
                reg_dados reg;
                memset(&reg, 0, sizeof(reg_dados));
                
                if (ler_registro(fdados, &reg) == 1) {
                    if (match_registro(&reg, criterios, m)) {
                        remover_registro_fisico(fdados, rrn, &topo_atual);
                        remover_do_indice(indice_array, qtd_indice, reg.codEstacao);
                    }
                    if (reg.tamNomeEstacao > 0 && reg.nomeEstacao) free(reg.nomeEstacao);
                    if (reg.tamNomeLinha > 0 && reg.nomeLinha) free(reg.nomeLinha);
                }
            }
        }
    }

    // REGRAVANDO O ARQUIVO DE DADOS (Atualiza o topo e o status)
    status_dados = '1';
    fseek(fdados, 0, SEEK_SET);
    fwrite(&status_dados, sizeof(char), 1, fdados);
    fwrite(&topo_atual, sizeof(int), 1, fdados);
    fclose(fdados);

    // REGRAVANDO O ARQUIVO DE ÍNDICE (Apenas com os registros não removidos)
    findice = fopen(arq_indice, "wb");
    if (findice != NULL) {
        status_indice = '0';
        fwrite(&status_indice, sizeof(char), 1, findice);
        
        for (int i = 0; i < qtd_indice; i++) {
            if (indice_array[i].removido == 0) {
                fwrite(&indice_array[i].cod, sizeof(int), 1, findice);
                fwrite(&indice_array[i].rrn, sizeof(int), 1, findice);
            }
        }
        
        status_indice = '1';
        fseek(findice, 0, SEEK_SET);
        fwrite(&status_indice, sizeof(char), 1, findice);
        fclose(findice);
    }

    if (indice_array) free(indice_array);

    BinarioNaTela((char*)arq_dados);
    BinarioNaTela((char*)arq_indice);
}