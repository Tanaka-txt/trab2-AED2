## ⚠️ Restrições Gerais do Projeto
Antes de iniciar as funcionalidades, certifique-se de que sua implementação em **C** garanta as seguintes regras globais:

* **Identificação:** O código deve conter um comentário no início com o NUSP e nome do(s) aluno(s).
* **Manipulação de Arquivos:** * Uso exclusivo da biblioteca `<stdio.h>`.
    * Leitura e gravação estritamente em **modo binário** (nada de modo texto).
    * A escrita dos dados deve ser feita **campo a campo** (não é permitido escrever o registro inteiro de uma vez).
* **Estrutura dos Dados:** Nomes, tamanhos, ordem e existência dos campos são fixos. Não adicione nem altere campos.
* **Documentação:** O código deve ser rigorosamente documentado (funções, procedimentos, variáveis e blocos lógicos).
* **Tratamento de Dados:**
    * Manipule valores nulos conforme a especificação de cada funcionalidade.
    * Não é necessário tratar truncamento de dados.
* **Saídas:** Exiba apenas as mensagens de erro ou saídas exatamente como especificado.

---

## Funcionalidade [5]: CREATE INDEX (Criação de Índice Primário)
**Objetivo:** Criar um arquivo de índice primário binário (`indexaEstacao.bin`) a partir de um arquivo de dados existente, indexando pelo campo chave primária.

* **Comando de Entrada:** `5 arquivoEntrada.bin arquivoIndicePrimario.bin`
* **Regras de Implementação:**
    * Ler o arquivo de dados sequencialmente.
    * Gerar o arquivo de índice primário contendo um registro de cabeçalho e os registros de dados (pares de chave de busca e byte offset).
* **Saída Esperada:**
    * *Sucesso:* Executar a função `binarioNaTela()` passando o nome do arquivo de índice gerado.
    * *Erro:* Exibir `Falha no processamento do arquivo.`

---

## Funcionalidade [6]: SELECT WHERE (Busca de Registros)
**Objetivo:** Recuperar e exibir registros que satisfaçam a um critério de busca (um ou múltiplos campos combinados).

* **Comando de Entrada:** `6 arquivoEntrada.bin arquivoIndicePrimario.bin n` seguido por `n` linhas de buscas.
* **Regras de Implementação:**
    * **Busca Indexada:** Se o campo `codEstacao` estiver entre os critérios, a busca **deve** usar o arquivo de índice para encontrar o registro diretamente.
    * **Busca Sequencial:** Para quaisquer outros campos, percorrer o arquivo de dados sequencialmente.
    * Ignorar completamente registros marcados como logicamente removidos.
    * Usar a função `scan_quote_string` para ler *strings* entre aspas duplas (`""`). Campos nulos na entrada são lidos como `NULO`.
* **Saída Esperada:**
    * *Sucesso:* Imprimir os campos de cada registro encontrado na mesma linha, separados por espaço, na ordem: `codEstacao`, `nomeEstacao`, `codLinha`, `nomeLinha`, `codProxEstacao`, `distProxEstacao`, `codLinhaIntegra`, `codEstIntegra`.
        * Campos numéricos (tamanho fixo) nulos: imprimir `NULO` (e não `-1`).
        * Campos *string* (tamanho variável) nulos: imprimir `NULO`.
    * *Nenhum registro encontrado:* Exibir `Registro inexistente.`
    * *Erro:* Exibir `Falha no processamento do arquivo.`

---

## Funcionalidade [7]: DELETE (Remoção Lógica)
**Objetivo:** Remover logicamente registros que atendam aos critérios de busca utilizando a **abordagem dinâmica** (pilha de removidos).

* **Comando de Entrada:** `7 arquivoEntrada.bin arquivoIndicePrimario.bin n` seguido por `n` especificações de critérios de remoção.
* **Regras de Implementação:**
    * Identificar os registros usando as mesmas lógicas de busca da Funcionalidade [6] (Indexada ou Sequencial).
    * Implementar a **pilha de registros logicamente removidos** rigorosamente conforme visto em aula.
    * **Manutenção de Dados:** Ao remover, alterar **apenas** o status de remoção e o encadeamento. Os bytes dos outros campos no arquivo devem permanecer intactos.
    * **Atualização do Índice:** A chave de busca correspondente ao registro removido deve ser retirada do arquivo de índice primário.
    * O laço deve executar `n` vezes, mesmo que algumas buscas não encontrem registros.
* **Saída Esperada:**
    * *Sucesso:* Executar `binarioNaTela()` duas vezes: uma para o arquivo de dados e outra para o arquivo de índice.
    * *Erro:* Exibir `Falha no processamento do arquivo.`

---

## Funcionalidade [8]: INSERT INTO (Inserção de Registros)
**Objetivo:** Inserir novos registros no arquivo de dados reaproveitando o espaço de registros logicamente removidos (abordagem dinâmica).

* **Comando de Entrada:** `8 arquivoEntrada.bin arquivoIndicePrimario.bin n` seguido por `n` linhas de novos registros.
* **Regras de Implementação:**
    * Recuperar o espaço do topo da pilha de removidos. Se não houver, inserir no final do arquivo.
    * **Tratamento de Lixo:** Todo o espaço remanescente (lixo) no registro reaproveitado que não for sobrescrito pelos novos dados deve ser preenchido com o caractere `$`.
    * **Atualização do Índice:** A nova chave primária deve ser inserida ordenadamente no arquivo de índice.
    * As *strings* da entrada possuem aspas (usar `scan_quote_string`) e os nulos vêm escritos como `NULO`.
* **Saída Esperada:**
    * *Sucesso:* Executar `binarioNaTela()` para o arquivo de dados e para o arquivo de índice.
    * *Erro:* Exibir `Falha no processamento do arquivo.`

---

## Funcionalidade [9]: UPDATE (Atualização de Registros)
**Objetivo:** Atualizar valores de campos em registros específicos, baseando-se na busca por critérios e utilizando reaproveitamento dinâmico.

* **Comando de Entrada:** `9 arquivoEntrada.bin arquivoIndicePrimario.bin n` seguido de `n` blocos com os campos de busca e, em seguida, os campos que serão atualizados.
* **Regras de Implementação:**
    * Como os registros possuem tamanho fixo, a atualização ocorre **diretamente no mesmo registro (in-place)**. Não é necessário mover o registro.
    * Registros marcados como removidos devem ser ignorados na busca.
    * **Tratamento de Lixo:** Se a nova *string* for menor que a antiga, o lixo gerado no campo deve ser preenchido com o caractere `$`.
    * **Atualização do Índice:** Se o campo atualizado for o `codEstacao` (chave primária), o arquivo de índice primário deve ser atualizado para refletir o novo valor.
    * O ciclo de atualizações deve rodar `n` vezes, mesmo que algumas buscas retornem vazias.
* **Saída Esperada:**
    * *Sucesso:* Executar `binarioNaTela()` para o arquivo de dados e para o arquivo de índice.
    * *Erro:* Exibir `Falha no processamento do arquivo.`

