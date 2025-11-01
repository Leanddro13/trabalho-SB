#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINHAS 500
#define MAX_TAM_LINHA 200
#define MAX_MACROS 10
#define MAX_PARAM 5

typedef struct {
    char linhas[MAX_LINHAS][MAX_TAM_LINHA];
    int numLinhas;
} CodigoAssembly;

typedef struct {
    char nome[50];
    int numParametros;
    char parametros[MAX_PARAM][20];
    char corpo[MAX_LINHAS][MAX_TAM_LINHA];
    int numLinhasCorpo;
} Macro;

typedef struct {
    Macro macros[MAX_MACROS];
    int numMacros;
} ListaMacros;

// Funções auxiliares:
void removeNewline(char *str) {
    str[strcspn(str, "\n")] = '\0';
}

void lerArquivo(const char *nomeArquivo, CodigoAssembly *codigo) {
    FILE *arquivo = fopen(nomeArquivo, "r");
    if (!arquivo) {
        perror("Erro ao abrir o arquivo");
        exit(1);
    }

    codigo->numLinhas = 0;
    while (fgets(codigo->linhas[codigo->numLinhas], MAX_TAM_LINHA, arquivo)) {
        removeNewline(codigo->linhas[codigo->numLinhas]);
        codigo->numLinhas++;
    }

    fclose(arquivo);
}

int contarMacros(CodigoAssembly *codigo) {
    int count = 0;
    for (int i = 0; i < codigo->numLinhas; i++) {
        if (strstr(codigo->linhas[i], "MACRO") && !strstr(codigo->linhas[i], "ENDMACRO"))
            count++;
    }
    return count;
}

// Extração de macros:
void extrairMacros(CodigoAssembly *codigo, ListaMacros *lista) {
    lista->numMacros = 0;

    for (int i = 0; i < codigo->numLinhas; i++) {
        if (strstr(codigo->linhas[i], "MACRO") && !strstr(codigo->linhas[i], "ENDMACRO")) {
            Macro *m = &lista->macros[lista->numMacros++];
            m->numLinhasCorpo = 0;

            char linhaCopia[MAX_TAM_LINHA];
            strcpy(linhaCopia, codigo->linhas[i]);

            // Pega nome da macro (antes dos dois pontos)
            char *token = strtok(linhaCopia, ":");
            strcpy(m->nome, token);

            // Extrai parâmetros
            char *p = strstr(codigo->linhas[i], "MACRO");
            if (p) {
                p += strlen("MACRO");
                m->numParametros = 0;
                token = strtok(p, " ,");
                while (token && token[0] == '&') {
                    strcpy(m->parametros[m->numParametros++], token + 1); // remove o '&'
                    token = strtok(NULL, " ,");
                }
            }

            // Captura corpo até ENDMACRO
            i++;
            while (i < codigo->numLinhas && !strstr(codigo->linhas[i], "ENDMACRO")) {
                strcpy(m->corpo[m->numLinhasCorpo++], codigo->linhas[i]);
                i++;
            }
        }
    }
}

// Expansão de macros:
void expandirLinha(char *destino, const char *linha, Macro *macro, char argumentos[MAX_PARAM][20]) {
    strcpy(destino, linha);
    for (int i = 0; i < macro->numParametros; i++) {
        char procura[25], substitui[25];
        sprintf(procura, "&%s", macro->parametros[i]);
        strcpy(substitui, argumentos[i]);

        char buffer[MAX_TAM_LINHA];
        char *pos;
        while ((pos = strstr(destino, procura)) != NULL) {
            *pos = '\0';
            sprintf(buffer, "%s%s%s", destino, substitui, pos + strlen(procura));
            strcpy(destino, buffer);
        }
    }
}

void expandirMacros(CodigoAssembly *codigo, ListaMacros *lista) {
    CodigoAssembly novoCodigo;
    novoCodigo.numLinhas = 0;
    int dentroDeMacro = 0;

    for (int i = 0; i < codigo->numLinhas; i++) {
        // Ignorar linhas dentro da definição da macro
        if (strstr(codigo->linhas[i], "MACRO") && !strstr(codigo->linhas[i], "ENDMACRO")) {
            dentroDeMacro = 1;
            continue;
        }
        if (strstr(codigo->linhas[i], "ENDMACRO")) {
            dentroDeMacro = 0;
            continue;
        }
        if (dentroDeMacro) continue;

        int expandido = 0;

        // Tenta expandir cada macro conhecida
        for (int j = 0; j < lista->numMacros; j++) {
            Macro *m = &lista->macros[j];
            if (strstr(codigo->linhas[i], m->nome) &&
                !strstr(codigo->linhas[i], "MACRO") &&
                !strstr(codigo->linhas[i], "ENDMACRO")) {

                // Obter argumentos
                char linhaCopia[MAX_TAM_LINHA];
                strcpy(linhaCopia, codigo->linhas[i]);

                strtok(linhaCopia, " "); // pula nome
                char *token = strtok(NULL, " ,");
                char argumentos[MAX_PARAM][20];
                int narg = 0;

                while (token && narg < MAX_PARAM) {
                    strcpy(argumentos[narg++], token);
                    token = strtok(NULL, " ,");
                }

                // Expandir corpo
                for (int k = 0; k < m->numLinhasCorpo; k++) {
                    char linhaExpandida[MAX_TAM_LINHA];
                    expandirLinha(linhaExpandida, m->corpo[k], m, argumentos);
                    strcpy(novoCodigo.linhas[novoCodigo.numLinhas++], linhaExpandida);
                }

                expandido = 1;
                break;
            }
        }

        if (!expandido) {
            strcpy(novoCodigo.linhas[novoCodigo.numLinhas++], codigo->linhas[i]);
        }
    }

    *codigo = novoCodigo;
}

// Saída:
void gerarArquivoSaida(CodigoAssembly *codigo, const char *nomeSaida) {
    FILE *arquivo = fopen(nomeSaida, "w");
    if (!arquivo) {
        perror("Erro ao criar arquivo de saída");
        exit(1);
    }

    for (int i = 0; i < codigo->numLinhas; i++) {
        fprintf(arquivo, "%s\n", codigo->linhas[i]);
    }

    fclose(arquivo);
}

// Main:
int main() {
    CodigoAssembly codigo;
    ListaMacros lista;

    lerArquivo("entrada.asm", &codigo);
    int qtdMacros = contarMacros(&codigo);

    if (qtdMacros == 1) {
        extrairMacros(&codigo, &lista);
        for (int i = 0; i < 2; i++) expandirMacros(&codigo, &lista);
    } else if (qtdMacros == 2) {
        extrairMacros(&codigo, &lista);
        for (int i = 0; i < 5; i++) expandirMacros(&codigo, &lista);
    } else {
        printf("Nenhuma macro detectada ou mais de duas macros.\n");
    }

    gerarArquivoSaida(&codigo, "saida.pre");
    printf("Arquivo 'saida.pre' gerado com sucesso\n");

    return 0;
}
