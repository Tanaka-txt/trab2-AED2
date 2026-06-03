/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#include "features.h"
#include "registro.h"

int ler_registro(FILE *binario, reg_dados *registro){ // como 'registro' foi recebido como ponteiro (*), foi usado a seta '->' para acessar os campos na struct original
    // & indica o endereço de memória onde o fread deve salvar o dado lido
    
    if(fread(&registro->status_removido, sizeof(char), 1, binario) == 0){
        return 0; // significa que o arquivo acabou e retorna 0 para quebrar o laço (break) na função principal
    }

    if(registro->status_removido == '1'){ // significa que o registro foi apagado depois de ler o 1º byte e a linha será pulada
        fseek(binario, 79, SEEK_CUR);     // como já leu o 1º pula o restante (79 bytes a partir da posição atual) indo para a próxima linha
        return 2; 
    }
    
    fseek(binario, 4, SEEK_CUR); // pula os 4 bytes do campo "próximo" (RRN do próximo registro removido)

    // leitura campo a campo, na ordem que aparece no arquivo --> 6 inteiros de 4 bytes
    fread(&registro->codEstacao, sizeof(int), 1, binario);
    fread(&registro->codLinha, sizeof(int), 1, binario);
    fread(&registro->codProxEstacao, sizeof(int), 1, binario);
    fread(&registro->distProxEstacao, sizeof(int), 1, binario);
    fread(&registro->codLinhaIntegra, sizeof(int), 1, binario);
    fread(&registro->codEstIntegra, sizeof(int), 1, binario);

    // leitura dos campos de tamanho variável
    fread(&registro->tamNomeEstacao, sizeof(int), 1, binario); // lê primeiro o tamanho da string
        if (registro->tamNomeEstacao > 0) { 
            registro->nomeEstacao = malloc(registro->tamNomeEstacao + 1);                   // malloc aloca memória dinâmica, o +1 garante espaço para o caractere ('\0')
            fread(registro->nomeEstacao, sizeof(char), registro->tamNomeEstacao, binario);  // lê a quantidade de letras especificada, sem ler o lixo do arquivo 
            registro->nomeEstacao[registro->tamNomeEstacao] = '\0';                         // insere o '\0' para que printf e strcmp funcionem na memória, já que o arquivo não armazena esse caractere
        } else {
            registro->nomeEstacao = NULL; // <--- proteção extra contra Segmentation Fault
        }

    fread(&registro->tamNomeLinha, sizeof(int), 1, binario);
        if (registro->tamNomeLinha > 0) { 
            registro->nomeLinha = malloc(registro->tamNomeLinha + 1); 
            fread(registro->nomeLinha, sizeof(char), registro->tamNomeLinha, binario); 
            registro->nomeLinha[registro->tamNomeLinha] = '\0'; 
        } else {
            registro->nomeLinha = NULL; // <--- proteção extra contra Segmentation Fault
        }
    
    // somando apenas os bytes fixos: 1 do removido, 4 do próximo, 24 do bloco fico, 4 do tamNomeEstacao e 4 tamNomeLinha = 37 bytes
    // dos 80 bytes, 37 são fixos e temos a quantidade de letras da estação e linha, o resto é o lixo de memória ($) que vai ser pulado
    int bytesrestantes = 80 - 37 - registro->tamNomeEstacao - registro->tamNomeLinha;
    fseek(binario, bytesrestantes, SEEK_CUR); // fseek(arquivo, tamanho_do_pulo_em_bytes_do_lixo, a_partir_da_posição_atual_do_cursor)
    
    return 1; // retorna 1 se leu com sucesso
}

/*  a seta '->' está no lugar '.' porque a struct não foi criada aqui 
    a fread recebe apenas um ponteiro (*), então a seta acessa a memória da struct que está guardada em outro lugar
    o'&' passa esse endereço exato para o fread
*/

/* 
=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
*/

void le_linha_csv(char *linha, reg_dados *registro, int topo) {
    char *ptr = linha;
    char *temp;

    registro->status_removido = '0'; // status inicial
    registro->prox_queue = topo; // o próximo da fila é o topo atual, que é passado como argumento para a função

    // - CodEstação
    temp = strsep(&ptr, ",");
    if (temp != NULL && strlen(temp) > 0) {
        registro->codEstacao = atoi(temp);
    }

    // - Nome estação
    temp = strsep(&ptr, ",");
    registro->nomeEstacao = malloc(strlen(temp) + 1); 
    temp[strcspn(temp, "\r\n")] = '\0';  // Isso garante que o fgets não bugue com o buffer registrando \n e possíbilitando entradas aleatórias pelo buffer
    strcpy(registro->nomeEstacao, temp); 
    registro->tamNomeEstacao = strlen(temp);

    // - CodLinha
    temp = strsep(&ptr, ",");
    if(temp == NULL || strlen(temp) == 0) {
        registro->codLinha = -1; // se o campo estiver vazio ou for nulo, atribui -1 para indicar que não há próximo
    } else {
        registro->codLinha = atoi(temp);
    }

    // - Nome Linha
    temp = strsep(&ptr, ",");
    registro->nomeLinha = malloc(strlen(temp) + 1); 
    temp[strcspn(temp, "\r\n")] = '\0'; // Isso garante que o fgets não bugue com o buffer registrando \n e possíbilitando entradas aleatórias pelo buffer
    strcpy(registro->nomeLinha, temp); 
    registro->tamNomeLinha = strlen(temp); 

    // - Cod Prox Estac
    temp = strsep(&ptr, ",");
    if(temp == NULL || strlen(temp) == 0) {
        registro->codProxEstacao = -1; // se o campo estiver vazio ou for nulo, atribui -1 para indicar que não há próximo
    } else {
        registro->codProxEstacao = atoi(temp); 
    }

    // - Dist Proxi Estac
    temp = strsep(&ptr, ",");
    if(temp == NULL || strlen(temp) == 0) {
        registro->distProxEstacao = -1; // se o campo estiver vazio ou for nulo, atribui -1 para indicar que não há próximo
    } else {
        registro->distProxEstacao = atoi(temp);
    }

    // - Cod Linha Inte
    temp = strsep(&ptr, ",");
    if (temp == NULL || strlen(temp) == 0) {
        registro->codLinhaIntegra = -1; // se o campo estiver vazio ou for nulo, atribui -1 para indicar que não há próximo
    } else {
        registro->codLinhaIntegra = atoi(temp);
    }

    // - Cod Est Integra
    temp = strsep(&ptr, ",");
    if (temp != NULL) { // fazemos isso porque o campo anterior visto usou fgets ai garantimos a limpeza do buffer
        temp[strcspn(temp, "\r\n")] = '\0';  // Isso garante que o fgets não bugue com o buffer registrando \n e possíbilitando entradas aleatórias pelo buffer
    }
    if (temp == NULL || strlen(temp) == 0) { 
        registro->codEstIntegra = -1; // se o campo estiver vazio ou for nulo, atribui -1 para indicar que não há próximo
    } else {
        registro->codEstIntegra = atoi(temp);
    }
}