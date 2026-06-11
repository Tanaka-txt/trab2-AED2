/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#include "features.h"
#include "leitura.h"
#include "registro.h"
#include "fornecidas.h"
#include <string.h>
#include <stdlib.h>

// Estrutura auxiliar para pares (origem, destino)
typedef struct {
    int origem;
    int destino;
} Par;

// Verifica se uma estação já existe na lista
static int existe_estacao(const char *nome, char **lista, int tamanho) {
    for (int i = 0; i < tamanho; i++) {
        if (strcmp(nome, lista[i]) == 0)
            return 1;
    }
    return 0;
}

// Verifica se um par (origem, destino) já existe na lista
static int existe_par(int a, int b, Par *lista, int tamanho) {
    for (int i = 0; i < tamanho; i++) {
        if (lista[i].origem == a && lista[i].destino == b)
            return 1;
    }
    return 0;
}

// Escreve um registro de 80 bytes no arquivo binário (formato fixo)
static void write_registro_bin(const reg_dados *dados, FILE *binario) {
    // Tamanho dos campos fixos (exceto strings variáveis)
    const int TAM_FIXO = 37; // 1 (removido) + 4 (prox) + 6*4 (ints) + 4 (tamNomeEstacao) + 4 (tamNomeLinha)
    int tamanho_variavel = dados->tamNomeEstacao + dados->tamNomeLinha;
    int lixo = 80 - (TAM_FIXO + tamanho_variavel);

    fwrite(&dados->status_removido, 1, 1, binario);
    fwrite(&dados->prox_queue, sizeof(int), 1, binario);
    fwrite(&dados->codEstacao, sizeof(int), 1, binario);
    fwrite(&dados->codLinha, sizeof(int), 1, binario);
    fwrite(&dados->codProxEstacao, sizeof(int), 1, binario);
    fwrite(&dados->distProxEstacao, sizeof(int), 1, binario);
    fwrite(&dados->codLinhaIntegra, sizeof(int), 1, binario);
    fwrite(&dados->codEstIntegra, sizeof(int), 1, binario);
    fwrite(&dados->tamNomeEstacao, sizeof(int), 1, binario);
    fwrite(dados->nomeEstacao, 1, dados->tamNomeEstacao, binario);
    fwrite(&dados->tamNomeLinha, sizeof(int), 1, binario);
    fwrite(dados->nomeLinha, 1, dados->tamNomeLinha, binario);

    // Preenche o restante com '$' (lixo)
    for (int i = 0; i < lixo; i++) {
        char lixo_char = '$';
        fwrite(&lixo_char, 1, 1, binario);
    }
}

void create_regi_bin(char arq_csv[256], char arq_bin[256]) {
    FILE *csv = fopen(arq_csv, "r");
    if (!csv) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    FILE *binario = fopen(arq_bin, "wb");  // modo binário para escrita
    if (!binario) {
        printf("Falha no processamento do arquivo.\n");
        fclose(csv);
        return;
    }

    // Cabeçalho inicial (inconsistente)
    reg_cabecalho cabecalho;
    cabecalho.status = '0';
    cabecalho.topo = -1;
    cabecalho.proxRRN = 0;
    cabecalho.nroEstacoes = 0;
    cabecalho.nroParesEstacoes = 0;

    // Escreve o cabeçalho no início do arquivo
    fwrite(&cabecalho.status, 1, 1, binario);
    fwrite(&cabecalho.topo, sizeof(int), 1, binario);
    fwrite(&cabecalho.proxRRN, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroEstacoes, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroParesEstacoes, sizeof(int), 1, binario);

    // Pula a primeira linha do CSV (cabeçalho das colunas)
    char linha[256];
    fgets(linha, sizeof(linha), csv);

    // Listas dinâmicas para controle de estações e pares (sem variáveis globais)
    char **estacoes = NULL;
    int total_estacoes = 0;
    Par *pares = NULL;
    int total_pares = 0;
    int ultimoRRN = 0;   // próximo RRN a ser usado (equivale ao número de registros escritos)

    reg_dados registro;

    while (fgets(linha, sizeof(linha), csv)) {
        // Inicializa a estrutura do registro (importante para ponteiros NULL)
        memset(&registro, 0, sizeof(reg_dados));

        // Faz o parsing da linha CSV. O quarto argumento (topo) é usado para preencher
        // o campo 'prox_queue' (lista de removidos). Como ainda não há removidos, passa -1.
        le_linha_csv(linha, &registro, -1);

        // Atualiza contagem de estações distintas
        if (!existe_estacao(registro.nomeEstacao, estacoes, total_estacoes)) {
            estacoes = realloc(estacoes, (total_estacoes + 1) * sizeof(char*));
            estacoes[total_estacoes] = strdup(registro.nomeEstacao);
            total_estacoes++;
        }
        cabecalho.nroEstacoes = total_estacoes;

        // Atualiza contagem de pares (codEstacao, codProxEstacao) distintos
        if (registro.codProxEstacao != -1 &&
            !existe_par(registro.codEstacao, registro.codProxEstacao, pares, total_pares)) {
            pares = realloc(pares, (total_pares + 1) * sizeof(Par));
            pares[total_pares].origem = registro.codEstacao;
            pares[total_pares].destino = registro.codProxEstacao;
            total_pares++;
        }
        cabecalho.nroParesEstacoes = total_pares;

        // Escreve o registro no arquivo binário
        write_registro_bin(&registro, binario);

        // Libera as strings alocadas por le_linha_csv
        if (registro.nomeEstacao) free(registro.nomeEstacao);
        if (registro.nomeLinha) free(registro.nomeLinha);

        ultimoRRN++;
        cabecalho.proxRRN = ultimoRRN;
    }

    // Finalização: arquivo consistente
    cabecalho.status = '1';
    fseek(binario, 0, SEEK_SET);
    fwrite(&cabecalho.status, 1, 1, binario);
    fwrite(&cabecalho.topo, sizeof(int), 1, binario);
    fwrite(&cabecalho.proxRRN, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroEstacoes, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroParesEstacoes, sizeof(int), 1, binario);

    fclose(csv);
    fclose(binario);

    // Libera memória das listas auxiliares
    for (int i = 0; i < total_estacoes; i++) free(estacoes[i]);
    free(estacoes);
    free(pares);

    // Exibe o conteúdo do arquivo binário (conforme especificação)
    BinarioNaTela(arq_bin);
}