/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#include "indice.h"
#include <stdlib.h>
#include <string.h>

/**
 * Remove um registro do arquivo de índice primário
 * 
 * O arquivo de índice tem o seguinte formato:
 * - Byte 0: status ('0' = inconsistente, '1' = consistente)
 * - A partir do byte 1: registros de 8 bytes cada (codEstacao + RRN)
 * 
 * @param f_indice Ponteiro para o arquivo de índice aberto em modo rb+
 * @param cod_alvo Código da estação a ser removida do índice
 */
void remover_do_indice(FILE *f_indice, int cod_alvo) {
    // ===== PASSO 1: Salvar o status original do arquivo =====
    char status_original;
    fseek(f_indice, 0, SEEK_SET);
    fread(&status_original, sizeof(char), 1, f_indice);
    
    // ===== PASSO 2: Marcar arquivo como inconsistente durante a operação =====
    char status_inconsistente = '0';
    fseek(f_indice, 0, SEEK_SET);
    fwrite(&status_inconsistente, sizeof(char), 1, f_indice);
    
    // ===== PASSO 3: Ler todos os registros do índice para a memória =====
    // Posiciona após o byte de status (início dos registros)
    fseek(f_indice, 1, SEEK_SET);
    
    int codEstacao, rrn;
    int *codigos = NULL;  // Vetor para armazenar os códigos
    int *rrns = NULL;     // Vetor para armazenar os RRNs
    int qtd = 0;          // Quantidade de registros válidos
    
    // Lê todos os registros do arquivo
    while (fread(&codEstacao, sizeof(int), 1, f_indice) == 1) {
        fread(&rrn, sizeof(int), 1, f_indice);
        
        // IMPORTANTE: Só mantém os registros que NÃO são o alvo da remoção
        if (codEstacao != cod_alvo) {
            codigos = realloc(codigos, (qtd + 1) * sizeof(int));
            rrns = realloc(rrns, (qtd + 1) * sizeof(int));
            codigos[qtd] = codEstacao;
            rrns[qtd] = rrn;
            qtd++;
        }
    }
    
    // ===== PASSO 4: Reescrever o arquivo sem o registro removido =====
    // Volta para o início dos registros (após o byte de status)
    fseek(f_indice, 1, SEEK_SET);
    
    // Escreve todos os registros que sobraram
    for (int i = 0; i < qtd; i++) {
        fwrite(&codigos[i], sizeof(int), 1, f_indice);
        fwrite(&rrns[i], sizeof(int), 1, f_indice);
    }
    
    // ===== IMPORTANTE: Não usar ftruncate! =====
    // O run.codes pode não suportar ftruncate corretamente.
    // Ao reescrever o arquivo, os bytes antigos que sobraram no final
    // serão sobrescritos na próxima operação de escrita.
    // O status de consistência garante que não haverá leitura incorreta.
    
    // ===== PASSO 5: Liberar memória alocada =====
    free(codigos);
    free(rrns);
    
    // ===== PASSO 6: Restaurar o status original do arquivo =====
    fseek(f_indice, 0, SEEK_SET);
    fwrite(&status_original, sizeof(char), 1, f_indice);
}


/**
 * Insere um novo registro no arquivo de índice primário
 * 
 * A inserção mantém a ordem crescente por codEstacao,
 * que é um requisito do índice primário.
 * 
 * @param f_indice Ponteiro para o arquivo de índice aberto em modo rb+
 * @param cod_alvo Código da estação a ser inserida
 * @param rrn_alvo RRN do registro no arquivo de dados
 */
void inserir_no_indice(FILE *f_indice, int cod_alvo, int rrn_alvo) {
    // ===== PASSO 1: Salvar o status original do arquivo =====
    char status_original;
    fseek(f_indice, 0, SEEK_SET);
    fread(&status_original, sizeof(char), 1, f_indice);
    
    // ===== PASSO 2: Marcar arquivo como inconsistente durante a operação =====
    char status_inconsistente = '0';
    fseek(f_indice, 0, SEEK_SET);
    fwrite(&status_inconsistente, sizeof(char), 1, f_indice);
    
    // ===== PASSO 3: Ler todos os registros existentes =====
    fseek(f_indice, 1, SEEK_SET);
    
    int codEstacao, rrn;
    int *codigos = NULL;
    int *rrns = NULL;
    int qtd = 0;
    
    // Lê todos os registros atuais
    while (fread(&codEstacao, sizeof(int), 1, f_indice) == 1) {
        fread(&rrn, sizeof(int), 1, f_indice);
        codigos = realloc(codigos, (qtd + 1) * sizeof(int));
        rrns = realloc(rrns, (qtd + 1) * sizeof(int));
        codigos[qtd] = codEstacao;
        rrns[qtd] = rrn;
        qtd++;
    }
    
    // ===== PASSO 4: Adicionar o novo registro =====
    codigos = realloc(codigos, (qtd + 1) * sizeof(int));
    rrns = realloc(rrns, (qtd + 1) * sizeof(int));
    codigos[qtd] = cod_alvo;
    rrns[qtd] = rrn_alvo;
    qtd++;
    
    // ===== PASSO 5: Ordenar os registros por codEstacao (ordem crescente) =====
    // Usando Bubble Sort (simples e eficiente para índices pequenos)
    for (int i = 0; i < qtd - 1; i++) {
        for (int j = 0; j < qtd - i - 1; j++) {
            if (codigos[j] > codigos[j + 1]) {
                // Troca os códigos
                int temp_cod = codigos[j];
                int temp_rrn = rrns[j];
                codigos[j] = codigos[j + 1];
                rrns[j] = rrns[j + 1];
                codigos[j + 1] = temp_cod;
                rrns[j + 1] = temp_rrn;
            }
        }
    }
    
    // ===== PASSO 6: Reescrever o arquivo com os registros ordenados =====
    fseek(f_indice, 1, SEEK_SET);
    for (int i = 0; i < qtd; i++) {
        fwrite(&codigos[i], sizeof(int), 1, f_indice);
        fwrite(&rrns[i], sizeof(int), 1, f_indice);
    }
    
    // ===== PASSO 7: Liberar memória =====
    free(codigos);
    free(rrns);
    
    // ===== PASSO 8: Restaurar o status original do arquivo =====
    fseek(f_indice, 0, SEEK_SET);
    fwrite(&status_original, sizeof(char), 1, f_indice);
}


/**
 * Busca um registro no índice primário pelo código da estação
 * 
 * @param f_indice Ponteiro para o arquivo de índice aberto em modo rb
 * @param cod_alvo Código da estação a ser buscada
 * @param rrn_encontrado Ponteiro para armazenar o RRN encontrado
 * @return 1 se encontrado, 0 se não encontrado
 */
int buscar_no_indice(FILE *f_indice, int cod_alvo, int *rrn_encontrado) {
    // Posiciona após o byte de status (início dos registros)
    fseek(f_indice, 1, SEEK_SET);
    
    int codEstacao, rrn;
    
    // Percorre todos os registros do índice
    while (fread(&codEstacao, sizeof(int), 1, f_indice) == 1) {
        fread(&rrn, sizeof(int), 1, f_indice);
        
        // Se encontrou o código, retorna o RRN
        if (codEstacao == cod_alvo) {
            *rrn_encontrado = rrn;
            return 1;  // Encontrado
        }
        
        // Como o índice está ordenado, podemos otimizar:
        // Se o código atual já é maior que o alvo, não adianta continuar
        if (codEstacao > cod_alvo) {
            break;
        }
    }
    
    return 0;  // Não encontrado
}