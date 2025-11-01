#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>

#define MAX_LINHAS 1000
#define MAX_OPCODES 14
const char *tabela_instrucoes[MAX_OPCODES] = {
    "add", "sub", "mult", "div", "jmp", "jmpn", "jmpp", "jmpz",
    "copy", "load", "store", "input", "output", "stop"
};
// NOVO: Tabela para contagem de operandos esperados
const int tabela_operandos_instrucoes[MAX_OPCODES] = {
    1, // add
    1, // sub
    1, // mult
    1, // div
    1, // jmp
    1, // jmpn
    1, // jmpp
    1, // jmpz
    2, // copy
    1, // load
    1, // store
    1, // input
    1, // output
    0  // stop
};
const char *tabela_diretivas[] = {
    "space", "const",
};


// =================================================================
// NOVO: SISTEMA DE TRATAMENTO DE ERROS
// =================================================================
typedef enum {
    ERRO_LEXICO,
    ERRO_SINTATICO,
    ERRO_SEMANTICO
} TipoErro;

typedef struct {
    int linha;
    TipoErro tipo;
    char *mensagem;
} Erro;

Erro *lista_de_erros = NULL;
int num_erros = 0;
int parar_compilacao = 0; // Flag para erros fatais (ex: malloc)

/**
 * @brief Adiciona um novo erro à lista global de erros.
 */
void adicionarErro(int linha, TipoErro tipo, const char *mensagem) {
    num_erros++;
    lista_de_erros = realloc(lista_de_erros, sizeof(Erro) * num_erros);
    if (!lista_de_erros) {
        printf("Erro fatal: Falha ao alocar memoria para lista de erros.\n");
        parar_compilacao = 1;
        return;
    }
    lista_de_erros[num_erros - 1].linha = linha;
    lista_de_erros[num_erros - 1].tipo = tipo;
    lista_de_erros[num_erros - 1].mensagem = strdup(mensagem);
}

/**
 * @brief Converte o enum TipoErro para uma string legível.
 */
const char* tipoErroParaString(TipoErro tipo) {
    switch (tipo) {
        case ERRO_LEXICO: return "Lexico";
        case ERRO_SINTATICO: return "Sintatico";
        case ERRO_SEMANTICO: return "Semantico";
        default: return "Desconhecido";
    }
}

/**
 * @brief Imprime todos os erros coletados.
 */
void imprimirErros() {
    if (num_erros == 0) {
        printf("Compilacao bem-sucedida, sem erros.\n");
        return;
    }
    printf("\n--- Relatorio de Erros Encontrados ---\n");
    for (int i = 0; i < num_erros; i++) {
        printf("Erro linha %d, %s: %s\n",
               lista_de_erros[i].linha,
               tipoErroParaString(lista_de_erros[i].tipo),
               lista_de_erros[i].mensagem);
        free(lista_de_erros[i].mensagem); // Libera a mensagem duplicada
    }
    free(lista_de_erros); // Libera a lista de erros
    lista_de_erros = NULL;
    num_erros = 0;
}

// =================================================================
// NOVO: FUNÇÕES DE LIMPEZA DE MEMÓRIA
// =================================================================

void liberarTabelaTokens(char ***tabela_tokens) {
    if (!tabela_tokens) return;
    for (int i = 0; tabela_tokens[i] != NULL; i++) {
        for (int j = 0; j < 4; j++) {
            free(tabela_tokens[i][j]); // Libera cada token (string)
        }
        free(tabela_tokens[i]); // Libera a linha (array de ponteiros)
    }
    free(tabela_tokens); // Libera a tabela (array de linhas)
}

void liberarTabelaDefinicao(char ***tabela_definicao) {
    if (!tabela_definicao) return;
    // Agora a tabela tem 5 colunas
    for (int i = 0; tabela_definicao[i] != NULL; i++) {
        for (int j = 0; j < 5; j++) {
            free(tabela_definicao[i][j]);
        }
        free(tabela_definicao[i]);
    }
    free(tabela_definicao);
}

// =================================================================
// CÓDIGO EXISTENTE (com modificações)
// =================================================================

void imprimeArquivoObjeto(int *codigo_objeto, int tamanho, char *nome_arquivo){
    // (Código original sem alterações)
    FILE *arquivo_objeto = fopen(nome_arquivo, "w");
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

void resolvePendencias(char *rotulo, int endereco, char* ultima_pendencia , int *codigo_objeto){
    // (Código original sem alterações)
    int pendencia = atoi(ultima_pendencia);
    int proxima_pendencia;
    while(pendencia != -1){
        proxima_pendencia = codigo_objeto[pendencia];
        codigo_objeto[pendencia] = endereco;
        pendencia = proxima_pendencia;
    }
}

/**
 * @brief NOVO: Verifica a tabela de definições após a passagem única
 * para encontrar rótulos não declarados.
 */
void checarPendencias(char ***tabela_definicao) {
    if (!tabela_definicao) return;
    for (int linha = 0; tabela_definicao[linha]; linha++) {
        // Se o status ainda é "Pendente"
        if (strcmp(tabela_definicao[linha][2], "P") == 0) {
            char msg[100];
            sprintf(msg, "Rotulo nao declarado: '%s'.", tabela_definicao[linha][0]);
            // Usa a linha da primeira ocorrência (armazenada na coluna 4)
            int linha_erro = atoi(tabela_definicao[linha][4]);
            adicionarErro(linha_erro, ERRO_SEMANTICO, msg);
        }
    }
}

// Passagem única
// MODIFICADO: Retorna a tabela de definições para checagem final
char ***passagemUnica(char ***tabela_tokenizada){
    // Variáveis
    char *token = NULL, ***tabela_definicao = NULL;
    int * codigo_objeto1 = NULL, *codigo_objeto2 = NULL;
    int lin_atual = 0, rotulo_definido = 0, linha = 0, endereco_atual = 0, end_encontrado = 0;
    int operando_encontrado = 0;
    char *op_space, *op_const;
    char msg_erro[100]; // Buffer para mensagens de erro

    // Percorre linha a linha pegando os tokens
    while (tabela_tokenizada[lin_atual]){
        if (parar_compilacao) break; // Para se houver erro fatal de memória

        token = tabela_tokenizada[lin_atual][0];
        // Verifica se o token (rótulo) existe
        if (token && tabela_definicao){
            //Procura na tabela de definições
            rotulo_definido = 0;
            for(linha = 0; tabela_definicao[linha]; linha++){
                if(strcmp(tabela_definicao[linha][0], token) == 0){
                    // MODIFICADO: Padronizado para 'D' (Definido)
                    if(strcmp(tabela_definicao[linha][2], "P") == 0){
                        // Rótulo pendente, agora definido
                        free(tabela_definicao[linha][2]); // Libera "P"
                        tabela_definicao[linha][2] = strdup("D"); // Definido
                        free(tabela_definicao[linha][1]); // Libera "-1"
                        tabela_definicao[linha][1] = malloc(5 * sizeof(char));
                        sprintf(tabela_definicao[linha][1], "%d", endereco_atual); // Endereço
                        rotulo_definido = 1;
                        resolvePendencias(token, endereco_atual, tabela_definicao[linha][3], codigo_objeto2);
                        break;
                    }else if(strcmp(tabela_definicao[linha][2], "D") == 0){
                        // MODIFICADO: Rótulo já existe (Erro Semântico)
                        rotulo_definido = 1;
                        sprintf(msg_erro, "Rotulo '%s' declarado duas vezes (redefinicao).", token);
                        adicionarErro(lin_atual + 1, ERRO_SEMANTICO, msg_erro);
                        break;
                    }
                }
            }
            if(!rotulo_definido){
                // Adiciona o rótulo na tabela de definições
                // MODIFICADO: Tabela agora tem 5 colunas
                tabela_definicao = realloc(tabela_definicao, sizeof(char **) * (linha + 2));
                tabela_definicao[linha] = malloc(sizeof(char *) * 5); // 0:Rotulo, 1:End, 2:Status, 3:Pend, 4:LinhaErro
                tabela_definicao[linha][0] = strdup(token); // Rótulo
                tabela_definicao[linha][1] = malloc(5 * sizeof(char));
                sprintf(tabela_definicao[linha][1], "%d", endereco_atual); // Endereço
                tabela_definicao[linha][2] = strdup("D"); // Definido
                tabela_definicao[linha][3] = strdup("-1"); // Última pendência
                tabela_definicao[linha][4] = strdup("-1"); // Linha do erro (não aplicável)
                tabela_definicao[linha + 1] = NULL; // Marca o fim da tabela
            }
        }else if (token && !tabela_definicao){
            // Primeira vez que adiciona um rótulo
            // MODIFICADO: Tabela agora tem 5 colunas
            tabela_definicao = malloc(sizeof(char **) * 2);
            tabela_definicao[0] = malloc(sizeof(char *) * 5);
            tabela_definicao[0][0] = strdup(token); // Rótulo
            tabela_definicao[0][1] = malloc(5 * sizeof(char));
            sprintf(tabela_definicao[0][1], "%d", endereco_atual); // Endereço
            tabela_definicao[0][2] = strdup("D"); // Definido
            tabela_definicao[0][3] = strdup("-1"); // Última pendência
            tabela_definicao[0][4] = strdup("-1"); // Linha do erro (não aplicável)
            tabela_definicao[1] = NULL; // Marca o fim da tabela
        }


        // Verifica a instrução
        token = tabela_tokenizada[lin_atual][1];
        if (token == NULL) { // Linha só com rótulo (ex: "LOOP:")
            lin_atual++;
            continue;
        }

        end_encontrado = 0;
        for(int i = 0; i < MAX_OPCODES; i++){
            if(strcmp(token, tabela_instrucoes[i]) == 0){
                // Instrução encontrada
                codigo_objeto1 = realloc(codigo_objeto1, sizeof(int) * (endereco_atual + 1));
                codigo_objeto1[endereco_atual] = i+1;
                codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
                codigo_objeto2[endereco_atual] = i+1;
                endereco_atual++;
                end_encontrado = 1;

                // NOVO: Checagem de número de operandos (Erro Sintático)
                int esperados = tabela_operandos_instrucoes[i];
                char *op1 = tabela_tokenizada[lin_atual][2];
                char *op2 = tabela_tokenizada[lin_atual][3];
                int fornecidos = (op1 ? 1 : 0) + (op2 ? 1 : 0);

                if (esperados == 0 && fornecidos > 0) {
                    sprintf(msg_erro, "Instrucao '%s' nao espera operandos, mas %d foram fornecidos.", token, fornecidos);
                    adicionarErro(lin_atual + 1, ERRO_SINTATICO, msg_erro);
                } else if (esperados == 1 && fornecidos != 1) {
                    sprintf(msg_erro, "Instrucao '%s' espera 1 operando, mas %d foram fornecidos.", token, fornecidos);
                    adicionarErro(lin_atual + 1, ERRO_SINTATICO, msg_erro);
                } else if (esperados == 2 && fornecidos != 2) {
                    sprintf(msg_erro, "Instrucao '%s' espera 2 operandos, mas %d foram fornecidos.", token, fornecidos);
                    adicionarErro(lin_atual + 1, ERRO_SINTATICO, msg_erro);
                }
                break;
            }
        }
        if(!end_encontrado && strcmp(token, "const") == 0){
            // Diretiva CONST
            // NOVO: Checagem de operandos
            if (tabela_tokenizada[lin_atual][2] == NULL || tabela_tokenizada[lin_atual][3] != NULL) {
                adicionarErro(lin_atual + 1, ERRO_SINTATICO, "Diretiva 'const' espera exatamente 1 operando (o valor).");
            }
            
            codigo_objeto1 = realloc(codigo_objeto1, sizeof(int) * (endereco_atual + 1));
            codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
            int valor_const = (tabela_tokenizada[lin_atual][2] ? atoi(tabela_tokenizada[lin_atual][2]) : 0);
            codigo_objeto1[endereco_atual] = valor_const;
            codigo_objeto2[endereco_atual] = valor_const;
            endereco_atual++;
            end_encontrado = 1; // Marca como "encontrado" para não cair no erro de instrução inválida
        }else if(!end_encontrado && strcmp(token, "space") == 0){
            // Diretiva SPACE
            // NOVO: Checagem de operandos
            if (tabela_tokenizada[lin_atual][3] != NULL) {
                 adicionarErro(lin_atual + 1, ERRO_SINTATICO, "Diretiva 'space' espera 0 ou 1 operando.");
            }
            op_space = tabela_tokenizada[lin_atual][2];
            int quantidade = 1;
            if(op_space != NULL){
                int valor = atoi(op_space);
                if(valor > 0) quantidade = valor;
            }
            for(int i = 0; i < quantidade; i++){
                codigo_objeto1 = realloc(codigo_objeto1, sizeof(int) * (endereco_atual + 1));
                codigo_objeto1[endereco_atual] = 0;
                codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
                codigo_objeto2[endereco_atual] = 0;
                endereco_atual++;
            }
            end_encontrado = 1; // Marca como "encontrado"
        }else if(!end_encontrado){
            // MODIFICADO: Erro: Instrução inválida (Erro Semântico)
            sprintf(msg_erro, "Instrucao ou diretiva invalida: '%s'.", token);
            adicionarErro(lin_atual + 1, ERRO_LEXICO, msg_erro);
        }


        // Verifica os operandos (Rótulos)
        for(int i = 2; i <=3; i++){
            if(tabela_tokenizada[lin_atual][i] == NULL) continue;
            // Pula operandos de diretivas (já tratados)
            if(strcmp(tabela_tokenizada[lin_atual][1], "const") == 0 || strcmp(tabela_tokenizada[lin_atual][1], "space") == 0) continue;

            operando_encontrado = 0;
            token = tabela_tokenizada[lin_atual][i];
            
            // Tenta converter para número (ex: ADD 5). Se for, não é um rótulo.
            char *endptr;
            strtol(token, &endptr, 10);
            if (*endptr == '\0') {
                // É um número válido, não um rótulo.
                // Erro semântico? Instruções (exceto CONST) deveriam usar rótulos?
                // Assumindo que a arquitetura não suporta "immediate addressing"
                sprintf(msg_erro, "Operando '%s' invalido. Esperava-se um rotulo.", token);
                adicionarErro(lin_atual + 1, ERRO_SEMANTICO, msg_erro);
                // Mesmo com erro, aloca espaço para o número
                codigo_objeto1 = realloc(codigo_objeto1, sizeof(int) * (endereco_atual + 1));
                codigo_objeto1[endereco_atual] = atoi(token);
                codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
                codigo_objeto2[endereco_atual] = atoi(token);
                endereco_atual++;
                continue; // Pula para o próximo operando
            }


            if(tabela_definicao){
                // Procura na tabela de definicoes
                for(linha = 0; tabela_definicao[linha]; linha++){
                    op_const = tabela_definicao[linha][0];
                    if(strcmp(op_const, token) == 0){
                        // Operando encontrado
                        operando_encontrado = 1;
                        if(strcmp(tabela_definicao[linha][2], "D") == 0){ // MODIFICADO: "S" -> "D"
                            // Rótulo já definido
                            codigo_objeto1 = realloc(codigo_objeto1, sizeof(int) * (endereco_atual + 1));
                            codigo_objeto1[endereco_atual] = atoi(tabela_definicao[linha][1]);
                            codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
                            codigo_objeto2[endereco_atual] = atoi(tabela_definicao[linha][1]);
                        }else{
                            // Rótulo ainda não definido ("P")
                            int ultimo_endereco_pendente = atoi(tabela_definicao[linha][3]);
                            codigo_objeto1 = realloc(codigo_objeto1, sizeof(int) * (endereco_atual + 1));
                            codigo_objeto1[endereco_atual] = ultimo_endereco_pendente;
                            codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
                            codigo_objeto2[endereco_atual] = ultimo_endereco_pendente;
                            
                            free(tabela_definicao[linha][3]); // Libera pendencia antiga
                            tabela_definicao[linha][3] = malloc(5 * sizeof(char));
                            sprintf(tabela_definicao[linha][3], "%d", endereco_atual);
                        }
                        break;
                    }
                }
                if(!operando_encontrado){
                    // Adiciona operando como pendente na tabela de definicoes
                    // MODIFICADO: 5 colunas
                    tabela_definicao = realloc(tabela_definicao, sizeof(char **) * (linha + 2));
                    tabela_definicao[linha] = malloc(sizeof(char *) * 5);
                    tabela_definicao[linha][0] = strdup(token); // Rótulo
                    tabela_definicao[linha][1] = strdup("-1"); // Endereço
                    tabela_definicao[linha][2] = strdup("P"); // Pendente
                    tabela_definicao[linha][3] = malloc(5 * sizeof(char));
                    sprintf(tabela_definicao[linha][3], "%d", endereco_atual); // Última pendência
                    // NOVO: Armazena a linha do erro
                    tabela_definicao[linha][4] = malloc(5 * sizeof(char));
                    sprintf(tabela_definicao[linha][4], "%d", lin_atual + 1); // Linha da primeira pendencia
                    tabela_definicao[linha + 1] = NULL;
                    
                    codigo_objeto1 = realloc(codigo_objeto1, sizeof(int) * (endereco_atual + 1));
                    codigo_objeto1[endereco_atual] = -1; // Fim da lista de pendencias
                    codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
                    codigo_objeto2[endereco_atual] = -1; // Fim da lista
                }
            }else{
                // Primeira vez que adiciona um operando pendente
                // MODIFICADO: 5 colunas
                tabela_definicao = malloc(sizeof(char **) * 2);
                tabela_definicao[0] = malloc(sizeof(char *) * 5);
                tabela_definicao[0][0] = strdup(token); // Rótulo
                tabela_definicao[0][1] = strdup("-1"); // Endereço
                tabela_definicao[0][2] = strdup("P"); // Pendente
                tabela_definicao[0][3] = malloc(5 * sizeof(char));
                sprintf(tabela_definicao[0][3], "%d", endereco_atual); // Última pendência
                // NOVO: Armazena a linha do erro
                tabela_definicao[0][4] = malloc(5 * sizeof(char));
                sprintf(tabela_definicao[0][4], "%d", lin_atual + 1); // Linha da primeira pendencia
                tabela_definicao[1] = NULL;
                
                codigo_objeto1 = realloc(codigo_objeto1, sizeof(int) * (endereco_atual + 1));
                codigo_objeto1[endereco_atual] = -1;
                codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
                codigo_objeto2[endereco_atual] = -1;
            }
            endereco_atual++;
        }
        lin_atual++;
    }

    // imprimir os arquivos objeto
    imprimeArquivoObjeto(codigo_objeto1, endereco_atual, "output.o1");
    imprimeArquivoObjeto(codigo_objeto2, endereco_atual, "output.o2");

    // Libera a memória dos vetores de código objeto
    free(codigo_objeto1);
    free(codigo_objeto2);

    return tabela_definicao; // Retorna a tabela para checagem final
}

// Remover se houver mais de um espaco/tab/quebra de linha
char *limparEspaco(char *arquivo_pre){
    // (Código original sem alterações)
    int i = 0, j = 0;
    int tamanho = strlen(arquivo_pre);
    char *arquivoLimpo = (char *) malloc(tamanho + 1);

    if(!arquivoLimpo) return NULL;

    int dentroDePalavra = 0;
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
    if(j > 0 && (arquivoLimpo[j - 1] == ' ' || arquivoLimpo[j - 1] == '\n'))
        j--;

    arquivoLimpo[j] = '\0';
    return arquivoLimpo;
}

char *abreArquivo(char *nomeArquivo){
    // (Código original sem alterações)
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

    char *conteudo = (char *)malloc(sizeof(char) * (tamanho + 1)); 
    if (conteudo == NULL){
        printf("Erro ao alocar memória\n");
        fclose(arquivo);
        return NULL;
    }

    size_t lidos = fread(conteudo, 1, (size_t)tamanho, arquivo);
    conteudo[lidos] = '\0';
    fclose(arquivo);
    return conteudo;
}

void converterMinusculo(char *arquivo){
    // (Código original sem alterações)
    for(int i = 0; arquivo[i] != '\0'; i++)
        arquivo[i] = tolower(arquivo[i]);
}

char ***separaTokens(char *arquivo_pre){
    // (Código original com modificações para liberar memória e checar erros)

    int cont_car = 0;
    char ***tabela_tokens = malloc(sizeof(char **) * (MAX_LINHAS + 1));
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
    free(linha_atual); // Libera a última alocação extra
    linhas_arquivo[cont_lin] = NULL; // Marca o fim do array de linhas

    // Inicializa a tabela de tokens
    for(int i = 0; i <= cont_lin; i++){
        tabela_tokens[i] = malloc(sizeof(char *) * 4);
        for(int k = 0; k < 4; k++) tabela_tokens[i][k] = NULL;
    }
    tabela_tokens[cont_lin] = NULL;

    // separa os tokens
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

            // NOVO: Checagem Lexical do Rótulo
            if (isdigit(label[0])) {
                adicionarErro(i + 1, ERRO_LEXICO, "Rotulo nao pode comecar com numero.");
            } else {
                for (int k = 0; label[k] != '\0'; k++) {
                    if (!isalnum(label[k]) && label[k] != '_') {
                        adicionarErro(i + 1, ERRO_LEXICO, "Rotulo contem caractere invalido. Permitido apenas letras, numeros e '_'.");
                        break;
                    }
                }
            }
        }

        // Trim inicial
        while(*ptr == ' ' || *ptr == '\t') ptr++;

        // NOVO: Checagem de dois rótulos (Erro Sintático)
        if (*ptr == ':') {
            adicionarErro(i + 1, ERRO_SINTATICO, "Dois rotulos na mesma linha.");
            // Tenta se recuperar pulando o segundo rótulo
            char *segundo_colon = strchr(ptr + 1, ':');
            if (segundo_colon) {
                ptr = segundo_colon + 1;
            } else {
                ptr = ptr + 1; // Pula o ':'
            }
            while(*ptr == ' ' || *ptr == '\t') ptr++;
        }

        char *instr = NULL;
        char *op1 = NULL;
        char *op2 = NULL;

        char *saveptr;
        char *tok = strtok_r(ptr, " \t\n", &saveptr);
        if(tok) instr = strdup(tok);

        if(instr){
            char *rest = saveptr;
            if(rest){
                char *o1 = strtok_r(rest, ",\t\n", &saveptr);
                if(o1){
                    while(*o1 == ' ') o1++;
                    if (*o1 != '\0') op1 = strdup(o1); // Evita duplicar strings vazias
                }
                char *o2 = strtok_r(NULL, ",\t\n", &saveptr);
                if(o2){
                    while(*o2 == ' ') o2++;
                    if (*o2 != '\0') op2 = strdup(o2); // Evita duplicar strings vazias
                }
            }
        }

        tabela_tokens[i][0] = label;
        tabela_tokens[i][1] = instr;
        tabela_tokens[i][2] = op1;
        tabela_tokens[i][3] = op2;

        free(copy);
    }

    // MODIFICADO: Libera o array de linhas temporário
    for(int i = 0; i < cont_lin; i++) {
        free(linhas_arquivo[i]);
    }
    free(linhas_arquivo);

    return tabela_tokens;
}


int main(void){
    char *conteudo = abreArquivo("saida.pre");
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
    // MODIFICADO: Captura a tabela de definições
    char ***tabela_defs = passagemUnica(tokens);

    // NOVO: Checagem final de erros e impressão
    checarPendencias(tabela_defs);
    imprimirErros();

    // NOVO: Liberação da memória
    liberarTabelaTokens(tokens);
    liberarTabelaDefinicao(tabela_defs);

    return 0;
}