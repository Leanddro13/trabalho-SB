#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>

#define MAX_LINHAS 1000
#define MAX_OPCODES 14
const char *tabela_intrucoes[MAX_OPCODES][2] = {
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
const char *tabela_diretivas[] = {
    "SPACE",
    "CONST",
};

char ***separaTokens(char *arquivo_pre){

    int cont_car = 0;
    int tamanho = strlen(arquivo_pre);
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

void imprimeArquivoObjeto(int *codigo_objeto, int tamanho){
    FILE *arquivo_objeto = fopen("output.o1", "w");
    if(!arquivo_objeto){
        printf("Erro ao criar o arquivo objeto\n");
        return;
    }
    for(int i = 0; i < tamanho; i++){
        fprintf(arquivo_objeto, "%d\n", codigo_objeto[i]);
    }
    fclose(arquivo_objeto);
}

// Passagem única
void passagemUnica(char ***tabela_tokenizada){
    char *rotulo, *instrucao, *operando1, *operando2;
    char *tabela_simbolos[MAX_LINHAS][4]; // Rótulo, Endereço(-1 == ), foi definido, ultimo endereco pendente (Enquanto não for -1, existe endereço pendente)
    int codigo_objeto[1000];
    int endereco_atual = 0;
    // Percorrer linha a linha da minha tabela tokenizada
    for(int i = 0; tabela_tokenizada[i] != NULL; i++){
        rotulo = tabela_tokenizada[i][0];
        instrucao = tabela_tokenizada[i][1];
        operando1 = tabela_tokenizada[i][2];
        operando2 = tabela_tokenizada[i][3];

        // Verificar se existe rótulo
        if (rotulo != NULL){
            // Verifica se o rótulo já existe na tabela de símbolos
            int rotulo_existe = 0;
            for(int j = 0; j < MAX_LINHAS; j++){
                if(tabela_simbolos[j][0] != NULL && strcmp(tabela_simbolos[j][0], rotulo) == 0){
                    // Rótulo já existe
                    printf("Erro: rótulo '%s' já definido\n", rotulo);
                    rotulo_existe = 1;
                    break;
                }
            }
            // Adicionar o rótulo na tabela de símbolos
            if(!rotulo_existe){
                for(int j = 0; j < MAX_LINHAS; j++){
                    if(tabela_simbolos[j][0] == NULL){
                        tabela_simbolos[j][0] = strdup(rotulo);
                        tabela_simbolos[j][1] = malloc(10 * sizeof(char));
                        sprintf(tabela_simbolos[j][1], "%d", endereco_atual);
                        tabela_simbolos[j][2] = strdup("SIM");
                        tabela_simbolos[j][3] = strdup("-1");
                        break;
                    }
                }
            }
        }
        
        // Verificar qual a instrução
        int endereco_encontrado = 0;
        for(int j = 0; j < MAX_OPCODES; j++){
            if(strcmp(instrucao, tabela_intrucoes[j][0]) == 0){
                // Instrução encontrada
                codigo_objeto[endereco_atual] = atoi(tabela_intrucoes[j][1]);
                endereco_atual++;
                endereco_encontrado = 1;
                break;
            }
        }
        if(!endereco_encontrado && strcmp(instrucao, "CONST") == 0){
            // Diretiva CONST
            codigo_objeto[endereco_atual] = atoi(operando1); // Valor constante
            endereco_atual++;
        }else if(!endereco_encontrado && strcmp(instrucao, "SPACE") == 0){
            // Diretiva SPACE
            if (operando1 != NULL && atoi(operando1) > 0){
                for(int j = 0; j < atoi(operando1); j++){
                    codigo_objeto[endereco_atual] = 0; // Espaço reservado
                    endereco_atual++;
                }
            }else if(operando1 == NULL){
                codigo_objeto[endereco_atual] = 0; // Espaço reservado
                endereco_atual++;
            }else{
                // Erro: Operando inválido para SPACE
                printf("Erro: Operando inválido para SPACE '%s'\n", operando1);
            }
        }else{
            // Erro: Instrução inválida
            printf("Erro: Instrução inválida '%s'\n", instrucao);
        }

        // Tratar operandos (se existirem)
        if(operando1 != NULL && (strcmp(instrucao, "CONST") != 0) && (strcmp(instrucao, "SPACE") != 0)){
            // Verificar se o operando1 é um rótulo
            int operando_encontrado = 0;
            for(int j = 0; j < MAX_LINHAS && tabela_simbolos[j][0] != NULL; j++){
                if(strcmp(tabela_simbolos[j][0], operando1) == 0){
                    // Operando1 encontrado na tabela de símbolos
                    operando_encontrado = 1;

                    if(strcmp(tabela_simbolos[j][2], "SIM") == 0){
                        // Rótulo já definido
                        codigo_objeto[endereco_atual] = atoi(tabela_simbolos[j][1]);

                    }else{
                        // Rótulo ainda não definido
                        // adiciona último endereço pendente no endereço atual e atualiza o endereço pendente
                        int ultimo_endereco_pendente = atoi(tabela_simbolos[j][3]);
                        codigo_objeto[endereco_atual] = ultimo_endereco_pendente;
                        tabela_simbolos[j][3] = malloc(10 * sizeof(char));
                        sprintf(tabela_simbolos[j][3], "%d", endereco_atual);

                    }
                    break;
                }else if(tabela_simbolos[j][0] == NULL){
                    break;
                }
            }
            if(!operando_encontrado){
                // Adicionar operando1 como pendente na tabela de símbolos
                for(int j = 0; j < MAX_LINHAS; j++){
                    if(tabela_simbolos[j][0] == NULL){
                        // Encontrou uma linha vazia na tabela de símbolos
                        tabela_simbolos[j][0] = strdup(operando1);
                        tabela_simbolos[j][1] = strdup("-1");
                        tabela_simbolos[j][2] = strdup("PEND");
                        tabela_simbolos[j][3] = malloc(10 * sizeof(char));
                        sprintf(tabela_simbolos[j][3], "%d", endereco_atual);
                        break;
                    }
                }
                codigo_objeto[endereco_atual] = -1; // Fim da lista de pendencias
            }
            endereco_atual++;
        }


        if(operando2 != NULL && (strcmp(instrucao, "CONST") != 0) && (strcmp(instrucao, "SPACE") != 0)){
            // Verificar se o operando2 é um rótulo
            int operando_encontrado = 0;
            for(int j = 0; j < MAX_LINHAS && tabela_simbolos[j][0] != NULL; j++){
                if(strcmp(tabela_simbolos[j][0], operando2) == 0){
                    // Operando1 encontrado na tabela de símbolos
                    operando_encontrado = 1;

                    if(strcmp(tabela_simbolos[j][2], "SIM") == 0){
                        // Rótulo já definido
                        codigo_objeto[endereco_atual] = atoi(tabela_simbolos[j][1]);

                    }else{
                        // Rótulo ainda não definido
                        // adiciona último endereço pendente no endereço atual e atualiza o endereço pendente
                        int ultimo_endereco_pendente = atoi(tabela_simbolos[j][3]);
                        codigo_objeto[endereco_atual] = ultimo_endereco_pendente;
                        tabela_simbolos[j][3] = malloc(10 * sizeof(char));
                        sprintf(tabela_simbolos[j][3], "%d", endereco_atual);

                    }
                    break;
                }else if(tabela_simbolos[j][0] == NULL){
                    break;
                }
            }
            if(!operando_encontrado){
                // Adicionar operando2 como pendente na tabela de símbolos
                for(int j = 0; j < MAX_LINHAS; j++){
                    if(tabela_simbolos[j][0] == NULL){
                        // Encontrou uma linha vazia na tabela de símbolos
                        tabela_simbolos[j][0] = strdup(operando2);
                        tabela_simbolos[j][1] = strdup("-1");
                        tabela_simbolos[j][2] = strdup("PEND");
                        tabela_simbolos[j][3] = malloc(10 * sizeof(char));
                        sprintf(tabela_simbolos[j][3], "%d", endereco_atual);
                        break;
                    }
                }
                codigo_objeto[endereco_atual] = -1; // Fim da lista de pendencias
            }
            endereco_atual++;
        }
    }
    // imprimir o código objeto em um arquivo .o1
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
/*
    passagemUnica(tokens); // .o1 (pendências ainda não resolvidas)
    return 0;
    */
}
