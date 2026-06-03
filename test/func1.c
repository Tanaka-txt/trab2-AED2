#include "features.h"
#include "leitura.h"
#include "registro.h"
#include "fornecidas.h"

// Definição de Var


// Pares Estações
typedef struct {
    int origem;
    int destino;
} Par;


// leitura csv, fazer filtro para interpretar cada coluna da tabela ",", filtrar condições se dado pode ser escrito

// Cria Cabeçalho
reg_cabecalho cabecalho;
reg_dados registro;


void write_registro_bin(reg_dados dados, FILE *binario); // Chamando ele aqui para usar no read csv


int ultimoRRN = 0; // tem que apontar para uma atualização do RRN

// Nro estacoes 👀
char **estacao = NULL;
int total_estacoes = 0;

// Pares Estações 👀
Par *pares = NULL;
int total_pares = 0;

void create_cabecalho(){
  cabecalho.status = '1';
  cabecalho.topo = -1;
  cabecalho.proxRRN = 0; // Comeã em 0, 0+1 = 1
  cabecalho.nroEstacoes = 0;
  cabecalho.nroParesEstacoes = 0;
}

//                 RegEstacoes   Estacoes   totalEstacoes
int existe_estacao(char *nome, char **lista, int tamanho){ // função de busca!👀
    for(int i = 0; i < tamanho; i++){  // Faz um loop com o total de estacoes👀
        if(strcmp(nome, lista[i]) == 0) // Verifica o nome da estação atual com o a lista de nome de estações👀
            return 1;
    }
    return 0;
}

//Pares Estações 👀
int existe_par(int a, int b, Par *lista, int tamanho){ //👀
  for(int i = 0; i < tamanho; i++){ // 👀
    if(lista[i].origem == a && lista[i].destino == b) // 👀
        return 1; // 👀
  }
    return 0; // 👀
}

void create_regi_bin(char arq_csv[256], char arq_bin[256]){
    // =-=-= Abrimos o Arquivo CSV e Binário=-=-=
    FILE *csv = fopen(arq_csv, "r"); 
    FILE *binario = fopen(arq_bin, "wb+"); 

    if (csv == NULL || binario == NULL) { 
        printf("Falha no processamento do arquivo.");
        return;
    }

    // =-=-= Cabeçalho =-=-= 
    create_cabecalho();
    cabecalho.status = '0'; // agora inconsistente

    char linha[256]; 
    reg_dados registro;

    fgets(linha, 256, csv); // Pula o cabeçalho do csv

    fseek(binario, 0, SEEK_SET); 
    fwrite(&cabecalho.status, 1, 1, binario);
    fwrite(&cabecalho.topo, sizeof(int), 1, binario);
    fwrite(&cabecalho.proxRRN, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroEstacoes, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroParesEstacoes, sizeof(int), 1, binario);

    fseek(binario, 17, SEEK_SET); 

    // Loop de leitura do CSV
    while (fgets(linha, 256, csv)) { 
        
        // Chama a nova função modularizada passando a linha e o endereço de registro
        le_linha_csv(linha, &registro, cabecalho.topo);

        // Verifica nroEstacoes 👀
        if (!existe_estacao(registro.nomeEstacao, estacao, total_estacoes)) { 
            estacao = realloc(estacao, (total_estacoes + 1) * sizeof(char*)); 
            estacao[total_estacoes] = strdup(registro.nomeEstacao); 
            total_estacoes++; 
        }
        cabecalho.nroEstacoes = total_estacoes;

        // Pares Estações 👀
        if (registro.codProxEstacao != -1 && !existe_par(registro.codEstacao, registro.codProxEstacao, pares, total_pares)) { 
            pares = realloc(pares, (total_pares + 1) * sizeof(Par)); 
            pares[total_pares].origem = registro.codEstacao; 
            pares[total_pares].destino = registro.codProxEstacao; 
            total_pares++;
        }
        cabecalho.nroParesEstacoes = total_pares;

        // Escreve registro no arquivo binário
        write_registro_bin(registro, binario);

        ultimoRRN++; 
        cabecalho.proxRRN = ultimoRRN;
    }

    fseek(binario, 0, SEEK_SET); 
    cabecalho.status = '1'; // agora consistente
    fwrite(&cabecalho.status, 1, 1, binario);
    fwrite(&cabecalho.topo, sizeof(int), 1, binario);
    fwrite(&cabecalho.proxRRN, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroEstacoes, sizeof(int), 1, binario);
    fwrite(&cabecalho.nroParesEstacoes, sizeof(int), 1, binario);

    fclose(csv); 
    fclose(binario);

    // Libera memória
    free(pares);
    pares = NULL;
    total_pares = 0;

    free(registro.nomeEstacao);
    free(registro.nomeLinha);

    for(int i = 0; i < total_estacoes; i++){
        free(estacao[i]);
    }
    free(estacao);
    estacao = NULL;
    total_estacoes = 0;
    ultimoRRN = 0;

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

  for(int i = 0; i < lixo; i++){
    char caracter_lixo = '$';
    fwrite(&caracter_lixo, 1, 1, binario); // Printa os lixos
  }
}
