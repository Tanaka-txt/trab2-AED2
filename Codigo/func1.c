/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#include "features.h"
#include "leitura.h"
#include "registro.h"
#include "fornecidas.h"

// Definição de Var
int ultimoRRN = 0; // tem que apontar para uma atualização do RRN

// Nro estacoes
char **estacao = NULL; // é um ponteiro para ponteiro que inicializa como vazio (vetor de ponteiros, cada item é um malloc)
int total_estacoes = 0;

// Cria Cabeçalho
reg_cabecalho cabecalho;
reg_dados registro;


// Struct de auxilio para verificar a pariedade das estações
typedef struct{
    int origem;
    int destino;
}Par;

// Pares Estações
Par *pares = NULL; // é um ponteiro que inicializa como vazio
int total_pares = 0;

// chama protótipo de func
void write_registro_bin(reg_dados dados, FILE *binario); // Chamando ele aqui para usar no read csv

// Inicio Funções
void create_cabecalho(){
    cabecalho.status = '1';
    cabecalho.topo = -1;
    cabecalho.proxRRN = 0; // Comeã em 0, 0+1 = 1
    cabecalho.nroEstacoes = 0;
    cabecalho.nroParesEstacoes = 0;
}

// Funções para registrar no cabeçalho
//                 RegEstacoes   Estacoes   totalEstacoes
int existe_estacao(char *nome, char **lista, int tamanho){ // função de busca!
    for(int i = 0; i < tamanho; i++){  // Faz um loop com o total de estacoes
        if(strcmp(nome, lista[i]) == 0) // Verifica o nome da estação atual com o a lista de nome de estações, se não houver terona 1
            return 1; // volta para a função que chamou
    }
    return 0; // volta para a função que chamou
}

//Pares Estações
int existe_par(int a, int b, Par *lista, int tamanho){ // recebe A como origem e B como destino, recebe a lista que é do tipo da struct de apoio e o tamanho d
  for(int i = 0; i < tamanho; i++){ // fazemos loop com o total de 
    if(lista[i].origem == a && lista[i].destino == b) // verifica se ja temos um representante da estação na origem e de um destino 
        return 1; // se sim (retorna 1 e assim sabemos que são pares)
    }
    return 0; // se não (retorna 0 e vai para a função que o chamou)
}

void create_regi_bin(char arq_csv[256], char arq_bin[256]){
    // =-=-= Abrimos o Arquivo CSV e Binário=-=-=
    FILE *csv = fopen(arq_csv, "r"); 
    FILE *binario = fopen(arq_bin, "wb+"); 

    if(csv == NULL || binario == NULL){  // Verificação se foi possível abrir os arquivos
        printf("Falha no processamento do arquivo.");
        return;
    }

    // =-=-= Cabeçalho =-=-= 
    create_cabecalho(); // Cria cabeçalho

    // Consistencia do cabeçalho (0 pois vamos escrever nele)
    cabecalho.status = '0'; // agora inconsistente

    char linha[256]; // um buffer pro tamanho da linha, uma linha tem 255 caracteres + \0

    fseek(binario, 0, SEEK_SET);  // Posiciona no inicio do arquivo binário, depois escrevemos nele
    fwrite(&cabecalho.status, 1, 1, binario);
    fwrite(&cabecalho.topo, sizeof(int), 1, binario);
    fwrite(&cabecalho.proxRRN, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroEstacoes, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroParesEstacoes, sizeof(int), 1, binario);

    fseek(binario, 17, SEEK_SET); // Posiciona o "ponteiro" para a próxima escrita no ultimo byte do cabeçalho para o próximo ser a parte do registro

    fgets(linha, 256, csv); // Pula o cabeçalho do csv (aquela parte que tem a categoria das tabelas)

    // Loop de leitura do CSV
    while (fgets(linha, 256, csv)) { 
        
        // Chama função le_linha_csv, para cada vez no loop fazer toda a leitura e registrar no registro e passa o 
        le_linha_csv(linha, &registro, cabecalho.topo);

        // Verifica nroEstacoes !existe_estacao(registro.nomeEstacao, estacao, total_estacoes)
        if(existe_estacao(registro.nomeEstacao, estacao, total_estacoes) == 0 ){ // Verifica primeiro se caso não existir na lista ele coloca na lista
            estacao = realloc(estacao, (total_estacoes + 1) * sizeof(char*)); // realocamos memório com o realloc, na estacao o espaço para o total de estacoes mais 1 que é o novo
            estacao[total_estacoes] = strdup(registro.nomeEstacao); // strdup usado para alocar a memório suficiente para o registro "lido" 
            total_estacoes++; // aumenta o total de estacoes para o valor atual
        }
        cabecalho.nroEstacoes = total_estacoes; // anota o total de estaçoes no cabeçalho

        // Pares Estações 
        // Verifico se o codProxEstacao existe no registro com -1, por definição
        if(registro.codProxEstacao != -1 && existe_par(registro.codEstacao, registro.codProxEstacao, pares, total_pares) == 0){ // se recebermos um 0
            pares = realloc(pares, (total_pares + 1) * sizeof(Par));  // alocamos em pares mais um espaço no total de pares do tipo Par(struct)
            pares[total_pares].origem = registro.codEstacao;  // anotamos essa nova origem na lista de origens 
            pares[total_pares].destino = registro.codProxEstacao; // anotamos esse novo destino na lista de destinos
            total_pares++; // aumentamos o tamnho
        }
        cabecalho.nroParesEstacoes = total_pares; // mandamos o tamanho atual para o cabeçalho

        // Escreve registro no arquivo binário
        write_registro_bin(registro, binario); // chamamos a função para escrever no bínario

        free(registro.nomeEstacao); // libero após todo registro
        registro.nomeEstacao = NULL;
        free(registro.nomeLinha); // libero após todo registro
        registro.nomeLinha = NULL;

        ultimoRRN++; 
        cabecalho.proxRRN = ultimoRRN;
    }
    // Colocamos o "ponteiro" no começo do arquivo para garantir que estamos no cabeçalho
    fseek(binario, 0, SEEK_SET); 

    // Anotamos no cabeçalho e 
    cabecalho.status = '1'; // agora consistente (temos que alterar para mostrar que está consistente) - PRECISAMOS FAZER ISSO POR ULTIMO?
    fwrite(&cabecalho.status, 1, 1, binario);
    fwrite(&cabecalho.topo, sizeof(int), 1, binario);
    fwrite(&cabecalho.proxRRN, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroEstacoes, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroParesEstacoes, sizeof(int), 1, binario);

    // Fechamos os arquivos
    fclose(csv);
    fclose(binario);

    // Libera memória
    free(pares); // libero só pares pois só alocamos o pares do tipo Par(struct) então só precisamos liberar o Pares
    pares = NULL;

    free(registro.nomeEstacao);
    registro.nomeEstacao = NULL;

    free(registro.nomeLinha);
    registro.nomeLinha = NULL;

    for(int i = 0; i < total_estacoes; i++){
        free(estacao[i]); // liberamos cada um das alocações da lista de estações (libera cada string)
    }
    free(estacao); // Libera o vetor de ponteiros
    estacao = NULL;

    // Reset de variaveis
    total_estacoes = 0;
    total_pares = 0;
    ultimoRRN = 0;

    // printa o binário na tela
    BinarioNaTela(arq_bin);
}

// escrever em um arquivo binário da forma esperada (atualizar o RRN do status)
void write_registro_bin(reg_dados dados, FILE *binario){ // arquivo binário, para escrever
 // Verificação de tamanho com base no tamanho do nome
  int tamanho_variados = 37 + dados.tamNomeEstacao + dados.tamNomeLinha; // 37 bytes fixos + bytes variados pois vetores de char são 1 byte cada caracter
  int lixo = 80 - tamanho_variados; // lixo para ser completado com $
    if (tamanho_variados > 80) {
        printf("Erro: registro maior que 80 bytes\n");
        return;
    }

  //fwrite(buffer ,            sizeof,numero, ponteiro); - escrita 
    fwrite(&dados.status_removido, 1, 1, binario); 
    fwrite(&dados.prox_queue, sizeof(int), 1, binario);  // 4 bytes e 1 elemento a ser lido 
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

    for(int i = 0; i < lixo; i++){ // completador de lixo do registro, insere após todo o registro
        char caracter_lixo = '$';
        fwrite(&caracter_lixo, 1, 1, binario); // Printa os lixos
    }
    // retornamos para a função anterior
}

// leitura csv, fazer filtro para interpretar cada coluna da tabela ",", filtrar condições se dado pode ser escrito