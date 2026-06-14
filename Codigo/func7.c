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

typedef struct {
    int cod;
    int rrn;
} IndiceEntry;

static int compare_cod(const void *a, const void *b) {
    IndiceEntry *ia = (IndiceEntry*)a;
    IndiceEntry *ib = (IndiceEntry*)b;
    if (ia->cod != ib->cod) return ia->cod - ib->cod;
    return ia->rrn - ib->rrn;
}

// Função que verifica se o registro satisfaz TODOS os critérios (AND logic)
static int match_all(reg_dados *reg, char **criterios, int m) {
    for (int i = 0; i < m; i++) {
        char *campo = criterios[i*2];
        char *valor = criterios[i*2+1];
        
        // Se o valor é "NULO", trata como NULL
        int is_null_value = (strcmp(valor, "NULO") == 0);
        
        if (strcmp(campo, "codEstacao") == 0) {
            int val = is_null_value ? -1 : atoi(valor);
            if (reg->codEstacao != val) return 0;
        } else if (strcmp(campo, "codLinha") == 0) {
            int val = is_null_value ? -1 : atoi(valor);
            if (reg->codLinha != val) return 0;
        } else if (strcmp(campo, "codProxEstacao") == 0) {
            int val = is_null_value ? -1 : atoi(valor);
            if (reg->codProxEstacao != val) return 0;
        } else if (strcmp(campo, "distProxEstacao") == 0) {
            int val = is_null_value ? -1 : atoi(valor);
            if (reg->distProxEstacao != val) return 0;
        } else if (strcmp(campo, "codLinhaIntegra") == 0) {
            int val = is_null_value ? -1 : atoi(valor);
            if (reg->codLinhaIntegra != val) return 0;
        } else if (strcmp(campo, "codEstIntegra") == 0) {
            int val = is_null_value ? -1 : atoi(valor);
            if (reg->codEstIntegra != val) return 0;
        } else if (strcmp(campo, "nomeEstacao") == 0) {
            if (is_null_value) {
                if (reg->nomeEstacao != NULL) return 0;
            } else {
                if (reg->nomeEstacao == NULL) return 0;
                if (strcmp(reg->nomeEstacao, valor) != 0) return 0;
            }
        } else if (strcmp(campo, "nomeLinha") == 0) {
            if (is_null_value) {
                if (reg->nomeLinha != NULL) return 0;
            } else {
                if (reg->nomeLinha == NULL) return 0;
                if (strcmp(reg->nomeLinha, valor) != 0) return 0;
            }
        }
    }
    return 1;  // Todos os critérios foram satisfeitos
}

// Remove entrada do índice primário
static void remover_do_indice(FILE *fidx, int codEstacao) {
    fseek(fidx, 0, SEEK_END);
    long tam = ftell(fidx);
    int nReg = (int)((tam - 1) / 8);  // 1 byte header + 8 bytes por registro
    
    if (nReg == 0) return;
    
    // Busca binária para achar a posição
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
    
    if (pos == -1) return;  // não achou
    
    // Desloca registros posteriores uma posição para trás
    for (int i = pos + 1; i < nReg; i++) {
        fseek(fidx, 1 + i * 8, SEEK_SET);
        int cod, rrn;
        fread(&cod, 4, 1, fidx);
        fread(&rrn, 4, 1, fidx);
        
        fseek(fidx, 1 + (i - 1) * 8, SEEK_SET);
        fwrite(&cod, 4, 1, fidx);
        fwrite(&rrn, 4, 1, fidx);
    }
    
    // Trunca o último registro
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

    // Lê cabeçalho original
    reg_cabecalho cab;
    fseek(fdados, 0, SEEK_SET);
    fread(&cab.status, 1, 1, fdados);
    fread(&cab.topo, 4, 1, fdados);
    fread(&cab.proxRRN, 4, 1, fdados);
    fread(&cab.nroEstacoes, 4, 1, fdados);
    fread(&cab.nroParesEstacoes, 4, 1, fdados);
    if (cab.status != '1') {
        printf("Falha no processamento do arquivo.\n");
        fclose(fdados);
        return;
    }
    int total_registros = cab.proxRRN;

    // Marca como inconsistente NO ARQUIVO
    fseek(fdados, 0, SEEK_SET);
    char status_inconsistente = '0';
    fwrite(&status_inconsistente, 1, 1, fdados);

    // Processa cada comando DELETE
    for (int b = 0; b < n; b++) {
        int m;
        scanf("%d", &m);
        
        // Lê os critérios de busca
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

        // Varredura em ordem CRESCENTE
        for (int rrn = 0; rrn < total_registros; rrn++) {
            fseek(fdados, 17 + rrn * 80, SEEK_SET);

            reg_dados reg;
            memset(&reg, 0, sizeof(reg_dados));

            int ret = ler_registro(fdados, &reg);
            if (ret != 1)
                continue;

            // Verifica se satisfaz TODOS os critérios
            int satisfaz = 1;
            for (int j = 0; j < m; j++) {
                if (strcmp(nomesCampos[j], "codEstacao") == 0) {
                    if (reg.codEstacao != valoresInts[j]) { satisfaz = 0; break; }
                } else if (strcmp(nomesCampos[j], "codLinha") == 0) {
                    if (reg.codLinha != valoresInts[j]) { satisfaz = 0; break; }
                } else if (strcmp(nomesCampos[j], "codProxEstacao") == 0) {
                    if (reg.codProxEstacao != valoresInts[j]) { satisfaz = 0; break; }
                } else if (strcmp(nomesCampos[j], "distProxEstacao") == 0) {
                    if (reg.distProxEstacao != valoresInts[j]) { satisfaz = 0; break; }
                } else if (strcmp(nomesCampos[j], "codLinhaIntegra") == 0) {
                    if (reg.codLinhaIntegra != valoresInts[j]) { satisfaz = 0; break; }
                } else if (strcmp(nomesCampos[j], "codEstIntegra") == 0) {
                    if (reg.codEstIntegra != valoresInts[j]) { satisfaz = 0; break; }
                } else if (strcmp(nomesCampos[j], "nomeEstacao") == 0) {
                    if (valoresStrings[j][0] == '\0') {
                        if (reg.nomeEstacao != NULL) { satisfaz = 0; break; }
                    } else {
                        if (reg.nomeEstacao == NULL) { satisfaz = 0; break; }
                        if (strcmp(reg.nomeEstacao, valoresStrings[j]) != 0) { satisfaz = 0; break; }
                    }
                } else if (strcmp(nomesCampos[j], "nomeLinha") == 0) {
                    if (valoresStrings[j][0] == '\0') {
                        if (reg.nomeLinha != NULL) { satisfaz = 0; break; }
                    } else {
                        if (reg.nomeLinha == NULL) { satisfaz = 0; break; }
                        if (strcmp(reg.nomeLinha, valoresStrings[j]) != 0) { satisfaz = 0; break; }
                    }
                }
            }

            if (satisfaz) {
                // Verifica se a estação ainda sobrevive em outro registro ativo
                int estacao_ainda_existe = 0;
                long posAtual = ftell(fdados);
                fseek(fdados, 17, SEEK_SET);
                
                for (int rrn2 = 0; rrn2 < total_registros; rrn2++) {
                    if (rrn2 == rrn) continue;  // Ignora o próprio registro
                    fseek(fdados, 17 + rrn2 * 80, SEEK_SET);
                    
                    reg_dados aux;
                    memset(&aux, 0, sizeof(reg_dados));
                    int ret2 = ler_registro(fdados, &aux);
                    if (ret2 != 1 || aux.nomeEstacao == NULL) {
                        if (aux.tamNomeEstacao > 0 && aux.nomeEstacao) free(aux.nomeEstacao);
                        if (aux.tamNomeLinha > 0 && aux.nomeLinha) free(aux.nomeLinha);
                        continue;
                    }
                    
                    // Se encontra outro registro com mesma estação, ela ainda existe
                    if (reg.nomeEstacao && aux.nomeEstacao && 
                        strcmp(reg.nomeEstacao, aux.nomeEstacao) == 0) {
                        estacao_ainda_existe = 1;
                    }
                    
                    if (aux.tamNomeEstacao > 0 && aux.nomeEstacao) free(aux.nomeEstacao);
                    if (aux.tamNomeLinha > 0 && aux.nomeLinha) free(aux.nomeLinha);
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
                
                // Encadeia na pilha
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

            if (reg.tamNomeEstacao > 0 && reg.nomeEstacao)
                free(reg.nomeEstacao);

            if (reg.tamNomeLinha > 0 && reg.nomeLinha)
                free(reg.nomeLinha);
        }
    }

    // ========== ESCREVE CABEÇALHO FINAL (consistente) ==========
    cab.status = '1';
    fseek(fdados, 0, SEEK_SET);
    fwrite(&cab.status, 1, 1, fdados);
    fwrite(&cab.topo, 4, 1, fdados);
    fwrite(&cab.proxRRN, 4, 1, fdados);
    fwrite(&cab.nroEstacoes, 4, 1, fdados);
    fwrite(&cab.nroParesEstacoes, 4, 1, fdados);
    fclose(fdados);

    // Exibe os dois arquivos
    BinarioNaTela((char*)arq_dados);
    BinarioNaTela((char*)arq_indice);
}