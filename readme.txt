Nome do Grupo:
- Leandro Souza da Silva - 211031109
- João Victor Prata - 202028857
- Rafael Bonach - 221008365

Como rodar:

No Linux:
    Para rodar o pré-processamento e o compilador, digite:
    gcc preprocessamento.c -o preproc && ./preproc && gcc compilador.c -o compilador && ./compilador arquivo.asm

    Para rodar apenas o pré-processamento digite:
    gcc preprocessamento.c -o preproc && ./preproc

    Para rodar apenas o compilador digite:
    gcc compilador.c -o compilador && ./compilador arquivo.asm

No Windows (CodeBlocks ou terminal):
    Para rodar o pré-processamento e o compilador, digite:
    gcc preprocessamento.c -o preproc && ./preproc && gcc compilador.c -o compilador.exe && compilador.exe arquivo.asm

    Para rodar apenas o pré-processamento digite:
    gcc preprocessamento.c -o preproc && ./preproc

    Para rodar apenas o compilador digite:
    gcc compilador.c -o compilador.exe && compilador.exe arquivo.asm

Todos os arquivos (.asm, .pre, .o1, .o2) devem estar na mesma pasta.
