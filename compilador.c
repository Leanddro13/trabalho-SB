#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>

#define MAX_LINHAS 1000
#define MAX_OPCODES 14
const char *tabela_intrucoes_upper[MAX_OPCODES][2] = {
    {"ADD", "1"},
    {"SUB", "2"},
    {"MUL", "3"},
    {"DIV", "4"},
    {"JMP", "5"},
    {"JMPN", "6"},
    {"JMPP", "7"},
    {"JMPZ", "8"},
    {"COPY", "9"},
    {"LOAD", "10"},
    {"STORE", "11"},
    {"INPUT", "12"},
    {"OUTPUT", "13"},
    {"STOP", "14"}
};
const char *tabela_intrucoes_lower[MAX_OPCODES] = {
    "add",
    "sub",
    "mul",
    "div",
    "jmp",
    "jmpn",
    "jmpp",
    "jmpz",
    "copy",
    "load",
    "store",
    "input",
    "output",
    "stop"
};
const char *tabela_diretivas[] = {
    "space",
    "const",
};

void imprimeArquivoObjeto(int *codigo_objeto, int tamanho){
    FILE *arquivo_objeto = fopen("output.o1", "w");
    if(!arquivo_objeto){
        printf("Erro ao criar o arquivo objeto\n");
        return;
    }
    for(int i = 0; i < tamanho; i++){
        if(i == tamanho - 1) fprintf(arquivo_objeto, "%d", codigo_objeto[i]);
        else fprintf(arquivo_objeto, "%d ", codigo_objeto[i]);
    }

    fclose(arquivo_objeto);
}

// Passagem única
void passagemUnica(char ***tabela_tokenizada){
    // Variáveis
    char *token = NULL, ***tabela_definicao = NULL; // Rótulo, Endereço
    int * codigo_objeto = NULL;
    int lin_atual = 0, rotulo_definido = 0, linha = 0, endereco_atual = 0, end_encontrado = 0;
    int operando_encontrado = 0;
    char *op_space, *op_const;

    // Percorre linha a linha pegando os tokens
    while (tabela_tokenizada[lin_atual]){
        token = tabela_tokenizada[lin_atual][0];
        // Verifica se o token existe
        if (token && tabela_definicao){
            //Procura na tabela de definições
            rotulo_definido = 0;
            for(linha = 0; tabela_definicao[linha]; linha++){
                if(strcmp(tabela_definicao[linha][0], token) == 0){
                    if(strcmp(tabela_definicao[linha][2], "P") == 0){
                        // Rótulo pendente, agora definido
                        tabela_definicao[linha][2] = strdup("S"); // Definido
                        tabela_definicao[linha][1] = malloc(5 * sizeof(char));
                        sprintf(tabela_definicao[linha][1], "%d", endereco_atual); // Endereço
                        rotulo_definido = 1;
                        break;
                    }else{
                        // Rótulo já existe
                        rotulo_definido = 1;
                        printf("Erro: rótulo '%s' já definido\n", token);
                        break;
                    }
                }
            }
            if(!rotulo_definido){
                // Adiciona o rótulo na tabela de definições
                // Rotulo, endereco, definido, ultima pendencia
                tabela_definicao = realloc(tabela_definicao, sizeof(char **) * (linha + 2));
                tabela_definicao[linha] = malloc(sizeof(char *) * 4);
                tabela_definicao[linha][0] = strdup(token); // Rótulo
                tabela_definicao[linha][1] = malloc(5 * sizeof(char));
                sprintf(tabela_definicao[linha][1], "%d", endereco_atual); // Endereço
                tabela_definicao[linha][2] = strdup("S"); // Definido
                tabela_definicao[linha][3] = strdup("-1"); // Última pendência
                tabela_definicao[linha + 1] = NULL; // Marca o fim da tabela
            }
        }else if (token && !tabela_definicao){
            // Primeira vez que adiciona um rótulo
            tabela_definicao = malloc(sizeof(char **) * 2);
            tabela_definicao[0] = malloc(sizeof(char *) * 4);
            tabela_definicao[0][0] = strdup(token); // Rótulo
            tabela_definicao[0][1] = malloc(5 * sizeof(char));
            sprintf(tabela_definicao[0][1], "%d", endereco_atual); // Endereço
            tabela_definicao[0][2] = strdup("D"); // Definido
            tabela_definicao[0][3] = strdup("-1"); // Última pendência
            tabela_definicao[1] = NULL; // Marca o fim da tabela
        }


        // Verifica a instrução
        token = tabela_tokenizada[lin_atual][1];
        end_encontrado = 0;
        for(int i = 0; i < MAX_OPCODES; i++){
            if(token != NULL && strcmp(token, tabela_intrucoes_lower[i]) == 0){
                // Instrução encontrada
                codigo_objeto = realloc(codigo_objeto, sizeof(int) * (endereco_atual + 1));
                codigo_objeto[endereco_atual] = i+1;
                endereco_atual++;
                end_encontrado = 1;
                break;
            }
        }
        if(!end_encontrado && strcmp(token, "const") == 0){
            // Diretiva CONST
            codigo_objeto = realloc(codigo_objeto, sizeof(int) * (endereco_atual + 1));
            codigo_objeto[endereco_atual] = atoi(tabela_tokenizada[lin_atual][2]); // Valor constante
            endereco_atual++;
        }else if(!end_encontrado && strcmp(token, "space") == 0){
            // Diretiva SPACE
            // Se houver um operando, usa seu valor como quantidade; caso contrário, reserva 1 posição
            op_space = tabela_tokenizada[lin_atual][2];
            int quantidade = 1;
            if(op_space != NULL){
                int valor = atoi(op_space);
                if(valor > 0) quantidade = valor;
            }
            for(int i = 0; i < quantidade; i++){
                codigo_objeto = realloc(codigo_objeto, sizeof(int) * (endereco_atual + 1));
                codigo_objeto[endereco_atual] = 0; // Espaço reservado
                endereco_atual++;
            }
        }else if(!end_encontrado){
            // Erro: Instrução inválida
            printf("Erro: Instrução inválida '%s'\n", token);
        }


        // Verifica os operandos
        for(int i = 2; i <=3; i++){
            if(tabela_tokenizada[lin_atual][i] == NULL || strcmp(tabela_tokenizada[lin_atual][1], "const") == 0 || strcmp(tabela_tokenizada[lin_atual][1], "space") == 0) continue;
            operando_encontrado = 0;
            token = tabela_tokenizada[lin_atual][i];
            if(tabela_definicao){
                // Procura na tabela de definicoes
                for(linha = 0; tabela_definicao[linha]; linha++){
                    op_const = tabela_definicao[linha][0];
                    if(strcmp(op_const, token) == 0){
                        // Operando encontrado
                        operando_encontrado = 1;
                        if(strcmp(tabela_definicao[linha][2], "S") == 0){
                            // Rótulo já definido
                            codigo_objeto = realloc(codigo_objeto, sizeof(int) * (endereco_atual + 1));
                            codigo_objeto[endereco_atual] = atoi(tabela_definicao[linha][1]);
                        }else{
                            // Rótulo ainda não definido
                            // adiciona último endereço pendente no endereço atual e atualiza o endereço pendente
                            int ultimo_endereco_pendente = atoi(tabela_definicao[linha][3]);
                            codigo_objeto = realloc(codigo_objeto, sizeof(int) * (endereco_atual + 1));
                            codigo_objeto[endereco_atual] = ultimo_endereco_pendente;
                            tabela_definicao[linha][3] = malloc(5 * sizeof(char));
                            sprintf(tabela_definicao[linha][3], "%d", endereco_atual);
                        }
                        break;
                    }
                }
                if(!operando_encontrado){
                    // Adiciona operando como pendente na tabela de definicoes
                    tabela_definicao = realloc(tabela_definicao, sizeof(char **) * (linha + 2));
                    tabela_definicao[linha] = malloc(sizeof(char *) * 4);
                    tabela_definicao[linha][0] = strdup(token); // Rótulo
                    tabela_definicao[linha][1] = strdup("-1"); // Endereço
                    tabela_definicao[linha][2] = strdup("P"); // Pendente
                    tabela_definicao[linha][3] = malloc(5 * sizeof(char));
                    sprintf(tabela_definicao[linha][3], "%d", endereco_atual); // Última pendência
                    tabela_definicao[linha + 1] = NULL; // Marca o fim da tabela
                    codigo_objeto = realloc(codigo_objeto, sizeof(int) * (endereco_atual + 1));
                    codigo_objeto[endereco_atual] = -1; // Fim da lista de pendencias
                }
            }else{
                // Primeira vez que adiciona um operando pendente
                tabela_definicao = malloc(sizeof(char **) * 2);
                tabela_definicao[0] = malloc(sizeof(char *) * 4);
                tabela_definicao[0][0] = strdup(token); // Rótulo
                tabela_definicao[0][1] = strdup("-1"); // Endereço
                tabela_definicao[0][2] = strdup("P"); // Pendente
                tabela_definicao[0][3] = malloc(5 * sizeof(char));
                sprintf(tabela_definicao[0][3], "%d", endereco_atual); // Última pendência
                tabela_definicao[1] = NULL; // Marca o fim da tabela
                codigo_objeto = realloc(codigo_objeto, sizeof(int) * (endereco_atual + 1));
                codigo_objeto[endereco_atual] = -1; // Fim da lista de pendencias
            }
            endereco_atual++;
        }
        lin_atual++;
    }
    /*
    // imprimir o código objeto em um arquivo .o1
    printf("____________________ codigo objeto gerado ____________________\n");
    for(int i = 0; i < endereco_atual; i++){
        printf("%d\n", codigo_objeto[i]);
    }
        */
    imprimeArquivoObjeto(codigo_objeto, endereco_atual);
}

// Remover se houver mais de um espaco/tab/quebra de linha
char *limparEspaco(char *arquivo_pre){
    int i = 0, j = 0;
    int tamanho = strlen(arquivo_pre);
    char *arquivoLimpo = (char *) malloc(tamanho + 1);

    if(!arquivoLimpo) return NULL;

    // Verifica se o caractere anterior fazia parte de uma palavra
    int dentroDePalavra = 0;
    // Mantem as linhas só se preocupando em remover os espaços em excesso
    int ultimaFoiQuebra = 0;

    while(arquivo_pre[i] != '\0'){
        if(arquivo_pre[i] == ' ' || arquivo_pre[i] == '\t'){
            if(dentroDePalavra){
                arquivoLimpo[j++] = ' ';
                dentroDePalavra = 0;
            }
        }
        else if(arquivo_pre[i] == '\n' || arquivo_pre[i] == '\r'){
            if(!ultimaFoiQuebra){
                arquivoLimpo[j++] = '\n';
                ultimaFoiQuebra = 1;
                dentroDePalavra = 0;
            }
        }
        else{
            arquivoLimpo[j++] = arquivo_pre[i];
            dentroDePalavra = 1;
            ultimaFoiQuebra = 0;
        }
        i++;
    }

    // Remove espaço no final do arquivo, se tiver
    if(j > 0 && (arquivoLimpo[j - 1] == ' ' || arquivoLimpo[j - 1] == '\n'))
        j--;

    arquivoLimpo[j] = '\0';
    return arquivoLimpo;
}

char *abreArquivo(char *nomeArquivo){
    FILE *arquivo = fopen(nomeArquivo, "r");

    if(arquivo == NULL){
        perror("Erro ao abrir o arquivo");
        return NULL;
    }

    if(fseek(arquivo, 0, SEEK_END) != 0){
        perror("Erro ao buscar o final do arquivo");
        fclose(arquivo);
        return NULL;
    }
    long tamanho = ftell(arquivo);
    if(tamanho < 0){
        perror("Erro ao obter o tamanho do arquivo");
        fclose(arquivo);
        return NULL;
    }
    rewind(arquivo);

    char *conteudo = (char *)malloc(sizeof(char) * (tamanho + 1)); // Aloca memória para o conteúdo do arquivo
    if (conteudo == NULL){
        printf("Erro ao alocar memória\n");
        fclose(arquivo);
        return NULL;
    }

    size_t lidos = fread(conteudo, 1, (size_t)tamanho, arquivo);
    conteudo[lidos] = '\0'; // Adiciona o caractere nulo ao final da string
    fclose(arquivo);
    return conteudo;
}

void converterMinusculo(char *arquivo){
    for(int i = 0; arquivo[i] != '\0'; i++)
        arquivo[i] = tolower(arquivo[i]);
}

char ***separaTokens(char *arquivo_pre){

    int cont_car = 0;
    char ***tabela_tokens = malloc(sizeof(char **) * (MAX_LINHAS + 1)); // Rótulo, Operação, Operando1, Operando2
    char *linha_atual = malloc(1);
    char **linhas_arquivo = malloc(sizeof(char *) * (MAX_LINHAS + 1));
    int cont_lin = 0;
    int cont_car_lin = 0;

    // Separa as linhas
    do{
        linha_atual = realloc(linha_atual, sizeof(char) * (cont_car_lin + 1));
        if(arquivo_pre[cont_car] == '\n' || arquivo_pre[cont_car] == '\0'){
            linha_atual[cont_car_lin] = '\0';
            linhas_arquivo[cont_lin] = linha_atual;
            cont_lin++;
            linhas_arquivo = realloc(linhas_arquivo, sizeof(char *) * (cont_lin + 1));
            cont_car_lin = 0;
            linha_atual = malloc(1);
        }
        else{
            linha_atual[cont_car_lin] = arquivo_pre[cont_car];
            cont_car_lin++;
        }
    }while(arquivo_pre[cont_car++] != '\0');

    // Inicializa a tabela de tokens (cada linha tem 4 ponteiros)
    for(int i = 0; i <= cont_lin; i++){
        tabela_tokens[i] = malloc(sizeof(char *) * 4);
        for(int k = 0; k < 4; k++) tabela_tokens[i][k] = NULL;
    }
    tabela_tokens[cont_lin] = NULL; // Marca o fim da tabela de tokens

    // separa os tokens (forma simples e robusta usando strtok_r / strchr)
    for(int i = 0; i < cont_lin; i++){
        char *line = linhas_arquivo[i];
        if(line == NULL || line[0] == '\0') continue;

        char *copy = strdup(line);
        char *ptr = copy;

        // Verifica se há rótulo (':')
        char *label = NULL;
        char *colon = strchr(ptr, ':');
        if(colon){
            *colon = '\0';
            label = strdup(ptr);
            ptr = colon + 1;
        }

        // Trim inicial
        while(*ptr == ' ' || *ptr == '\t') ptr++;

        char *instr = NULL;
        char *op1 = NULL;
        char *op2 = NULL;

        char *saveptr;
        char *tok = strtok_r(ptr, " \t\n", &saveptr);
        if(tok) instr = strdup(tok);

        if(instr){
            // A partir do restante (saveptr) pegar operandos separados por ','
            char *rest = saveptr;
            if(rest){
                char *o1 = strtok_r(rest, ",\t\n", &saveptr);
                if(o1){
                    while(*o1 == ' ') o1++;
                    op1 = strdup(o1);
                    char *o2 = strtok_r(NULL, ",\t\n", &saveptr);
                    if(o2){
                        while(*o2 == ' ') o2++;
                        op2 = strdup(o2);
                    }
                }
            }
        }

        tabela_tokens[i][0] = label;
        tabela_tokens[i][1] = instr;
        tabela_tokens[i][2] = op1;
        tabela_tokens[i][3] = op2;

        free(copy);
    }

    return tabela_tokens;
}


int main(void){
    char *conteudo = abreArquivo("output.pre");
    if(conteudo == NULL){
        return 1;
    }
    printf("____________________ documento original ____________________\n");
    printf("%s\n\n", conteudo);

    converterMinusculo(conteudo);
    printf("____________________ documento em minusculo ____________________\n");
    printf("%s\n\n", conteudo);

    char *conteudoLimpo = limparEspaco(conteudo);
    free(conteudo);
    if(conteudoLimpo == NULL){
        return 1;
    }
    printf("____________________ documento limpo ____________________\n");
    printf("%s\n\n", conteudoLimpo);

    char ***tokens = separaTokens(conteudoLimpo);
    free(conteudoLimpo);
    if(tokens == NULL){
        printf("Erro ao separar os tokens\n");
        return 1;
    }
    printf("____________________ tokens separados ____________________\n");
    for(int i = 0; tokens[i] != NULL; i++){
        printf("Linha %d: ", i + 1);
        for(int j = 0; j < 4; j++){
            if(tokens[i][j] != NULL)
                printf("[%s] ", tokens[i][j]);
            else
                printf("[NULL] ");
        }
        printf("\n");
    }

    printf("\n");
    passagemUnica(tokens); // .o1 (pendências ainda não resolvidas)
    return 0;
}
