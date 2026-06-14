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

// Cria o indice primario do arquivo de dados, ordena e grava.
void funcionalidade5(const char *nome_arq_dados, const char *nome_arq_indice) {
    FILE *dados = fopen(nome_arq_dados, "rb");  // abre o arquivo e verifica se foi aberto corretamente
    if (dados == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // le o status do arquivo para ver a consistencia do arquivo de dados.
    char status;
    fread(&status, sizeof(char), 1, dados);
    if (status == '0') { // 0 = inconsistente, 1 = consistente
        printf("Falha no processamento do arquivo.\n");
        fclose(dados);
        return;
    }

    // o ponteiro do arquivo é posivionado no primeiro registro de dados, já pronto pra fazer a leitura.
    fseek(dados, 17, SEEK_SET);

    // aloca um vetor dinâmico para guardar os indices do registros validos.
    int capacidade = 20; 
    IndiceEntry *entries = malloc(capacidade * sizeof(IndiceEntry));
    if (entries == NULL) { // verifica a alocação de memória
        printf("Falha no processamento do arquivo.\n");
        fclose(dados);
        return;
    }

    int count = 0; // contador de registros validos
    int rrn = 0;   // RRN usado para calcular a posição fisica do registro no arq.
    reg_dados reg; // struct para guardar os dados do registro.

    // loop para ler cada regitro do arquivo de dados e continua até chegar no fim do arquivo.
    while (1) {
        // usado para evitar lixo no buffer caso o proximo registro seja menos que o anterior.
        memset(&reg, 0, sizeof(reg_dados)); 
        
        // função externa do leitura.h usada para ler registro do arquivo de dados e preencher o reg.
        int ret = ler_registro(dados, &reg);
        
        if (ret == 0) { // se chegar aqui significa que o arquivo de dados acabou, então sai do loop.
            break;
        }

        // retorna 1 se for refistro valido, para filtrar os registros removidos e só guardar os validos no vetor de indices.
        if (ret == 1) { 
            // aumento da capacidade do vetor para evitar estouro e permitir mais registros.
            if (count >= capacidade) {
                capacidade *= 2; // dobra a capacidade 20 -> 40 -> 80, etc
                
                // realloc para redimensionar o bloco de memória e não apagando oq tem lá
                IndiceEntry *temp = realloc(entries, capacidade * sizeof(IndiceEntry));
                if (temp == NULL) {
                    printf("Falha no processamento do arquivo.\n");
                    free(entries);
                    fclose(dados);
                    return;
                }
                entries = temp;
            }

            // salva o codigo de estação e o rrn no vetor
            entries[count].cod = reg.codEstacao;
            entries[count].rrn = rrn;
            count++; // incrementa o contador de registros validos

            // verifica se a string foi alocada tamanho > 0 e ponteiro não é NULL antes de tentar liberar a memória.
            if (reg.tamNomeEstacao > 0 && reg.nomeEstacao != NULL) {
                free(reg.nomeEstacao);
            }
            if (reg.tamNomeLinha > 0 && reg.nomeLinha != NULL) {
                free(reg.nomeLinha);
            }
        }
        
        // incrementa o rrn para o próximo registro, em cima já é feita a leitura do registro atual, então o rrn é atualizado para o próximo registro.
        rrn++; 
    }
    

    // fecha o arquivo de dados para liberar recursos do sistema e evitar vazamento de memoria
    fclose(dados);

    // ordena o vetor com o qsort e compara com o compare_cod, que ordena pelo código de estação e desempata com o RRN
    qsort(entries, count, sizeof(IndiceEntry), compare_cod);

    // Inicia a criação do novo arquivo de índice em modo de escrita binária ("wb")
    // inicia a criação de um novo arquivo de índice em modo de escrita binária ("wb"), se o arquivo já existir, ele será sobrescrito. Se não existir, ele será criado.
    FILE *indice = fopen(nome_arq_indice, "wb");
    if (indice == NULL) {
        printf("Falha no processamento do arquivo.\n");
        free(entries);
        return;
    }

    // Deixa o arquivo de índice com status '0' (inconsistente) enquanto grava os dados, para garantir que se o programa travar durante a gravação, o arquivo de índice não seja considerado válido.
    char status_indice = '0';
    fwrite(&status_indice, sizeof(char), 1, indice);

    // Varre o nosso vetor na memória que já está ordenado e grava chave e RRN de cada um.
    for (int i = 0; i < count; i++) {
        fwrite(&entries[i].cod, sizeof(int), 1, indice);
        fwrite(&entries[i].rrn, sizeof(int), 1, indice);
    }

    // Volta o ponteiro para o início do arquivo de índice (byte 0)
    fseek(indice, 0, SEEK_SET);
    
    // Agora que a gravação terminou com sucesso, atualiza o status para '1' (consistente)
    status_indice = '1';
    fwrite(&status_indice, sizeof(char), 1, indice);

    // Fecha o arquivo salvo e libera a memória do vetor
    fclose(indice);
    free(entries);

    // printa binario na tela
    BinarioNaTela((char*)nome_arq_indice);
}
