/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#include "features.h"
#include "leitura.h"
#include "fornecidas.h"
#include "indice.h"
#include <stdlib.h>
#include <string.h>

void write_registro_bin(reg_dados dados, FILE *binario);

void inserir_bin(char *arq_dados, char *arq_indice) {
    FILE *f_dados = fopen(arq_dados, "rb+");
    FILE *f_indice = fopen(arq_indice, "rb+");

    if (!f_dados || !f_indice) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // Lê cabeçalho completo (17 bytes)
    char status;
    int topo_atual, proxRRN, nroEstacoes, nroParesEstacoes;
    
    fread(&status, 1, 1, f_dados);
    fread(&topo_atual, 4, 1, f_dados);
    fread(&proxRRN, 4, 1, f_dados);        
    fread(&nroEstacoes, 4, 1, f_dados);   
    fread(&nroParesEstacoes, 4, 1, f_dados); 
    
    if (status == '0') {
        printf("Falha no processamento do arquivo.\n");
        fclose(f_dados);
        fclose(f_indice);
        return;
    }

    // Marca inconsistente
    status = '0';
    fseek(f_dados, 0, SEEK_SET);
    fwrite(&status, sizeof(char), 1, f_dados);
    fwrite(&topo_atual, sizeof(int), 1, f_dados);
    fwrite(&proxRRN, sizeof(int), 1, f_dados);
    fwrite(&nroEstacoes, sizeof(int), 1, f_dados);
    fwrite(&nroParesEstacoes, sizeof(int), 1, f_dados);

    // Lê número de inserções
    int n;
    scanf("%d", &n);

    int prox_rrn_atual = proxRRN;

    for (int i = 0; i < n; i++) {
        reg_dados novo_reg;
        memset(&novo_reg, 0, sizeof(reg_dados));
        
        char aux[256];
        
        // Leitura dos 8 campos na ordem especificada
        // 1. codEstacao
        scanf("%s", aux);
        novo_reg.codEstacao = (strcmp(aux, "NULO") == 0) ? -1 : atoi(aux);
        
        // 2. nomeEstacao (string com aspas)
        ScanQuoteString(aux);
        if (strcmp(aux, "NULO") != 0 && strlen(aux) > 0) {
            novo_reg.nomeEstacao = strdup(aux);
            novo_reg.tamNomeEstacao = strlen(aux);
        } else {
            novo_reg.nomeEstacao = NULL;
            novo_reg.tamNomeEstacao = 0;
        }
        
        // 3. codLinha
        scanf("%s", aux);
        novo_reg.codLinha = (strcmp(aux, "NULO") == 0) ? -1 : atoi(aux);
        
        // 4. nomeLinha (string com aspas)
        ScanQuoteString(aux);
        if (strcmp(aux, "NULO") != 0 && strlen(aux) > 0) {
            novo_reg.nomeLinha = strdup(aux);
            novo_reg.tamNomeLinha = strlen(aux);
        } else {
            novo_reg.nomeLinha = NULL;
            novo_reg.tamNomeLinha = 0;
        }
        
        // 5. codProxEstacao
        scanf("%s", aux);
        novo_reg.codProxEstacao = (strcmp(aux, "NULO") == 0) ? -1 : atoi(aux);
        
        // 6. distProxEstacao
        scanf("%s", aux);
        novo_reg.distProxEstacao = (strcmp(aux, "NULO") == 0) ? -1 : atoi(aux);
        
        // 7. codLinhaIntegra
        scanf("%s", aux);
        novo_reg.codLinhaIntegra = (strcmp(aux, "NULO") == 0) ? -1 : atoi(aux);
        
        // 8. codEstIntegra
        scanf("%s", aux);
        novo_reg.codEstIntegra = (strcmp(aux, "NULO") == 0) ? -1 : atoi(aux);
        
        // Configura como registro ativo
        novo_reg.status_removido = '0';
        novo_reg.prox_queue = -1;
        
        int rrn_inserido;
        
        // Verifica se há espaço na pilha de removidos
        if (topo_atual != -1) {
            // Reaproveita posição
            rrn_inserido = topo_atual;
            
            // Posiciona no início do registro removido
            fseek(f_dados, 17 + (topo_atual * 80), SEEK_SET);
            
            // Lê o próximo RRN da pilha (encadeamento)
            char removido_lixo;
            fread(&removido_lixo, sizeof(char), 1, f_dados);
            fread(&topo_atual, sizeof(int), 1, f_dados);
            
            // Volta para o início do registro para sobrescrever
            fseek(f_dados, 17 + (rrn_inserido * 80), SEEK_SET);
        } else {
            rrn_inserido = prox_rrn_atual;
            prox_rrn_atual++; 

            fseek(f_dados, 0, SEEK_END);
            // Garante que está na posição correta
            long tamanho = ftell(f_dados);
            if (tamanho < 17 + (rrn_inserido * 80)) {
                fseek(f_dados, 17 + (rrn_inserido * 80), SEEK_SET);
            }
        }
        
        // Escreve o registro no arquivo de dados
        write_registro_bin(novo_reg, f_dados);
        
        // Insere no índice primário
        inserir_no_indice(f_indice, novo_reg.codEstacao, rrn_inserido);
        
        // Atualiza contador de estações
        // nroEstacoes++;
        
        // Libera memória
        if (novo_reg.nomeEstacao) free(novo_reg.nomeEstacao);
        if (novo_reg.nomeLinha) free(novo_reg.nomeLinha);
    }
    
    // Restaura status consistente
    char status_final = '1';
    fseek(f_dados, 0, SEEK_SET);
    fwrite(&status_final, 1, 1, f_dados);
    fwrite(&topo_atual, 4, 1, f_dados);
    fwrite(&prox_rrn_atual, 4, 1, f_dados); 
    fwrite(&nroEstacoes, 4, 1, f_dados);
    fwrite(&nroParesEstacoes, 4, 1, f_dados);
    
    fclose(f_dados);
    fclose(f_indice);
    
    BinarioNaTela(arq_dados);
    BinarioNaTela(arq_indice);
}