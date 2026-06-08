#include "features.h"
#include "registro.h"
#include "leitura.h"
#include "impressao.h"
#include "fornecidas.h"
#include <stdlib.h>
#include <string.h>

// Estrutura para espelhar o arquivo de índice primário na memória
typedef struct {
    int cod;
    int rrn;
} IndiceEntry;

// Estrutura para guardar cada critério da cláusula WHERE
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
        if (indice[meio].cod == cod_alvo) return indice[meio].rrn;
        else if (indice[meio].cod < cod_alvo) esq = meio + 1;
        else dir = meio - 1;
    }
    return -1; // Não encontrou
}

// Verifica se um registro lido bate com TODOS os critérios de busca estipulados
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
    return 1; // Se passou por todos os IFs sem dar 'return 0', então deu Match!
}

void funcionalidade6(const char *arq_dados, const char *arq_indice, int n) {
    FILE *fdados = fopen(arq_dados, "rb");
    FILE *findice = fopen(arq_indice, "rb");

    if (fdados == NULL || findice == NULL) {
        printf("Falha no processamento do arquivo.\n");
        if (fdados) fclose(fdados);
        if (findice) fclose(findice);
        return;
    }

    // Validação de consistência do cabeçalho dos dados
    char status_dados, status_indice;
    fread(&status_dados, sizeof(char), 1, fdados);
    fread(&status_indice, sizeof(char), 1, findice);

    if (status_dados == '0' || status_indice == '0') {
        printf("Falha no processamento do arquivo.\n");
        fclose(fdados);
        fclose(findice);
        return;
    }

    // Carrega o arquivo de índice para a memória
    fseek(findice, 0, SEEK_END);
    long tam_indice = ftell(findice);
    int qtd_indice = (tam_indice - 1) / 8; 
    
    IndiceEntry *indice_array = NULL;
    if (qtd_indice > 0) {
        indice_array = malloc(qtd_indice * sizeof(IndiceEntry));
        fseek(findice, 1, SEEK_SET);
        fread(indice_array, sizeof(IndiceEntry), qtd_indice, findice);
    }

    // Loop de buscas
    for (int i = 0; i < n; i++) {
        int m;
        if (scanf("%d", &m) != 1) break;

        CriterioBusca criterios[m];
        int tem_codEstacao = 0;
        int cod_buscado = -1;

        // Leitura e parse dos parâmetros da busca atual
        for (int j = 0; j < m; j++) {
            scanf("%s", criterios[j].nomeCampo);
            ScanQuoteString(criterios[j].valorStr);

            criterios[j].isNulo = (strlen(criterios[j].valorStr) == 0) ? 1 : 0;
            
            if (!criterios[j].isNulo) {
                criterios[j].valorInt = atoi(criterios[j].valorStr);
            } else {
                criterios[j].valorInt = -1;
            }

            if (strcmp(criterios[j].nomeCampo, "codEstacao") == 0 && !criterios[j].isNulo) {
                tem_codEstacao = 1;
                cod_buscado = criterios[j].valorInt;
            }
        }

        int encontrou_algum = 0;

        // ESTRATÉGIA 1: BUSCA INDEXADA
        if (tem_codEstacao && indice_array != NULL) {
            int rrn = busca_binaria(indice_array, qtd_indice, cod_buscado);
            
            if (rrn != -1) {
                fseek(fdados, 17 + (rrn * 80), SEEK_SET); 
                
                reg_dados reg;
                memset(&reg, 0, sizeof(reg_dados));
                
                if (ler_registro(fdados, &reg) == 1) { 
                    if (match_registro(&reg, criterios, m)) {
                        imprimir_registro(&reg);
                        encontrou_algum = 1;
                    }
                    if (reg.tamNomeEstacao > 0 && reg.nomeEstacao) free(reg.nomeEstacao);
                    if (reg.tamNomeLinha > 0 && reg.nomeLinha) free(reg.nomeLinha);
                }
            }
        } 
        // ESTRATÉGIA 2: BUSCA SEQUENCIAL (Varredura)
        else {
            fseek(fdados, 17, SEEK_SET); 
            
            reg_dados reg;
            while (1) {
                memset(&reg, 0, sizeof(reg_dados));
                int ret = ler_registro(fdados, &reg);
                
                if (ret == 0) break; // Fim do arquivo
                
                if (ret == 1) {
                    if (match_registro(&reg, criterios, m)) {
                        imprimir_registro(&reg);
                        encontrou_algum = 1;
                    }
                    if (reg.tamNomeEstacao > 0 && reg.nomeEstacao) free(reg.nomeEstacao);
                    if (reg.tamNomeLinha > 0 && reg.nomeLinha) free(reg.nomeLinha);
                }
            }
        }

        // Imprime mensagem se a busca 'i' não achar nada
        if (!encontrou_algum) {
            printf("Registro inexistente.\n"); // APENAS 1 QUEBRA DE LINHA
        }
    }

    if (indice_array) free(indice_array);
    fclose(fdados);
    fclose(findice);
}