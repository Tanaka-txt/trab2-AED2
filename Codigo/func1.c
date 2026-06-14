/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#include "features.h"
#include "leitura.h"
#include "registro.h"
#include "fornecidas.h"

// Struct de auxilio para verificar a paridade das estações
typedef struct{
    int origem;
    int destino;
} Par;

// Protótipo da função de escrita
void write_registro_bin(reg_dados dados, FILE *binario);

int existe_estacao(char *nome, char **lista, int tamanho){
    for(int i = 0; i < tamanho; i++){
        if(strcmp(nome, lista[i]) == 0)
            return 1;
    }
    return 0;
}

int existe_par(int a, int b, Par *lista, int tamanho){
    for(int i = 0; i < tamanho; i++){
        if(lista[i].origem == a && lista[i].destino == b)
            return 1;
    }
    return 0;
}

void create_regi_bin(char arq_csv[256], char arq_bin[256]){
    // Agora tudo é variável local! Nada de variáveis globais.
    reg_cabecalho cabecalho;
    reg_dados registro;
    int ultimoRRN = 0; 
    char **estacao = NULL;
    int total_estacoes = 0;
    Par *pares = NULL;
    int total_pares = 0;
    
    FILE *csv = fopen(arq_csv, "r");
    FILE *binario = fopen(arq_bin, "wb");

    if(csv == NULL || binario == NULL){
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // Inicializando o cabeçalho na mão (Status '0' indicando que estamos editando)
    cabecalho.status = '0'; 
    cabecalho.topo = -1;
    cabecalho.proxRRN = 0;
    cabecalho.nroEstacoes = 0;
    cabecalho.nroParesEstacoes = 0;

    // Escreve cabeçalho inicial de 17 bytes completos
    fwrite(&cabecalho.status, 1, 1, binario);
    fwrite(&cabecalho.topo, sizeof(int), 1, binario);
    fwrite(&cabecalho.proxRRN, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroEstacoes, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroParesEstacoes, sizeof(int), 1, binario);

    char linha[256];
    fgets(linha, sizeof(linha), csv); // pula cabeçalho CSV

    while(fgets(linha, sizeof(linha), csv)){
        le_linha_csv(linha, &registro, cabecalho.topo);

        // Controle de estações
        if(existe_estacao(registro.nomeEstacao, estacao, total_estacoes) == 0){
            estacao = realloc(estacao, (total_estacoes + 1) * sizeof(char *));
            estacao[total_estacoes] = strdup(registro.nomeEstacao);
            total_estacoes++;
        }
        cabecalho.nroEstacoes = total_estacoes;

        // Controle de pares
        if(registro.codProxEstacao != -1 &&
           existe_par(registro.codEstacao,
                      registro.codProxEstacao,
                      pares,
                      total_pares) == 0){
            pares = realloc(pares, (total_pares + 1) * sizeof(Par));
            pares[total_pares].origem = registro.codEstacao;
            pares[total_pares].destino = registro.codProxEstacao;
            total_pares++;
        }
        cabecalho.nroParesEstacoes = total_pares;

        // Escreve registro
        write_registro_bin(registro, binario);

        free(registro.nomeEstacao);
        registro.nomeEstacao = NULL;
        free(registro.nomeLinha);
        registro.nomeLinha = NULL;

        ultimoRRN++;
    }

    // O loop acabou, vamos atualizar as contagens do cabeçalho
    cabecalho.status = '1'; // Consistente! Tudo deu certo.
    cabecalho.proxRRN = ultimoRRN;
    cabecalho.nroEstacoes = total_estacoes;
    cabecalho.nroParesEstacoes = total_pares;

    // MAGIA DO FSEEK: Volta pro byte zero do arquivo binário e sobrescreve o cabeçalho antigo
    fseek(binario, 0, SEEK_SET);
    fwrite(&cabecalho.status, 1, 1, binario);
    fwrite(&cabecalho.topo, sizeof(int), 1, binario);
    fwrite(&cabecalho.proxRRN, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroEstacoes, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroParesEstacoes, sizeof(int), 1, binario);

    // Agora sim, o arquivo é fechado apenas UMA vez no final do processo
    fclose(csv);
    fclose(binario);

    // Libera pares
    free(pares);
    pares = NULL;

    // Libera estações
    for(int i = 0; i < total_estacoes; i++){
        free(estacao[i]);
    }
    free(estacao);
    estacao = NULL;

    BinarioNaTela(arq_bin);
}

// Definição da função write_registro_bin (fora da create_regi_bin)
void write_registro_bin(reg_dados dados, FILE *binario) {
    int tamanho_variados = 37 + dados.tamNomeEstacao + dados.tamNomeLinha;
    int lixo = 80 - tamanho_variados;
    if (tamanho_variados > 80) {
        printf("Erro: registro maior que 80 bytes\n");
        return;
    }
    fwrite(&dados.status_removido, 1, 1, binario);
    fwrite(&dados.prox_queue, sizeof(int), 1, binario);
    fwrite(&dados.codEstacao, sizeof(int), 1, binario);
    fwrite(&dados.codLinha, sizeof(int), 1, binario);
    fwrite(&dados.codProxEstacao, sizeof(int), 1, binario);
    fwrite(&dados.distProxEstacao, sizeof(int), 1, binario);
    fwrite(&dados.codLinhaIntegra, sizeof(int), 1, binario);
    fwrite(&dados.codEstIntegra, sizeof(int), 1, binario);
    fwrite(&dados.tamNomeEstacao, sizeof(int), 1, binario);
    fwrite(dados.nomeEstacao, 1, dados.tamNomeEstacao, binario);
    fwrite(&dados.tamNomeLinha, sizeof(int), 1, binario);
    fwrite(dados.nomeLinha, 1, dados.tamNomeLinha, binario);
    for (int i = 0; i < lixo; i++) {
        char c = '$';
        fwrite(&c, 1, 1, binario);
    }
}