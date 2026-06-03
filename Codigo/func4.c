/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#include "features.h"
#include "impressao.h"
#include "leitura.h"
#include "registro.h"

int busca_por_rrn(char *arquivo, int posi_relativa){
    FILE* arq_bin = fopen(arquivo, "rb"); // Abre para leitura do arquivo binário
    if (arq_bin == NULL) { // Verifica se foi possível ler o arquivo binário
        printf("Falha no processamento do arquivo.\n");
        return 0;
    }

    // Utiliza struct do .h
    reg_cabecalho cabecalho;
    reg_dados registro;

    fread(&cabecalho.status, sizeof(char), 1, arq_bin);// le o status

    if(cabecalho.status == '0'){ // verifica a consistencia do arquivo
        printf("Falha no processamento do arquivo.\n");
        fclose(arq_bin);
        return 0;
    }

    // cabecalho.status = '0'; // estamos abrindo ele então temos que mudar seu status (NÃO PRECISA MUDAR?)

    fread(&cabecalho.topo, sizeof(int), 1, arq_bin); // pula o topo

    fread(&cabecalho.proxRRN, sizeof(int), 1, arq_bin); // le o proxRRN

    // Verifica se o RRN desejado é possível
    if(posi_relativa < 0 || posi_relativa >= cabecalho.proxRRN){
        printf("Falha no processamento do arquivo.\n");
        fclose(arq_bin);
        return 0;
    }

    int byteoffset = (posi_relativa * 80) + 17; // pula cabeçalho e inicia no registro pra fazer a leitura;
    fseek(arq_bin, byteoffset, SEEK_SET); // posiciona onde vai começar a leitura

    // Le o binário
    ler_registro(arq_bin, &registro); // faz a leitura do registro em questão

    // Verifica se o registro é apagado
    if(registro.status_removido == '1'){
        printf("falha no processamento do arquivo");
        fclose(arq_bin);
        return 0;
    }

    // Imprime
    imprimir_registro(&registro);

// Libera a memória que foi alocada pelos mallocs dentro do ler_registro
    if (registro.tamNomeEstacao > 0) { 
        free(registro.nomeEstacao); 
    } 
    if (registro.tamNomeLinha > 0) { 
        free(registro.nomeLinha); 
    } 
  
    cabecalho.status = '1'; // Concistente!
    fclose(arq_bin);
}

// Recebemos de entrada um 
// 4 arquivo.bin RRN

// Se retornar 0 retorna "registro inexistente"

// Se retornar 1, seus valores devem ser mostrados de maneira sequêncial com um espaçamento de seus campos,
// campos TAM FIXO com valores -1 não devem retornar -1 devem retornar NULO
// Campos de TAM VARIADO devem exibir NULO

// Ordem exibição: codEstacao, nomeEstacao, codLinha, nomeLinha, codProxEstacao, distProxEstacao, codLinhaInter, nomeLinhaInter, codEstacaoInter.

// Se o registro já for removido exibe "Registro inexistente"
// Qualquer erro deve exibir "falha no processamento do arquivo"