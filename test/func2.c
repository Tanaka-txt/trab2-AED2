/*
- ler linha por linha do arquivo binário, sabendo o tamanho exato de cada pedaço pra não ler informação errada
- comando 2 estacao.bin --> tentar abrir o arquivo no modo de leitura (em C rb é usado para ler binários)
- verificar se o arquivo foi aberto ou não --> mensagem de erro
- pular os 17 bytes de cabeçalho antes de começar a ler as estações (dá pra usar o fseek)
- lendo as linhas (registros) --> cada linha da tabela é um registro que tem 80 bytes
- colocar um laço de repetição pra ler um registro de 80 bytes por vez até o arquivo acabar
- o primeiro byte lido na linha de 80 bytes é o campo que diz se ela foi removida ou não
- se tiver o caractere '1', significa que essa estação foi "apagada" do sistema e ignora o resto da linha, não imprime nada, e pula para os próximos 80 bytes
- se o programa rodar o arquivo e não encontrar nenhuma linha para imprimir, deve exibir a mensagem "Registro inexistente."
*/

#include "features.h"
#include "impressao.h"
#include "leitura.h"

void read_bin(char *arq_bin){
    
    FILE *teste = fopen(arq_bin, "rb");                 // rb --> read binary
    if (teste == NULL) {                                // Verifica se foi possível ler o arquivo binário
        printf("Falha no processamento do arquivo.\n"); // mensagem de erro
        return;
    }

    reg_cabecalho cabecalho;    // Utiliza struct do .h
    reg_dados registro;         // Utiliza struct do .h

    fread(&cabecalho.status, sizeof(char), 1, teste); // primeiro byte do arquivo é o status --> precisamos ler pra saber se o arquivo está consistente

    if(cabecalho.status == '0'){ // verifica a consistencia do arquivo
        printf("Falha no processamento do arquivo.\n");
        fclose(teste);
        return;
    }

    fseek(teste, 17, SEEK_SET); // a partir do início (SEEK_SET) pula 17 bytes que é um tamanho fixo --> movendo para a posição 18º

    int registroValido = 0;  // contator para verificar se um registro válido foi encontrado no arquivo
    int status_leitura;

    // while vai ler cada linha do arquivo, baseando no tamanho fixo de 80 bytes e fread lê o arquivo e o fseek pular79 bytes quando o removido for 1

    while (1){
    
        status_leitura = ler_registro(teste, &registro); // a função ler_registro lê o byte "removido" ('0' ou '1') e os demais dados do registro

        if (status_leitura == 0){ // arquivo acabou --> sai do laço while
                break;
            }
        if (status_leitura == 2){ // registro apagado (tem o '1') --> pula os próximos bytes e volta para o início do laço
                continue;
            }

        registroValido ++; // se a linha não for apagada (tem o '0') lê cada campo dela --> os dados são lidos e tratados um por um

        imprimir_registro(&registro);

        // liberando a memória alocada das strings
        if (registro.tamNomeEstacao > 0) free(registro.nomeEstacao);
        if (registro.tamNomeLinha > 0) free(registro.nomeLinha);
    }
    
    if (registroValido == 0){ // se não tiver nenhum registro válido, ou seja, todas as estações foram removidas
        printf("Registro inexistente.\n"); // exibe a mensagem de erro
    }
    fclose(teste); // fecha o arquivo após terminar a leitura completa
}
