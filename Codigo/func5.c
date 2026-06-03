/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

// Funcionalidade 5: Criação de índice primário
#include "features.h"
#include "registro.h"
#include "fornecidas.h"

typedef struct {
    int chave;
    long offset;
} EntradaIndice;

// Função de comparação para qsort (ordem crescente pela chave)
static int comparar_chaves(const void *a, const void *b) {
    int ca = ((EntradaIndice*)a)->chave;
    int cb = ((EntradaIndice*)b)->chave;
    return (ca > cb) - (ca < cb);
}

void funcionalidade5(const char *nome_arq_dados, const char *nome_arq_indice) {
    FILE *dados = fopen(nome_arq_dados, "rb");
    if (dados == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // ----- Leitura e validação do cabeçalho -----
    reg_cabecalho cab;
    fread(&cab.status, sizeof(char), 1, dados);
    fread(&cab.topo, sizeof(int), 1, dados);
    fread(&cab.proxRRN, sizeof(int), 1, dados);
    fread(&cab.nroEstacoes, sizeof(int), 1, dados);
    fread(&cab.nroParesEstacoes, sizeof(int), 1, dados);

    if (cab.status != '1') {
        printf("Falha no processamento do arquivo.\n");
        fclose(dados);
        return;
    }

    // ----- Primeira passagem: contar registros ativos -----
    long qtd_ativos = 0;
    fseek(dados, 17, SEEK_SET);  // posiciona no primeiro registro

    for (int rrn = 0; rrn < cab.proxRRN; rrn++) {
        char status_removido;
        fread(&status_removido, sizeof(char), 1, dados);
        if (status_removido == '0') {
            qtd_ativos++;   // registro ativo
        }
        // Pula o restante do registro (79 bytes)
        fseek(dados, 79, SEEK_CUR);
    }

    if (qtd_ativos == 0) {
        // Arquivo sem nenhum registro ativo: cria índice vazio (apenas cabeçalho 0)
        FILE *indice = fopen(nome_arq_indice, "wb");
        if (indice == NULL) {
            printf("Falha no processamento do arquivo.\n");
            fclose(dados);
            return;
        }
        int zero = 0;
        fwrite(&zero, sizeof(int), 1, indice);
        fclose(indice);
        fclose(dados);
        BinarioNaTela((char*)nome_arq_indice);
        return;
    }

    // ----- Aloca vetor para as entradas do índice -----
    EntradaIndice *entradas = (EntradaIndice*) malloc(qtd_ativos * sizeof(EntradaIndice));
    if (entradas == NULL) {
        printf("Falha no processamento do arquivo.\n");
        fclose(dados);
        return;
    }

    // ----- Segunda passagem: coletar chave e offset de cada registro ativo -----
    fseek(dados, 17, SEEK_SET);
    long idx = 0;
    for (int rrn = 0; rrn < cab.proxRRN; rrn++) {
        long offset_registro = 17 + rrn * 80;   // byte offset absoluto
        char status_removido;
        fread(&status_removido, sizeof(char), 1, dados);
        if (status_removido == '0') {
            // Pula o campo prox_queue (4 bytes)
            fseek(dados, 4, SEEK_CUR);
            // Lê a chave primária (codEstacao)
            int chave;
            fread(&chave, sizeof(int), 1, dados);
            // Armazena a entrada
            entradas[idx].chave = chave;
            entradas[idx].offset = offset_registro;
            idx++;
            // Agora precisamos pular o restante do registro para chegar ao próximo.
            // Já lemos: 1 (status) + 4 (prox_queue) + 4 (chave) = 9 bytes.
            // Faltam 80 - 9 = 71 bytes para completar o registro.
            fseek(dados, 71, SEEK_CUR);
        } else {
            // Registro removido: pula os 79 bytes restantes (já lemos 1 byte)
            fseek(dados, 79, SEEK_CUR);
        }
    }

    // ----- Ordena as entradas pela chave -----
    qsort(entradas, qtd_ativos, sizeof(EntradaIndice), comparar_chaves);

    // ----- Escreve o arquivo de índice -----
    FILE *indice = fopen(nome_arq_indice, "wb");
    if (indice == NULL) {
        printf("Falha no processamento do arquivo.\n");
        free(entradas);
        fclose(dados);
        return;
    }

    // Cabeçalho: número de entradas (int)
    int num_entradas = (int) qtd_ativos;
    fwrite(&num_entradas, sizeof(int), 1, indice);

    // Registros de índice: chave (int) + offset (long)
    for (long i = 0; i < qtd_ativos; i++) {
        fwrite(&entradas[i].chave, sizeof(int), 1, indice);
        fwrite(&entradas[i].offset, sizeof(long), 1, indice);
    }

    fclose(indice);
    fclose(dados);
    free(entradas);

    // Exibe o arquivo de índice conforme especificação
    BinarioNaTela((char*)nome_arq_indice);
}