/*
Membros do grupo:
Laysa Almeida de Oliveira - NºUSP 14588002
Júlio César Tanaka Vergamini - NºUSP 15466276
*/

/*
- realizar uma busca por critério --> abrir o arquivo, pular o cabeçalho e usar o while para ler linha por linha (80 em 80 bytes), mas com a opção de procurar por algo
- o programa vai ler o que o usuário quer buscar (o nome do campo) e qual é o valor (o critério)
- antes de finalizar e printar igual a func2, precisa de um if de verificação, para saber se a informação lida para comparar com o que está sendo buscado
    - se for igual, imprimo a linha igual a lógica da func2
    - se for diferente, não imprime nada e dou um fseeek para pular pra próxima linha
- um contator para saber quantas linhas passaram no filtro, se continuar 0, imprime a mensagem de erro
- n é o número de buscas que o usuário quer fazer --> um laço for (int i = 0; i < n; i++) --> cada busca, lê o arquivo do começo ao fim
- dentro de cada busca, o usuário digita m, que é a quantidade de filtros (critérios)
IDEIA DO LAÇO:   
Lê os 'm' filtros que o usuário deseja aplicar na busca. O processo de leitura funciona assim: 
    1º - Identifica qual é o campo buscado (ex: "codLinha").
    2º - Ativa a busca desse campo (setando = 1 no painel).
    3º - Lê o valor desejado. Para os campos numéricos, a leitura é feita primeiro como texto (na variável 'aux'). Isso impede que a palavra "NULO" quebre o scanf, permitindo a conversão segura para inteiro (usando atoi) logo em seguida.
*/

#include "features.h"
#include "impressao.h"
#include "leitura.h"
#include "fornecidas.h"
#include "registro.h"
#include "utils.h"

// com esse "painel", o usuário digita o que ele quer, o programa lê a palavra e mudar a flag (busca_X) de 0 para 1 para indicar que aquele campo será filtrado
// a struct agrupa as flags (se busca ou não) e os valores buscados (critério)
typedef struct {
    int busca_codEstacao;       int valor_codEstacao;
    int busca_nomeEstacao;      char valor_nomeEstacao[50];
    int busca_codLinha;         int valor_codLinha;
    int busca_nomeLinha;        char valor_nomeLinha[50];
    int busca_codProxEstacao;   int valor_codProxEstacao;
    int busca_distProxEstacao;  int valor_distProxEstacao;
    int busca_codLinhaIntegra;  int valor_codLinhaIntegra;
    int busca_codEstIntegra;    int valor_codEstIntegra;
}Painel_Busca;

void busca_bin(char *arq_bin){
    FILE *teste = fopen(arq_bin, "rb"); // rb --> read binary
    if (teste == NULL) { // verifica se foi possível ler o arquivo binário
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    int n; // quantidade de buscas (consultas SELECT) que o usuário precisa fazer
    scanf ("%d", &n);

    for (int i = 0; i < n; i++){ // laço para cada busca, lê o arquivo do começo ao fim
        Painel_Busca painel = {0}; // zerando o painel de busca para o novo filtro, é crucial para limpar o "lixo de memória" da busca anterior
            
        int m; 
        scanf("%d", &m); // quantidade de filtros (critérios) a serem buscados

        for (int j = 0; j < m; j++){ // laço para ler os pares "nomeCampo" e "valorCampo" que compõem os filtros
            char palavra [30];       // vetor de char para armazenar o nome do campo digitado
            scanf("%s", palavra);    // lê o campo que o usuário quer buscar

            // strcmp vai comparar as strings, caracter por caracter e verifica se são iguais ou não
            if (strcmp(palavra, "codEstacao") == 0) {
                // significa que o usuário quer pesquisar isso. Lê o texto e converte para número ou trata o "NULO"
                ler_criterio_int(&painel.busca_codEstacao, &painel.valor_codEstacao);

            } else if (strcmp(palavra, "nomeEstacao") == 0) {
                painel.busca_nomeEstacao = 1;
                ScanQuoteString(painel.valor_nomeEstacao); // a função fornecida que lê strings entre aspas duplas

            } else if (strcmp(palavra, "codLinha") == 0) {
                ler_criterio_int(&painel.busca_codLinha, &painel.valor_codLinha);

            } else if (strcmp(palavra, "nomeLinha") == 0) {
                painel.busca_nomeLinha = 1;
                ScanQuoteString(painel.valor_nomeLinha);

            } else if (strcmp(palavra, "codProxEstacao") == 0) {
                ler_criterio_int(&painel.busca_codProxEstacao, &painel.valor_codProxEstacao);

            } else if (strcmp(palavra, "distProxEstacao") == 0) {
                ler_criterio_int(&painel.busca_distProxEstacao, &painel.valor_distProxEstacao);

            } else if (strcmp(palavra, "codLinhaIntegra") == 0) {
                ler_criterio_int(&painel.busca_codLinhaIntegra, &painel.valor_codLinhaIntegra);

            } else if (strcmp(palavra, "codEstIntegra") == 0) {
                ler_criterio_int(&painel.busca_codEstIntegra, &painel.valor_codEstIntegra);
            }
        }  
        
        fseek(teste, 17, SEEK_SET); // pula o cabeçalho para iniciar a leitura
        
        int registroValido = 0; // contador para ver quantas buscas deram certo 
        reg_dados registro;     // struct que guarda o que acabou de ser lido
        int status_leitura;     // variável para ser ajudante de leitura da função ler_registro
        int encontrou = 0;      // flag para indicar que já encontrou um registro com codEstacao

        while (1){
            memset(&registro, 0, sizeof(reg_dados)); // zerando a struct para garantir que os ponteiros comecem como NULL
            status_leitura = ler_registro(teste, &registro); // a função ler_registro lê o byte "removido" ('0' ou '1') e os demais dados do registro

            if (status_leitura == 0){ // arquivo acabou --> sai do laço while
                break;
            }
            if (status_leitura == 2){ // registro apagado (tem o '1') --> pula os próximos bytes e volta para o início do laço
                continue;
            }

            int pontos = 0; // registro lido começa com zero no filtro. 

            // se o campo está marcado para busca (flag == 1) e o valor lido é igual ao valor buscado, ganha 1 ponto
            if (painel.busca_codEstacao == 1 && registro.codEstacao == painel.valor_codEstacao) pontos ++; 
            if (painel.busca_codLinha == 1 && registro.codLinha == painel.valor_codLinha) pontos ++;
            if (painel.busca_codProxEstacao == 1 && registro.codProxEstacao == painel.valor_codProxEstacao) pontos ++;
            if (painel.busca_distProxEstacao == 1 && registro.distProxEstacao == painel.valor_distProxEstacao) pontos ++;
            if (painel.busca_codLinhaIntegra == 1 && registro.codLinhaIntegra == painel.valor_codLinhaIntegra) pontos ++;
            if (painel.busca_codEstIntegra == 1 && registro.codEstIntegra == painel.valor_codEstIntegra) pontos ++;

            // para os textos, é preciso usar strcmp para realizar a comparação
            if (painel.busca_nomeEstacao == 1 && registro.nomeEstacao != NULL && strcmp (registro.nomeEstacao, painel.valor_nomeEstacao) == 0) pontos ++;
            if (painel.busca_nomeLinha == 1 && registro.nomeLinha != NULL && strcmp (registro.nomeLinha, painel.valor_nomeLinha) == 0) pontos ++;
            // a função strcmp quebra o programa se tentar ler um ponteiro NULL. Com a verificação '!= NULL' antes do strcmp para testa a existência da string primeiro 
            // se for NULL, a verificação "entra em curto-circuito" e ignora a execução do strcmp

            if (pontos == m){ // se a quantidade de pontos for igual ao nº de exigências vai ser um registro válido
                imprimir_registro(&registro);
                registroValido ++; 
                
                // se a busca incluiu codEstacao, pode parar (pois é único)
                if (painel.busca_codEstacao == 1) {
                    encontrou = 1;
                    // libera memória e sai do loop
                    liberar_strings_registro(&registro);
                    break;
                }
            }
                
            // limpando a memória dos mallocs antes que o while rode novamente (executado se não der o break acima)
            liberar_strings_registro(&registro);
        }

        // verifica o resultado após sair do loop while
        if (encontrou) {
        } else if (registroValido == 0) {
            printf("Registro inexistente.\n"); // se não tiver nenhum registro válido
        }  

        if (i < n - 1){
            printf("\n"); // se a busca que acabou de rodar não for a última, imprime uma linha em branco --> quebra de linhas para a saída no Runcodes
        }
    }
    fclose(teste);
}