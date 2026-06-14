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
#include "utils.h"

#define TAM_CABECALHO  17
#define TAM_REGISTRO   80
#define VALOR_NULO     -1

// Carrega o índice inteiro, acha o código antigo, troca pelo novo, ordena e salva no arquivo
static void atualizar_chave_indice9(FILE *fIndice, int codAntigo, int codNovo, int rrn) {
    fseek(fIndice, 0, SEEK_END);
    long tam  = ftell(fIndice);
    int  nReg = (int)((tam - 1) / 8);
    if (nReg <= 0) return;

    IndiceEntry *lista = malloc(sizeof(IndiceEntry) * nReg);  // Cria uma lista na memória para caber todos os índices
    if (!lista) return;

    // Pula o status '0' ou '1' e lê os dados do índice
    fseek(fIndice, 1, SEEK_SET);
    for (int i = 0; i < nReg; i++) {
        fread(&lista[i].cod, sizeof(int), 1, fIndice);
        fread(&lista[i].rrn, sizeof(int), 1, fIndice);
    }

    // Procura onde está o código desatualizado e atualiza ele
    for (int i = 0; i < nReg; i++) {
        if (lista[i].cod == codAntigo && lista[i].rrn == rrn) {
            lista[i].cod = codNovo;
            break;
        }
    }

    qsort(lista, nReg, sizeof(IndiceEntry), compare_cod); // Organiza o array para poder manter a buscar binária do índice funcionando

    fseek(fIndice, 1, SEEK_SET); // Volta pro começo do arquivo (pulando o status) e grava a lista atualizada
    for (int i = 0; i < nReg; i++) {
        fwrite(&lista[i].cod, sizeof(int), 1, fIndice);
        fwrite(&lista[i].rrn, sizeof(int), 1, fIndice);
    }

    free(lista); // Liberando a memória para evitar vazamentos
}

// Sobrescreve o registro com os novos dados direto no arquivo de dados
static void reescrever_registro9(FILE *fDados, reg_dados *reg, int rrn) {
    long offset = TAM_CABECALHO + (long)rrn * TAM_REGISTRO;

    // Lê o prox_queue original (posição offset+1, após o byte de status)
    // Pega o valor original do prox_queue direto do arquivo, pra não sobrescrever o arquivo com zero e fazer ele quebrar
    int prox_queue_original;
    fseek(fDados, offset + 1, SEEK_SET);
    fread(&prox_queue_original, sizeof(int), 1, fDados);

    // Volta pro começo do registro para começar gravar os novos dados
    fseek(fDados, offset, SEEK_SET);

    fwrite(&reg->status_removido,  1,           1, fDados);
    fwrite(&prox_queue_original,   sizeof(int), 1, fDados); 
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

    // Padding '$' para completar os 80 bytes fixos
    int bytesEscritos = 37 + reg->tamNomeEstacao + reg->tamNomeLinha;
    int lixo = TAM_REGISTRO - bytesEscritos;
    char c = '$';
    for (int i = 0; i < lixo; i++)
        fwrite(&c, 1, 1, fDados);
}

/* Altera a struct na memória com os novos valores antes de gravar no arquivo. É preciso usar o "free" nas 
strings antigas antes de alocar para as novas com "malloc" para evitar que tenha vazamento de memória.*/
static void aplicar_atualizacao9(reg_dados *reg, int p, char nomesCamposA[][50], char valoresStrA[][100], int  valoresIntA[], int *mudouCodEstacao, int *codAntigo) {
    for (int i = 0; i < p; i++) {
        if (strcmp(nomesCamposA[i], "codEstacao") == 0) {
            *mudouCodEstacao = 1; // Mostra que o arquivo de índice vai precisar ser atualizado depois
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
            
            // A atualização é parada se a soma dos campos fixos (37 bytes) com as novas strings passarem do limite de 80 bytes do registro
            if (37 + novoTam + reg->tamNomeLinha > TAM_REGISTRO) continue; 
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
            
            if (37 + reg->tamNomeEstacao + novoTam > TAM_REGISTRO) continue; 
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

// Função de apoio que vai ler as informações digitadas pelo usuário no terminal. Valores nulos são convergidos a -1
static void ler_clausula9(int qtd, char nomesCampos[][50], char valoresStr[][100], int  valoresInt[]) {
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

void funcionalidade9(const char *arq_dados, const char *arq_indice, int n) {
    FILE *fDados = fopen(arq_dados, "r+b");
    if (!fDados) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    reg_cabecalho cab; // Lê o cabeçalho pra ver se o arquivo não tá corrompido
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

    // Muda os status para 0 (inconsistente), pois se o programa travar durante o UPDATE, dá pra saber que o arquivo de dados ficou corrompido
    char inconsistente = '0';
    fseek(fDados,  0, SEEK_SET); fwrite(&inconsistente, 1, 1, fDados);
    fseek(fIndice, 0, SEEK_SET); fwrite(&inconsistente, 1, 1, fIndice);
    fflush(fDados);
    fflush(fIndice);

    // Começa a ler as n atualizações que foram pedidas
    for (int i = 0; i < n; i++) {
        int mFiltros; // Lê filtros WHERE --> o que tem que buscar
        scanf("%d", &mFiltros);
        char nomesCamposB[10][50];
        char valoresStrB[10][100];
        int  valoresIntB[10];
        ler_clausula9(mFiltros, nomesCamposB, valoresStrB, valoresIntB);

        int pCampos; // Lê campos SET --> novos valores que vão ser colocados no lugar
        scanf("%d", &pCampos);
        char nomesCamposA[10][50];
        char valoresStrA[10][100];
        int  valoresIntA[10];
        ler_clausula9(pCampos, nomesCamposA, valoresStrA, valoresIntA);

        int rrnAtual = 0;
        fseek(fDados, TAM_CABECALHO, SEEK_SET);

        // Passa pelos registros do arquivo para procurar quem atende aos critérios
        while (1) {
            reg_dados reg;
            memset(&reg, 0, sizeof(reg_dados));

            int ret = ler_registro(fDados, &reg);
            if (ret == 0) break;   // EOF
            if (ret == 2) {        // removido
                rrnAtual++;
                continue;
            }

            // Registro válido, testa se ele vai bater com a busca
            if (atende_filtros_geral(&reg, mFiltros, nomesCamposB, valoresStrB, valoresIntB)) {
                int mudouCodEstacao = 0;
                int codAntigo       = reg.codEstacao;

                aplicar_atualizacao9(&reg, pCampos, nomesCamposA, valoresStrA, valoresIntA, &mudouCodEstacao, &codAntigo);
                reescrever_registro9(fDados, &reg, rrnAtual);
                fseek(fDados, TAM_CABECALHO + (long)(rrnAtual + 1) * TAM_REGISTRO, SEEK_SET); // Coloca o ponteiro de leitura no próximo registro

                // Atualiza a função se o índice do codEstacao mudou
                if (mudouCodEstacao && codAntigo != VALOR_NULO) {
                    atualizar_chave_indice9(fIndice, codAntigo, reg.codEstacao, rrnAtual);
                    fflush(fIndice);
                }
            }

            // Libera memória das strings
            if (reg.tamNomeEstacao > 0 && reg.nomeEstacao) free(reg.nomeEstacao);
            if (reg.tamNomeLinha   > 0 && reg.nomeLinha)   free(reg.nomeLinha);

            rrnAtual++;
        }
    }

    char consistente = '1'; // Restaura a consistência
    fseek(fDados,  0, SEEK_SET); fwrite(&consistente, 1, 1, fDados);
    fseek(fIndice, 0, SEEK_SET); fwrite(&consistente, 1, 1, fIndice);
    fflush(fDados);
    fflush(fIndice);

    fclose(fDados);
    fclose(fIndice);

    BinarioNaTela((char *)arq_dados);
    BinarioNaTela((char *)arq_indice);
}