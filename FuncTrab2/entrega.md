Material para Entregar
Arquivo compactado (a ser entregue no run.codes)
Deve ser preparado um arquivo .zip contendo:
• Código fonte do programa devidamente documentado.
• Makefile para a compilação do programa.
Vídeo (a ser entregue no e-disciplinas)
• Um vídeo gravado pelos integrantes do grupo, o qual deve ter, no máximo, 7
minutos de gravação. O vídeo deve explicar o trabalho desenvolvido. Ou seja, o
grupo deve apresentar: cada funcionalidade e uma breve descrição de como a
funcionalidade foi implementada. Todos os integrantes do grupo devem
participar do vídeo, sendo que o tempo de apresentação dos integrantes deve ser
balanceado. Ou seja, o tempo de participação de cada integrante deve ser
aproximadamente o mesmo. O uso da webcam é obrigatório.
Instruções para fazer o arquivo makefile. No [run.codes] tem uma orientação para
que, no makefile, a diretiva “all” contenha apenas o comando para compilar seu
programa e, na diretiva “run”, apenas o comando para executá-lo. Adicionalmente, para
utilizar a função binarioNaTela, é necessário usar a flag -lmd. Assim, a forma mais
simples de se fazer o arquivo makefile é:
all:
Av. Trabalhador São-carlense, 400 . centro . São Carlos - SP cep 13566-590 . Brasil . www.icmc.usp.br
gcc -o programaTrab *.c -lmd
run:
./programaTrab
Lembrando que *.c já engloba todos os arquivos .c presentes no seu zip.
Adicionalmente, no arquivo Makefile é importante se ter um tab nos locais colocados
acima, senão ele pode não funcionar.
Instruções de entrega.
O programa deve ser submetido via [run.codes]:
• página: https://runcodes.icmc.usp.br/
• Código de matrícula: L6SW
O vídeo gravado deve ser submetido por meio da página da disciplina no e-disciplinas,
no qual o grupo vai informar o nome de cada integrante, o número do grupo e um link
que contém o vídeo gravado. Ao submeter o link, verifique se o mesmo pode ser
acessado. Vídeos cujos links não puderem ser acessados receberão nota zero. Vídeos
corrompidos ou que não puderem ser corretamente acessados receberão nota zero