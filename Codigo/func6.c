/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#include "features.h"
#include "registro.h"
#include "leitura.h"
#include "impressao.h"
#include "fornecidas.h"
#include <stdlib.h>
#include <string.h>

// struct para guardar o código de estação e o rrn  do registro correspondente.
typedef struct {
    int cod; // cod estação
    int rrn; // posição relativa do registro no arquivo de dados (RRN)
} IndiceEntry;

// Estrutura que funciona como uma "cláusula WHERE" do SQL.
// Guarda qual campo o usuário quer buscar e o valor esperado.

// struct para guardar o criterio de busca
typedef struct {
    char nomeCampo[50];
    char valorStr[100];
    int valorInt;
    int isNulo; // flag para verificar se o usuario quer buscar por um valor nulo ou não
} CriterioBusca;


// busca binária para encontrar o RRN do código de estação
static int busca_binaria(IndiceEntry *indice, int n, int cod_alvo) {
    int esq = 0, dir = n - 1;
    while (esq <= dir) {
        int meio = esq + (dir - esq) / 2;
        if (indice[meio].cod == cod_alvo) return indice[meio].rrn; // Achou, retorna a posição (RRN)
        else if (indice[meio].cod < cod_alvo) esq = meio + 1;
        else dir = meio - 1;
    }
    return -1; // não achou
}

// verifica se o registro bate com o critério de busca.
static int match_registro(reg_dados *reg, CriterioBusca *criterios, int m) {
    for (int i = 0; i < m; i++) {

        // para cada critério ele verifica o campo correspondete e compara com o valor.
        if (strcmp(criterios[i].nomeCampo, "codEstacao") == 0) {
            // Verifica se é nulo: se o usuário quer um valor nulo, o campo do registro tem que ser -1. Se não for, descarta return 0
            if (criterios[i].isNulo) { if (reg->codEstacao != -1) return 0; }
            // Se não nulo, verifica se o valor corresponde. Se não, descarta
            else if (reg->codEstacao != criterios[i].valorInt) return 0;
            
        } else if (strcmp(criterios[i].nomeCampo, "nomeEstacao") == 0) {
            // checa se ta vazia ou compara
            if (criterios[i].isNulo) { if (reg->tamNomeEstacao > 0) return 0; }
            else if (reg->tamNomeEstacao == 0 || strcmp(reg->nomeEstacao, criterios[i].valorStr) != 0) return 0;
            
        } 

        // esse bloco todo compara cada campo possivel
        else if (strcmp(criterios[i].nomeCampo, "codLinha") == 0) {
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

    // se o registro paddou por tudo de filtro ele retorna 1
    return 1; 
}

// func 6 recebe os criterios de busca.
void funcionalidade6(const char *arq_dados, const char *arq_indice, int n) {
    FILE *fdados = fopen(arq_dados, "rb");
    FILE *findice = fopen(arq_indice, "rb");

    // arquivo aberto?
    if (fdados == NULL || findice == NULL) {
        printf("Falha no processamento do arquivo.\n");
        if (fdados) fclose(fdados);
        if (findice) fclose(findice);
        return;
    }


    // verifica o indice para ver se o arquivo está consistente.
    char status_dados, status_indice;
    fread(&status_dados, sizeof(char), 1, fdados);
    fread(&status_indice, sizeof(char), 1, findice);

    if (status_dados == '0' || status_indice == '0') {
        printf("Falha no processamento do arquivo.\n");
        fclose(fdados);
        fclose(findice);
        return;
    }

    // recebe o indice e calcula quantos registro de indice existem e aloca o vetor de indice inteiro na memória.
    fseek(findice, 0, SEEK_END);
    long tam_indice = ftell(findice);
    
    // esse é o calculo
    int qtd_indice = (tam_indice - 1) / 8; 
    
    IndiceEntry *indice_array = NULL;
    if (qtd_indice > 0) {
        // aloca memoria e carrega o indice inteiro
        indice_array = malloc(qtd_indice * sizeof(IndiceEntry));
        fseek(findice, 1, SEEK_SET); // Pula o cabeçalho
        fread(indice_array, sizeof(IndiceEntry), qtd_indice, findice);
    }

    // loop para procesar as 'n' buscas solicitadas pelo usuário
    for (int i = 0; i < n; i++) {
        int m; // quantidade de critérios para essa busca específica
        if (scanf("%d", &m) != 1) break;

        CriterioBusca criterios[m];
        int tem_codEstacao = 0; // flag da busca rapida
        int cod_buscado = -1;

        // le os criterios informados no terminal
        for (int j = 0; j < m; j++) {
            scanf("%s", criterios[j].nomeCampo);
            ScanQuoteString(criterios[j].valorStr); // func pra ler as strings


            // ve se entrou input nulo
            criterios[j].isNulo = (strlen(criterios[j].valorStr) == 0) ? 1 : 0;
            
            if (!criterios[j].isNulo) {
                criterios[j].valorInt = atoi(criterios[j].valorStr); // converte para inteiro se for string o valorInt não será usado mesmo então não tem problema.
            } else {
                criterios[j].valorInt = -1; // -1 representa nulo
            }

 
            // o if verifica se o campo é codEstacao e se o valor não é nulo porque se for nulo não tem como usar a busca otimizada mesmo
            if (strcmp(criterios[j].nomeCampo, "codEstacao") == 0 && !criterios[j].isNulo) {
                tem_codEstacao = 1;
                cod_buscado = criterios[j].valorInt;
            }
        }

        int encontrou_algum = 0;


        // verifica se o user forneceu o código de estação e se o indice de ram foi carregado corretamente.
        if (tem_codEstacao && indice_array != NULL) {
            // usado para achar o rrn do codigo de estação
            int rrn = busca_binaria(indice_array, qtd_indice, cod_buscado);
            
            if (rrn != -1) {
                // 17 bytes do cabeçalho + (RRN * 80 bytes por registro)
                fseek(fdados, 17 + (rrn * 80), SEEK_SET); 
                
                reg_dados reg;
                memset(&reg, 0, sizeof(reg_dados));
                
                //le o registro específico
                if (ler_registro(fdados, &reg) == 1) { 
                    // checa os filtros
                    if (match_registro(&reg, criterios, m)) {
                        imprimir_registro(&reg);
                        encontrou_algum = 1;
                    }
                    // limpa a memoria por conta dos possiveis vazamentos
                    if (reg.tamNomeEstacao > 0 && reg.nomeEstacao) free(reg.nomeEstacao);
                    if (reg.tamNomeLinha > 0 && reg.nomeLinha) free(reg.nomeLinha);
                }
            }
        } 

        // busca sequencial para buscar o nome ou outros campos sem indice
        else {
            fseek(fdados, 17, SEEK_SET); // vai para o início dos dados
            
            reg_dados reg;
            while (1) { // le o arquivo inteiro de ponta a ponta
                memset(&reg, 0, sizeof(reg_dados));
                int ret = ler_registro(fdados, &reg);
                
                if (ret == 0) break; // fim do arquivo
                
                if (ret == 1) { // registro válido que não seja removido logicamente
                    // verifica o registro atual contra os critérios de busca e imprime se for um match
                    if (match_registro(&reg, criterios, m)) {
                        imprimir_registro(&reg);
                        encontrou_algum = 1; // pode ter varios registros que batem, entao a flag é setada para 1 caso ache pelo menos um registro que bate com os criterios de busca
                    }
                    if (reg.tamNomeEstacao > 0 && reg.nomeEstacao) free(reg.nomeEstacao);
                    if (reg.tamNomeLinha > 0 && reg.nomeLinha) free(reg.nomeLinha);
                }
            }
        }


        // se o final não tiver nenhum registro que bateu, sai um aviso de registro inexistente
        if (!encontrou_algum) {
            printf("Registro inexistente.\n"); 
        }
    }


    // limpeza da memória alocada e fechamento dos arquivos
    if (indice_array) free(indice_array);
    fclose(fdados);
    fclose(findice);
}