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
#include <unistd.h>
#include "utils.h"

// Remove a entrada do índice primário após a exclusão do registro de dados, localizando a chave e deslocando os próximos registros, sobrescrevendo o valor excluído
static void remover_do_indice(FILE *fidx, int codEstacao) {
    fseek(fidx, 0, SEEK_END);
    long tam = ftell(fidx);
    int nReg = (int)((tam - 1) / 8);  // Cálculo da quantidade de registros
    
    if (nReg == 0) return;
    
    // Busca binária para achar a posição do registro alvo
    int esq = 0, dir = nReg - 1, pos = -1;
    while (esq <= dir) {
        int meio = (esq + dir) / 2;
        fseek(fidx, 1 + meio * 8, SEEK_SET);
        int cod;
        fread(&cod, 4, 1, fidx);
        if (cod == codEstacao) { pos = meio; break; }
        if (cod < codEstacao)  esq = meio + 1;
        else                   dir = meio - 1;
    }
    
    if (pos == -1) return;  // A chave não foi encontrada e interrompe a execução
    
    // Desloca registros localizados uma posição pra trás no arquivo
    for (int i = pos + 1; i < nReg; i++) {
        fseek(fidx, 1 + i * 8, SEEK_SET);
        int cod, rrn;
        fread(&cod, 4, 1, fidx);
        fread(&rrn, 4, 1, fidx);
        
        fseek(fidx, 1 + (i - 1) * 8, SEEK_SET);
        fwrite(&cod, 4, 1, fidx);
        fwrite(&rrn, 4, 1, fidx);
    }
    
    // Trunca o último registro, pra remover o registro que ficou duplicado após o deslocamento
    fflush(fidx);
    long novoTam = 1 + (long)(nReg - 1) * 8;
    ftruncate(fileno(fidx), novoTam);
}

void funcionalidade7(const char *arq_dados, const char *arq_indice, int n) {
    FILE *fdados = fopen(arq_dados, "r+b");
    if (!fdados) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    reg_cabecalho cab; // Lê o cabeçalho pra ver se o arquivo não tá corrompido
    if (!validar_cabecalho(fdados, &cab)) {
        printf("Falha no processamento do arquivo.\n");
        fclose(fdados);
        return;
    }
    int total_registros = cab.proxRRN;

    // Muda os status para 0 (inconsistente) no arquivo, se tiver falha de execução a base de dados vai estar corrompida
    fseek(fdados, 0, SEEK_SET);
    char status_inconsistente = '0';
    fwrite(&status_inconsistente, 1, 1, fdados);

    for (int b = 0; b < n; b++) {
        int m;
        scanf("%d", &m);
        
        // Lê os critérios de exclusão
        char nomesCampos[10][50];
        char valoresStrings[10][100];
        int  valoresInts[10];

        for (int i = 0; i < m; i++) {
            scanf("%s", nomesCampos[i]);
            valoresStrings[i][0] = '\0';
            valoresInts[i] = -1;
            
            // Se é campo string, lê com aspas; se é numérico, lê como número
            if (strcmp(nomesCampos[i], "nomeEstacao") == 0 ||
                strcmp(nomesCampos[i], "nomeLinha") == 0) {
                ScanQuoteString(valoresStrings[i]);
            } else {
                char temp[100];
                scanf("%s", temp);
                if (strcmp(temp, "NULO") == 0) valoresInts[i] = -1;
                else valoresInts[i] = atoi(temp);
            }
        }

        // Varredura em ordem crescente do RRN
        for (int rrn = 0; rrn < total_registros; rrn++) {
            fseek(fdados, 17 + rrn * 80, SEEK_SET); // Posiciona o ponteiro direto no registro utilizando o tamanho do cabeçalho e o RRN

            reg_dados reg;
            memset(&reg, 0, sizeof(reg_dados));

            int ret = ler_registro(fdados, &reg);
            if (ret != 1)
                continue;

            int satisfaz = atende_filtros_geral(&reg, m, nomesCampos, valoresStrings, valoresInts);

            if (satisfaz) {
                // Verifica se a estação ainda existe em outro registro ativo, mantendo a consistência do cabeçalho
                int estacao_ainda_existe = 0;
                long posAtual = ftell(fdados);
                fseek(fdados, 17, SEEK_SET);
                
                for (int rrn2 = 0; rrn2 < total_registros; rrn2++) {
                    if (rrn2 == rrn) continue;  // Ignora o registro alvo da remoção
                    fseek(fdados, 17 + rrn2 * 80, SEEK_SET);
                    
                    reg_dados aux;
                    memset(&aux, 0, sizeof(reg_dados));
                    int ret2 = ler_registro(fdados, &aux);
                    if (ret2 != 1 || aux.nomeEstacao == NULL) {
                        liberar_strings_registro(&aux);
                        continue;
                    }
                    
                    // Se encontra outro registro com mesma estação, ela ainda existe
                    if (reg.nomeEstacao && aux.nomeEstacao && 
                        strcmp(reg.nomeEstacao, aux.nomeEstacao) == 0) {
                        estacao_ainda_existe = 1;
                    }
                    
                    liberar_strings_registro(&aux);
                    if (estacao_ainda_existe) break;
                }
                
                fseek(fdados, posAtual, SEEK_SET);
                
                // Se a estação não existe mais, decrementa o contador
                if (!estacao_ainda_existe && reg.nomeEstacao != NULL) {
                    cab.nroEstacoes--;
                }
                
                // Se há um par válido (não é nulo e diferente da própria estação), decrementa
                if (reg.codProxEstacao != -1 && reg.codEstacao != reg.codProxEstacao) {
                    cab.nroParesEstacoes--;
                }

                // Marca o registro como removido
                fseek(fdados, 17 + rrn * 80, SEEK_SET);
                char rem = '1';
                fwrite(&rem, 1, 1, fdados);
                
                // Encadeia na pilha de reaproveitamento
                int novo_prox = cab.topo;
                fwrite(&novo_prox, 4, 1, fdados);
                
                cab.topo = rrn;
                
                // Remove do índice primário
                FILE *fidx = fopen(arq_indice, "r+b");
                if (fidx) {
                    remover_do_indice(fidx, reg.codEstacao);
                    fclose(fidx);
                }
            }

            // if (reg.tamNomeEstacao > 0 && reg.nomeEstacao)
            //     free(reg.nomeEstacao);

            // if (reg.tamNomeLinha > 0 && reg.nomeLinha)
            //     free(reg.nomeLinha);

            liberar_strings_registro(&reg);
        }
    }

    // O status retorna para 1, ou seja, consistente e os contadores atualziados são gravados
    cab.status = '1';
    fseek(fdados, 0, SEEK_SET);
    fwrite(&cab.status, 1, 1, fdados);
    fwrite(&cab.topo, 4, 1, fdados);
    fwrite(&cab.proxRRN, 4, 1, fdados);
    fwrite(&cab.nroEstacoes, 4, 1, fdados);
    fwrite(&cab.nroParesEstacoes, 4, 1, fdados);
    fclose(fdados);

    BinarioNaTela((char*)arq_dados);
    BinarioNaTela((char*)arq_indice);
}