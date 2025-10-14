#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>

const char *tabela_intrucoes[14][2] = {
    {"ADD", "01"},
    {"SUB", "02"},
    {"MUL", "03"},
    {"DIV", "04"},
    {"JMP", "05"},
    {"JMPN", "06"},
    {"JMPP", "07"},
    {"JMPZ", "08"},
    {"COPY", "09"},
    {"LOAD", "10"},
    {"STORE", "11"},
    {"INPUT", "12"},
    {"OUTPUT", "13"},
    {"STOP", "14"}
};

// Primeira parte do programa(Algoritmo de uma passagem)

void primeiraPassagem(char *arquivo_pre){
    char *linhas_arquivo[100]; // Máximo de 100 linhas no arquivo

    int cont_pos = 0;
    int cont_lin = 0;
    char *rotulo = NULL, *operacao = NULL, *operando[2] = {NULL}, *comentario = NULL;
    char *string_aux = (char *)malloc(sizeof(char) * 1);
    char tabela_simbolos[100][2]; // Tabela de símbolos (rótulo, linha que está o rótulo)
    string_aux[0] = '\0';



    struct lista_erros
    {
        int linha;
        char *mensagem;
        char *tipo; // Léxico, sintático, semântico
    } lista_erros[100];

    int cont_erros = 0;

    FILE *f = fopen(arquivo_pre, "r");
    if(!f) return NULL;

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
    }

    cont_lin = 0;
    while(linhas_arquivo[cont_lin] != NULL){
        char *linha_atual = linhas_arquivo[cont_lin];
        cont_lin++;
        int n_operandos = 0;

        for(int i = 0; linha_atual[i] != '\0'; i++){
            char c = linha_atual[i];
        
            // Encontra comentário
            if(c == ';'){
                comentario = (char *)malloc(sizeof(char) * (strlen(linha_atual) - i + 1));
                strcpy(comentario, &linha_atual[i]);
                break;
            }

            //Encontra Operação
            else if((isspace(c) || c == '\n') && strlen(string_aux) > 0 && n_operandos == 0 && operacao == NULL){
                n_operandos = 1;
                operacao = (char *)malloc(sizeof(char) * (strlen(string_aux) + 1));
                strcpy(operacao, string_aux);
                free(string_aux);
                string_aux = (char *)malloc(sizeof(char) * 1);
                string_aux[0] = '\0';
            }

            // Encontra Operando
            else if((isspace(c) || c == '\n' || c == ',') && strlen(string_aux) > 0 && n_operandos > 0){
                if(operando[0] == NULL){
                    operando[0] = (char *)malloc(sizeof(char) * (strlen(string_aux) + 1));
                    strcpy(operando[0], string_aux);
                } else if(operando[1] == NULL){
                    operando[1] = (char *)malloc(sizeof(char) * (strlen(string_aux) + 1));
                    strcpy(operando[1], string_aux);
                }
                free(string_aux);
                string_aux = (char *)malloc(sizeof(char) * 1);
                string_aux[0] = '\0';
                n_operandos++;
            }

            // Encontra Rótulo
            else if(c == ':' && strlen(string_aux) > 0){
                rotulo = (char *)malloc(sizeof(char) * (strlen(string_aux) + 1));
                strcpy(rotulo, string_aux);
                free(string_aux);
                string_aux = (char *)malloc(sizeof(char) * 1);
                string_aux[0] = '\0';
            }

            // Incrementador de string
            else{
                string_aux = (char *) realloc(string_aux, sizeof(char) * (strlen(string_aux) + 2));
                strncat(string_aux, &c, 1);
            } 



            
            // Procurar rótulo na tabela de símbolos 
            int rotulo_tabela_simbolos = 0;
            for(int i = 0; i < 100; i++){
                // Adiciona erro de rótulo duplicado na lista de erros
                if(strcmp(tabela_simbolos[i][0], rotulo) == 0){
                    lista_erros[cont_erros].linha = cont_lin;
                    lista_erros[cont_erros].mensagem = (char *)malloc(sizeof(char) * 50);
                    lista_erros[cont_erros].tipo = "léxico";
                    sprintf(lista_erros[cont_erros].mensagem, "Erro linha %d, %s", cont_lin, lista_erros[cont_erros].tipo);
                    cont_erros++;
                    rotulo_tabela_simbolos = 1;
                    break;
                }
            }
            if(!rotulo_tabela_simbolos){
                // Adiciona o rótulo na tabela de símbolos
                for(int i = 0; i < 100; i++){
                    if(tabela_simbolos[i][0] == NULL){
                        tabela_simbolos[i][0] = rotulo;
                        tabela_simbolos[i][1] = cont_lin;
                        break;
                    }
                }
            }

/* ----------- Procura operação na tabela de instruções ----------- */

        }
    }
    
    fclose(f);
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
    FILE *arquivo;

    arquivo = fopen(nomeArquivo, "r");
    
    if(arquivo == NULL){
        printf("Erro ao abrir o arquivo\n");
        return NULL;
    }

    char *conteudo = (char *)malloc(sizeof(char) * 1000); // Aloca memória para o conteúdo do arquivo
    if(conteudo == NULL){
        printf("Erro ao alocar memória\n");
        return NULL;
    }
    int i = 0;
    char c;
    while((c = fgetc(arquivo)) != EOF){
        conteudo[i++] = c;
    }
    conteudo[i] = '\0'; // Adiciona o caractere nulo ao final da string
    fclose(arquivo);
    return conteudo;
}

void converterMinusculo(char *arquivo){
    for(int i = 0; arquivo[i] != '\0'; i++)
        arquivo[i] = tolower(arquivo[i]);
}

// Segunda parte do programa(Algoritmo de duas passagens) .o2


void segundaPassagem(char *arquivo_pre){
    // Lógica para o algoritmo
}

int main(void){
    
    char *conteudo = abreArquivo(nomeArquivo);
    if(conteudo == NULL){
        return 1;
    }

    converterMinusculo(conteudo);
    printf("%s\n", conteudo);
    free(conteudo);

    char *conteudoLimpo = limparEspaco(conteudo);
    printf("%s\n", conteudoLimpo);

    primeiraPassagem(conteudoLimpo); // .o1 (pendências ainda não resolvidas)
    return 0;
}