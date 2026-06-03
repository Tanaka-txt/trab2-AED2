/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

#include "features.h"


int main() {
  int option, RRN;
  char arq_csv[256], arq_bin[256]; // buffer do tamanho de uma linha

  while(scanf("%d", &option) == 1) {
    getchar(); // limpa o buffer por conta do \n
      switch (option){
        case 1 :
          /*Função*/
          // printf("1\n");

          // Verificamos se a entrada é diferente de vazio
          scanf("%s %s", arq_csv, arq_bin);
          create_cabecalho();
          create_regi_bin(arq_csv, arq_bin);
          // BinarioNaTela("estacoes.bin"); teste para ver 
          break;

        case 2 :
          /*Função*/
          // Ler nome do arquivo binário
          scanf("%s", arq_bin);
          read_bin(arq_bin);
          // printf("2\n");
          break;

        case 3 :
          // Ler nome do arquivo binário e buscar
          scanf("%s", arq_bin);
          busca_bin(arq_bin);
          break;

        case 4 :
          /*Função*/
          // printf("4\n");
          scanf("%s %d", arq_bin, &RRN);
          busca_por_rrn(arq_bin, RRN);
          break;

        case 0 :
          /*Exit*/
          break;

        default:
        /* Caso que não é nenhum da erro ex. -1 -2  9 */
          printf("Erro\n");
          exit(0);
      }
  }
  return 0;
}