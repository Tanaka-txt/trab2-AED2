/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#include "features.h"
#include "leitura.h"
#include "registro.h"
#include "fornecidas.h"
#include "busca.h"  
#include "indice.h"

int ler_registro(FILE *binario, reg_dados *registro);

void remover_bin(char *arq_dados, char *arq_indice) {
    
    FILE *f_dados = fopen(arq_dados, "rb+"); 
    FILE *f_indice = fopen(arq_indice, "rb+"); 

    if (f_dados == NULL || f_indice == NULL) {
        printf("Falha no processamento do arquivo.\n");
        if (f_dados) fclose(f_dados);
        if (f_indice) fclose(f_indice);
        return;
    }

    // Leitura do cabeçalho completo (17 bytes)
    char status;
    int topo_atual, proxRRN, nroEstacoes, nroParesEstacoes;
    
    fread(&status, sizeof(char), 1, f_dados);
    fread(&topo_atual, sizeof(int), 1, f_dados);
    fread(&proxRRN, sizeof(int), 1, f_dados);
    fread(&nroEstacoes, sizeof(int), 1, f_dados);
    fread(&nroParesEstacoes, sizeof(int), 1, f_dados);
    
    if (status == '0') {
        printf("Falha no processamento do arquivo.\n");
        fclose(f_dados);
        fclose(f_indice);
        return;
    }

    // Marca o arquivo como inconsistente
    status = '0';
    fseek(f_dados, 0, SEEK_SET);
    fwrite(&status, sizeof(char), 1, f_dados);
    fwrite(&topo_atual, sizeof(int), 1, f_dados);
    fwrite(&proxRRN, sizeof(int), 1, f_dados);
    fwrite(&nroEstacoes, sizeof(int), 1, f_dados);
    fwrite(&nroParesEstacoes, sizeof(int), 1, f_dados);

    // Lê o número de remoções
    int n;
    scanf("%d", &n);

    for (int i = 0; i < n; i++) {
        // Lê o número de critérios para esta remoção
        int m;
        scanf("%d", &m);
        
        // Usa a função existente para montar o painel de busca
        Painel_Busca painel = montar_painel(m);
        
        // Varre todos os registros do arquivo
        fseek(f_dados, 17, SEEK_SET);
        int rrn_atual = 0;
        
        while (1) {
            long offset_atual = ftell(f_dados);
            reg_dados registro;
            memset(&registro, 0, sizeof(reg_dados));
            
            int status_leitura = ler_registro(f_dados, &registro);
            if (status_leitura == 0) break;  // Fim do arquivo
            
            if (status_leitura != 2) {  // Registro NÃO removido logicamente
                int atende_todos = 1;
                
                // Verifica cada campo que foi solicitado na busca
                if (painel.busca_codEstacao && registro.codEstacao != painel.valor_codEstacao) 
                    atende_todos = 0;
                    
                if (painel.busca_codLinha && registro.codLinha != painel.valor_codLinha) 
                    atende_todos = 0;
                    
                if (painel.busca_codProxEstacao && registro.codProxEstacao != painel.valor_codProxEstacao) 
                    atende_todos = 0;
                    
                if (painel.busca_distProxEstacao && registro.distProxEstacao != painel.valor_distProxEstacao) 
                    atende_todos = 0;
                    
                if (painel.busca_codLinhaIntegra && registro.codLinhaIntegra != painel.valor_codLinhaIntegra) 
                    atende_todos = 0;
                    
                if (painel.busca_codEstIntegra && registro.codEstIntegra != painel.valor_codEstIntegra) 
                    atende_todos = 0;
                    
                if (painel.busca_nomeEstacao && registro.nomeEstacao) {
                    if (strcmp(registro.nomeEstacao, painel.valor_nomeEstacao) != 0)
                        atende_todos = 0;
                }
                    
                if (painel.busca_nomeLinha && registro.nomeLinha) {
                    if (strcmp(registro.nomeLinha, painel.valor_nomeLinha) != 0)
                        atende_todos = 0;
                }
                
                // Se atende a todos os critérios, remove
                if (atende_todos) {
                    // Volta para o início do registro
                    fseek(f_dados, offset_atual, SEEK_SET);
                    
                    // Marca como removido (status_removido = '1')
                    char removido = '1';
                    fwrite(&removido, sizeof(char), 1, f_dados);
                    
                    // Atualiza o encadeamento da pilha
                    fwrite(&topo_atual, sizeof(int), 1, f_dados);
                    
                    // Atualiza o topo da pilha para este RRN
                    topo_atual = rrn_atual;
                    
                    // Remove a chave do índice primário
                    remover_do_indice(f_indice, registro.codEstacao);
                }
            }
            
            // Libera memória alocada para as strings
            if (registro.nomeEstacao) free(registro.nomeEstacao);
            if (registro.nomeLinha) free(registro.nomeLinha);
            
            rrn_atual++;
            // Posiciona no próximo registro (cada registro tem 80 bytes fixos)
            fseek(f_dados, offset_atual + 80, SEEK_SET);
        }
    }
    
    // Restaura o status do arquivo para consistente
    char status_final = '1';
    fseek(f_dados, 0, SEEK_SET);
    fwrite(&status_final, sizeof(char), 1, f_dados);
    fwrite(&topo_atual, sizeof(int), 1, f_dados);
    fwrite(&proxRRN, sizeof(int), 1, f_dados);
    fwrite(&nroEstacoes, sizeof(int), 1, f_dados);
    fwrite(&nroParesEstacoes, sizeof(int), 1, f_dados);
    
    fclose(f_dados);
    fclose(f_indice);
    
    BinarioNaTela(arq_dados);
    BinarioNaTela(arq_indice);
}