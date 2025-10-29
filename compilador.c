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
    char ***tabela_tokens = calloc(MAX_LINHAS, sizeof(char **));
    if(!tabela_tokens) return NULL;
    
    FILE *f = fopen(arquivo_pre, "r");
    if(!f) return NULL;

    char *linhas_arquivo[100]; // Máximo de 100 linhas no arquivo
    int cont_lin = 0;

    while(fgetc(f) != EOF){
        fseek(f, -1, SEEK_CUR);
        char *linha = (char *)malloc(sizeof(char) * 1000); // Máximo de 1000 caracteres por linha
        if(!linha) return NULL;

        int i = 0;
        char c;
        while((c = fgetc(f)) != '\n' && c != EOF){
            linha[i++] = c;
        }
        linha[i] = '\0';
        linhas_arquivo[cont_lin] = linha;
        cont_lin++;

        // Remove os comentarios
        char *comentario = strchr(linha, ';');
        if(comentario) *comentario = '\0';
    }

    char *rotuloPendente = NULL;

    for(int i = 0; i < cont_lin; i++){
        tabela_tokens[i] = malloc(sizeof(char *) * 4);
        
        for(int j = 0; j < 4; j++){
            tabela_tokens[i][j] = NULL;
        }
        
        char *linha_atual = linhas_arquivo[i];
        char *doisPontos = strchr(linha_atual, ':');
        char *resto = linha_atual;

        if(doisPontos){
            *doisPontos = '\0';
            char *rotulo = linha_atual;
            
            while(strlen(rotulo) > 0 && (rotulo[strlen(rotulo) - 1] == ' ' || rotulo[strlen(rotulo) - 1] == '\t')){
                rotulo[strlen(rotulo) - 1] = '\0';
            }
            
            resto = doisPontos + 1;
            while(*resto == ' ' || *resto == '\t') resto++;

            if(*resto == '\0'){
                if(rotuloPendente) free(rotuloPendente);
                rotuloPendente = strdup(rotulo);
                free(linhas_arquivo[i]);
                free(tabela_tokens[i]);
                continue;
            }
            else{
                tabela_tokens[i][0] = strdup(rotulo);
            }
        } 
        else if(rotuloPendente){
            tabela_tokens[i][0] = rotuloPendente;
            rotuloPendente = NULL;
        }

        char *token = strtok(resto, ",\t\n");
        if(token) tabela_tokens[i][1] = strdup(token);

        token = strtok(NULL, ",\t\n");
        if(token) tabela_tokens[i][2] = strdup(token);

        token = strtok(NULL, ",\t\n");
        if(token) tabela_tokens[i][3] = strdup(token);
        
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

    converterMinusculo(conteudo);
    printf("%s\n", conteudo);

    char *conteudoLimpo = limparEspaco(conteudo);
    free(conteudo);
    if(conteudoLimpo == NULL){
        return 1;
    }
    printf("%s\n", conteudoLimpo);

    char ***tokens = separaTokens(conteudoLimpo);
    passagemUnica(tokens); // .o1 (pendências ainda não resolvidas)
    return 0;
}
