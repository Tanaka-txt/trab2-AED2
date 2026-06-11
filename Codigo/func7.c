#include "features.h"
#include "registro.h"
#include "leitura.h"
#include "fornecidas.h"
#include <stdlib.h>
#include <string.h>

// ==================== FUNÇÃO AUXILIAR DE DEPURAÇÃO ====================
static void debug_soma_bytes(const char *nome_arquivo, const char *label) {
    FILE *f = fopen(nome_arquivo, "rb");
    if (!f) {
        printf("[DEBUG] %s: não foi possível abrir %s\n", label, nome_arquivo);
        return;
    }
    fseek(f, 0, SEEK_END);
    long tamanho = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *buf = malloc(tamanho);
    if (!buf) {
        fclose(f);
        return;
    }
    fread(buf, 1, tamanho, f);
    unsigned long soma = 0;
    for (long i = 0; i < tamanho; i++) {
        soma += buf[i];
    }
    free(buf);
    fclose(f);
    printf("[DEBUG] %s: %s -> tamanho=%ld, soma=%.2f\n", 
           label, nome_arquivo, tamanho, soma / 100.0);
}

// ---------- Estrutura e funções para critérios de busca ----------
typedef struct {
    char nomeCampo[50];
    char valorStr[100];
    int valorInt;
    int isNulo;
} CriterioBusca;

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
    return 1;
}

// ---------- Remoção lógica (pilha LIFO) ----------
static int remover_por_rrn(const char *nome_arq_dados, int rrn, int *topo_offset) {
    FILE *f = fopen(nome_arq_dados, "rb+");
    if (!f) return 0;

    int offset_reg = 17 + rrn * 80;
    fseek(f, offset_reg, SEEK_SET);

    char status;
    fread(&status, 1, 1, f);
    if (status == '1') {
        fclose(f);
        return 1;
    }

    int antigo_topo_rrn = (*topo_offset == -1) ? -1 : (*topo_offset - 17) / 80;
    
    // [DEBUG] Log da remoção
    printf("[DEBUG] Removendo RRN %d (offset %d). Topo antigo offset=%d (RRN=%d)\n",
           rrn, offset_reg, *topo_offset, antigo_topo_rrn);

    fseek(f, offset_reg, SEEK_SET);
    char removido = '1';
    fwrite(&removido, 1, 1, f);
    fwrite(&antigo_topo_rrn, sizeof(int), 1, f);

    // Atualiza o topo no cabeçalho
    fseek(f, 0, SEEK_SET);
    char status_cab;
    fread(&status_cab, 1, 1, f);
    int novo_topo_offset = offset_reg;
    fwrite(&novo_topo_offset, sizeof(int), 1, f);

    fclose(f);
    *topo_offset = novo_topo_offset;
    
    // [DEBUG] Log do novo topo
    printf("[DEBUG] Novo topo offset=%d (RRN=%d)\n", *topo_offset, (*topo_offset - 17) / 80);
    
    return 1;
}

// ---------- Reconstrução do índice primário ----------
typedef struct {
    int cod;
    int rrn;
} ParIndice;

static int cmp_par_indice(const void *a, const void *b) {
    const ParIndice *pa = (const ParIndice*)a;
    const ParIndice *pb = (const ParIndice*)b;
    if (pa->cod != pb->cod)
        return pa->cod - pb->cod;
    return pa->rrn - pb->rrn;
}

static void reconstruir_indice(const char *nome_arq_dados, const char *nome_arq_indice) {
    FILE *dados = fopen(nome_arq_dados, "rb");
    if (!dados) return;

    char status;
    fread(&status, 1, 1, dados);
    if (status == '0') {
        fclose(dados);
        return;
    }
    fseek(dados, 17, SEEK_SET);

    int capacidade = 20;
    ParIndice *entries = malloc(capacidade * sizeof(ParIndice));
    if (!entries) {
        fclose(dados);
        return;
    }
    int count = 0, rrn = 0;
    reg_dados reg;

    while (1) {
        memset(&reg, 0, sizeof(reg));
        int ret = ler_registro(dados, &reg);
        if (ret == 0) break;
        if (ret == 1) {
            if (count >= capacidade) {
                capacidade *= 2;
                ParIndice *temp = realloc(entries, capacidade * sizeof(ParIndice));
                if (!temp) {
                    free(entries);
                    fclose(dados);
                    return;
                }
                entries = temp;
            }
            entries[count].cod = reg.codEstacao;
            entries[count].rrn = rrn;
            count++;
            if (reg.tamNomeEstacao > 0) free(reg.nomeEstacao);
            if (reg.tamNomeLinha > 0) free(reg.nomeLinha);
        }
        rrn++;
    }
    fclose(dados);
    
    // [DEBUG] Quantidade de entradas do índice
    printf("[DEBUG] Índice reconstruído: count=%d (registros válidos), último RRN=%d\n", count, rrn);

    qsort(entries, count, sizeof(ParIndice), cmp_par_indice);

    FILE *idx = fopen(nome_arq_indice, "wb");
    if (!idx) {
        free(entries);
        return;
    }
    char status_idx = '1';
    fwrite(&status_idx, 1, 1, idx);
    for (int i = 0; i < count; i++) {
        fwrite(&entries[i].cod, sizeof(int), 1, idx);
        fwrite(&entries[i].rrn, sizeof(int), 1, idx);
    }
    fclose(idx);
    free(entries);
}

// ---------- Funcionalidade 7 (ponto de entrada) ----------
void funcionalidade7(const char *arq_dados, const char *arq_indice, int n) {
    // ========== VALIDAÇÃO INICIAL ==========
    FILE *fdados = fopen(arq_dados, "rb");
    FILE *findice = fopen(arq_indice, "rb");
    if (!fdados || !findice) {
        printf("Falha no processamento do arquivo.\n");
        if (fdados) fclose(fdados);
        if (findice) fclose(findice);
        return;
    }

    char status_dados, status_indice;
    fread(&status_dados, 1, 1, fdados);
    fread(&status_indice, 1, 1, findice);
    if (status_dados == '0' || status_indice == '0') {
        printf("Falha no processamento do arquivo.\n");
        fclose(fdados);
        fclose(findice);
        return;
    }
    fclose(fdados);
    fclose(findice);

    // Leitura do topo atual (byte offset)
    FILE *cab = fopen(arq_dados, "rb");
    if (!cab) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }
    fread(&status_dados, 1, 1, cab);
    int topo_offset;
    fread(&topo_offset, sizeof(int), 1, cab);
    fclose(cab);

    // [DEBUG] Estado inicial dos arquivos
    debug_soma_bytes(arq_dados, "ANTES_DA_REMOCAO");
    debug_soma_bytes(arq_indice, "INDICE_ANTES");
    printf("[DEBUG] Topo inicial offset = %d\n", topo_offset);

    // ========== LOOP DE REMOÇÕES ==========
    for (int i = 0; i < n; i++) {
        int m;
        if (scanf("%d", &m) != 1) break;

        CriterioBusca *criterios = malloc(m * sizeof(CriterioBusca));
        if (!criterios) {
            printf("Falha no processamento do arquivo.\n");
            return;
        }

        // Leitura dos critérios
        for (int j = 0; j < m; j++) {
            scanf("%s", criterios[j].nomeCampo);
            ScanQuoteString(criterios[j].valorStr);
            criterios[j].isNulo = (strlen(criterios[j].valorStr) == 0);
            if (!criterios[j].isNulo)
                criterios[j].valorInt = atoi(criterios[j].valorStr);
            else
                criterios[j].valorInt = -1;
        }

        // [DEBUG] Exibe os critérios lidos
        printf("[DEBUG] Busca %d: m=%d\n", i+1, m);
        for (int j = 0; j < m; j++) {
            printf("  campo='%s', isNulo=%d, valorStr='%s', valorInt=%d\n",
                   criterios[j].nomeCampo, criterios[j].isNulo,
                   criterios[j].valorStr, criterios[j].valorInt);
        }

        // Busca sequencial para encontrar os RRNs dos registros que satisfazem TODOS os critérios
        FILE *dados = fopen(arq_dados, "rb");
        if (!dados) {
            printf("Falha no processamento do arquivo.\n");
            free(criterios);
            return;
        }
        fseek(dados, 17, SEEK_SET);

        int *rrns = NULL;
        int num_remover = 0;

        while (1) {
            long pos = ftell(dados);
            reg_dados reg;
            memset(&reg, 0, sizeof(reg));
            int ret = ler_registro(dados, &reg);
            if (ret == 0) break;
            if (ret == 1) {
                if (match_registro(&reg, criterios, m)) {
                    int rrn = (pos - 17) / 80;
                    rrns = realloc(rrns, (num_remover + 1) * sizeof(int));
                    rrns[num_remover++] = rrn;
                }
                if (reg.tamNomeEstacao > 0) free(reg.nomeEstacao);
                if (reg.tamNomeLinha > 0) free(reg.nomeLinha);
            }
        }
        fclose(dados);

        // [DEBUG] Lista de RRNs que serão removidos
        printf("[DEBUG] Encontrados %d registros para remover: ", num_remover);
        for (int k = 0; k < num_remover; k++) printf("%d ", rrns[k]);
        printf("\n");

        // Remove cada um dos registros encontrados
        for (int k = 0; k < num_remover; k++) {
            if (!remover_por_rrn(arq_dados, rrns[k], &topo_offset)) {
                printf("Falha no processamento do arquivo.\n");
                free(rrns);
                free(criterios);
                return;
            }
        }
        free(rrns);
        free(criterios);

        // [DEBUG] Estado do arquivo de dados após esta rodada de remoções
        debug_soma_bytes(arq_dados, "APOS_REMOCAO_BUSCA");
        printf("[DEBUG] Topo atual offset = %d\n", topo_offset);
    }

    // ========== PÓS-REMOÇÕES ==========
    debug_soma_bytes(arq_dados, "ANTES_RECONSTRUIR_INDICE");

    // Reconstroi o índice (remove entradas correspondentes a registros removidos)
    reconstruir_indice(arq_dados, arq_indice);

    // [DEBUG] Índice após reconstrução
    debug_soma_bytes(arq_indice, "INDICE_RECONSTRUIDO");

    // Estado final dos arquivos
    debug_soma_bytes(arq_dados, "FINAL_DADOS");
    debug_soma_bytes(arq_indice, "FINAL_INDICE");

    // Exibe os dois arquivos atualizados (conforme especificação)
    BinarioNaTela((char*)arq_dados);
    BinarioNaTela((char*)arq_indice);
}