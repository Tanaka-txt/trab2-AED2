/*
- para cada linha válida, imprimir os dados em uma única linha no terminal, separando tudo por um espaço em branco
- a ordem de impressão tem que ser exatamente esta: código da estação, nome da estação, código da linha, nome da linha, código da próxima estação, distância, código da integração e, por fim, a estação de integração.
- antes do nome da estação tem um nº que indica o tamanho daquele nome --> se for 0 imprime NULO
- como 'registro' é um ponteiro para a struct, foi usado o operador de seta '->' para acessar os campos internos
*/

#include "impressao.h"

void imprimir_registro (reg_dados *registro){ // recebe um ponteiro (*registro) do tipo reg_dados
    
    printf("%d %s ", registro->codEstacao, registro->nomeEstacao); // nunca vão ser nulas, por isso não precisa de verificação

    // campos fixos que tiverem o valor nulo, recebem o valor -1 na leitura e tem que printar NULO
    // campos variáveis que tiverem o valor nulo, tem indicador de tamanho igua a 0 e devem printar NULO

    if (registro->codLinha == -1) {
        printf("NULO "); // substitui a impressão do -1 pela palavra "NULO"
    } else{
        printf("%d ", registro->codLinha);
    }

    if (registro->tamNomeLinha == 0){
        printf("NULO ");
    }else {
        printf("%s ", registro->nomeLinha); // se o tamanho for maior que 0, a string existe e pode ser impressa
    }

    if (registro->codProxEstacao == -1){
        printf("NULO ");
    }else {
        printf("%d ", registro->codProxEstacao);
    }

    if (registro->distProxEstacao == -1){
        printf("NULO ");
    }else {
        printf("%d ", registro->distProxEstacao);
    }

    if (registro->codLinhaIntegra == -1){
        printf("NULO ");
    }else {
        printf("%d ", registro->codLinhaIntegra);
    }

    if (registro->codEstIntegra == -1){
        printf("NULO\n");
    }else {
        printf("%d\n", registro->codEstIntegra);
    }
}

