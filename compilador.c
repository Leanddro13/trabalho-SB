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
// Tabela para contagem de operandos esperados

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

// Sistema de tratamento de erros

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
    if (parar_compilacao) return; // Evita adicionar erro se a alocação falhou
    num_erros++;
    Erro *temp = realloc(lista_de_erros, sizeof(Erro) * num_erros);
    if (!temp) {
        printf("Erro fatal: Falha ao alocar memoria para lista de erros.\n");
        parar_compilacao = 1;
        num_erros--; // Reverte a contagem
        return;
    }
    lista_de_erros = temp;
    lista_de_erros[num_erros - 1].linha = linha;
    lista_de_erros[num_erros - 1].tipo = tipo;
    lista_de_erros[num_erros - 1].mensagem = strdup(mensagem);
    if (!lista_de_erros[num_erros - 1].mensagem) {
         printf("Erro fatal: Falha ao duplicar mensagem de erro.\n");
         parar_compilacao = 1;
    }
}

/**
 * @brief Converte o enum TipoErro para uma string legível (usado no relatório).
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
 * @brief Converte o enum TipoErro para o prefixo desejado no arquivo.
 */
const char* tipoErroParaPrefixo(TipoErro tipo) {
    switch (tipo) {
        case ERRO_LEXICO: return "<ERRO LÉXICO> ";
        case ERRO_SINTATICO: return "<ERRO SINTÁTICO> ";
        case ERRO_SEMANTICO: return "<ERRO SEMÂNTICO> ";
        default: return "<ERRO DESCONHECIDO> ";
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
        // A liberação da mensagem agora é feita em 'liberarErros'
    }
}

/**
 * @brief Libera a memória alocada para a lista de erros.
 */
void liberarErros() {
    for (int i = 0; i < num_erros; i++) {
        free(lista_de_erros[i].mensagem);
    }
    free(lista_de_erros);
    lista_de_erros = NULL;
    num_erros = 0;
}


// Funções escrita do arquivo com anotação de erros

/**
 * @brief Escreve o arquivo de origem com os prefixos de erro nas linhas correspondentes.
 * @param nome_arquivo_origem Nome do arquivo de origem ("saida.pre").
 * @param nome_arquivo_destino Nome do arquivo anotado ("saida_anotada.pre").
 */
void escreverArquivoAnotado(char *nome_arquivo_origem, char *nome_arquivo_destino) {
    // 1. Array de prefixos por linha (inicialmente vazios)
    // O MAX_LINHAS aqui é um limite, mas o ideal seria descobrir o número exato de linhas.
    // Usaremos MAX_LINHAS como um limite seguro.
    char **prefixos = calloc(MAX_LINHAS, sizeof(char *));
    if (!prefixos) {
        printf("Erro fatal: Falha ao alocar memória para prefixos de erro.\n");
        return;
    }
    
    // 2. Coleta os erros e constrói os prefixos
    for (int i = 0; i < num_erros; i++) {
        int linha_index = lista_de_erros[i].linha - 1; // Linha 1 -> Índice 0
        if (linha_index >= 0 && linha_index < MAX_LINHAS) {
            const char *novo_prefixo = tipoErroParaPrefixo(lista_de_erros[i].tipo);
            if (prefixos[linha_index] == NULL) {
                // Primeira anotação para esta linha
                prefixos[linha_index] = strdup(novo_prefixo);
            } else {
                // Já existe um erro na linha, anexa o novo prefixo (para múltiplos erros)
                char *temp = prefixos[linha_index];
                size_t len_novo = strlen(novo_prefixo);
                size_t len_antigo = strlen(temp);
                prefixos[linha_index] = realloc(temp, len_antigo + len_novo + 1);
                if (prefixos[linha_index]) {
                    strcat(prefixos[linha_index], novo_prefixo);
                } else {
                    printf("Erro fatal: Falha ao realocar prefixo.\n");
                    prefixos[linha_index] = temp; // Restaura o ponteiro original
                }
            }
        }
    }

    // 3. Abre o arquivo de origem e destino
    FILE *origem = fopen(nome_arquivo_origem, "r");
    FILE *destino = fopen(nome_arquivo_destino, "w");

    if (!origem) {
        printf("Erro: Nao foi possivel abrir o arquivo de origem '%s'.\n", nome_arquivo_origem);
    } else if (!destino) {
        printf("Erro: Nao foi possivel criar o arquivo de destino '%s'.\n", nome_arquivo_destino);
        fclose(origem);
    } else {
        char *linha = NULL;
        size_t len = 0;
        size_t read;
        int cont_linha = 0;

        // 4. Lê linha por linha, adiciona o prefixo e escreve
        while ((read = getline(&linha, &len, origem)) != -1) {
            // Remove quebra de linha para adicionar o prefixo e reescrever
            if (read > 0 && linha[read - 1] == '\n') {
                linha[read - 1] = '\0';
            }

            if (cont_linha < MAX_LINHAS && prefixos[cont_linha] != NULL) {
                fprintf(destino, "%s%s\n", prefixos[cont_linha], linha);
            } else {
                fprintf(destino, "%s\n", linha);
            }
            cont_linha++;
        }

        free(linha);
        fclose(origem);
        fclose(destino);

        printf("\nArquivo anotado com erros salvo em: %s\n", nome_arquivo_destino);
    }

    // 5. Libera os prefixos
    for (int i = 0; i < MAX_LINHAS; i++) {
        free(prefixos[i]);
    }
    free(prefixos);
}

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
        if (token){
            // Procura na tabela de definições
            rotulo_definido = 0;
            for(linha = 0; tabela_definicao && tabela_definicao[linha]; linha++){
                if(strcmp(tabela_definicao[linha][0], token) == 0){
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
                        rotulo_definido = 1;
                        sprintf(msg_erro, "Rotulo '%s' declarado duas vezes (redefinicao).", token);
                        adicionarErro(lin_atual + 1, ERRO_SEMANTICO, msg_erro);
                        break;
                    }
                }
            }
            if(!rotulo_definido){
                // Adiciona o rótulo na tabela de definições
                int nova_linha = tabela_definicao ? linha : 0;
                tabela_definicao = realloc(tabela_definicao, sizeof(char **) * (nova_linha + 2));
                if (!tabela_definicao) { parar_compilacao = 1; break; }
                tabela_definicao[nova_linha] = calloc(5, sizeof(char *));
                if (!tabela_definicao[nova_linha]) { parar_compilacao = 1; break; }

                tabela_definicao[nova_linha][0] = strdup(token); // Rótulo
                tabela_definicao[nova_linha][1] = malloc(5 * sizeof(char));
                sprintf(tabela_definicao[nova_linha][1], "%d", endereco_atual); // Endereço
                tabela_definicao[nova_linha][2] = strdup("D"); // Definido
                tabela_definicao[nova_linha][3] = strdup("-1"); // Última pendência
                tabela_definicao[nova_linha][4] = strdup("-1"); // Linha do erro (não aplicável)
                tabela_definicao[nova_linha + 1] = NULL; // Marca o fim da tabela
            }
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
                codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
                if (!codigo_objeto1 || !codigo_objeto2) { parar_compilacao = 1; break; }
                codigo_objeto1[endereco_atual] = i+1;
                codigo_objeto2[endereco_atual] = i+1;
                endereco_atual++;
                end_encontrado = 1;

                // Checagem de número de operandos (Erro Sintático)
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
        if (parar_compilacao) break;
        
        if(!end_encontrado && strcmp(token, "const") == 0){
            // Diretiva CONST
            if (tabela_tokenizada[lin_atual][2] == NULL || tabela_tokenizada[lin_atual][3] != NULL) {
                adicionarErro(lin_atual + 1, ERRO_SINTATICO, "Diretiva 'const' espera exatamente 1 operando (o valor).");
            } else {
                 char *endptr;
                 strtol(tabela_tokenizada[lin_atual][2], &endptr, 10);
                 if (*endptr != '\0') {
                     adicionarErro(lin_atual + 1, ERRO_SEMANTICO, "Operando de 'const' deve ser um valor numerico.");
                 }
            }
            
            codigo_objeto1 = realloc(codigo_objeto1, sizeof(int) * (endereco_atual + 1));
            codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
            if (!codigo_objeto1 || !codigo_objeto2) { parar_compilacao = 1; break; }
            int valor_const = (tabela_tokenizada[lin_atual][2] ? atoi(tabela_tokenizada[lin_atual][2]) : 0);
            codigo_objeto1[endereco_atual] = valor_const;
            codigo_objeto2[endereco_atual] = valor_const;
            endereco_atual++;
            end_encontrado = 1; // Marca como "encontrado" para não cair no erro de instrução inválida
        }else if(!end_encontrado && strcmp(token, "space") == 0){
            // Diretiva SPACE
            if (tabela_tokenizada[lin_atual][3] != NULL) {
                 adicionarErro(lin_atual + 1, ERRO_SINTATICO, "Diretiva 'space' espera 0 ou 1 operando.");
            }
            op_space = tabela_tokenizada[lin_atual][2];
            int quantidade = 1;
            if(op_space != NULL){
                int valor = atoi(op_space);
                if(valor > 0) quantidade = valor;
                else adicionarErro(lin_atual + 1, ERRO_SEMANTICO, "Operando de 'space' deve ser um valor numerico positivo.");
            }
            for(int i = 0; i < quantidade; i++){
                codigo_objeto1 = realloc(codigo_objeto1, sizeof(int) * (endereco_atual + 1));
                codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
                if (!codigo_objeto1 || !codigo_objeto2) { parar_compilacao = 1; break; }
                codigo_objeto1[endereco_atual] = 0;
                codigo_objeto2[endereco_atual] = 0;
                endereco_atual++;
            }
            end_encontrado = 1; // Marca como "encontrado"
        }else if(!end_encontrado){
            sprintf(msg_erro, "Instrucao ou diretiva invalida: '%s'.", token);
            adicionarErro(lin_atual + 1, ERRO_LEXICO, msg_erro);
        }

        if (parar_compilacao) break;

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
                // Erro semântico: Instruções devem usar rótulos (endereços de memória)
                sprintf(msg_erro, "Operando '%s' invalido. Esperava-se um rotulo de memoria (end. indireto).", token);
                adicionarErro(lin_atual + 1, ERRO_SEMANTICO, msg_erro);
                
                // Aloca espaço para o número, para continuar a execução
                codigo_objeto1 = realloc(codigo_objeto1, sizeof(int) * (endereco_atual + 1));
                codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
                if (!codigo_objeto1 || !codigo_objeto2) { parar_compilacao = 1; break; }
                codigo_objeto1[endereco_atual] = atoi(token);
                codigo_objeto2[endereco_atual] = atoi(token);
                endereco_atual++;
                continue; // Pula para o próximo operando
            }
            if (parar_compilacao) break;

            if(tabela_definicao){
                // Procura na tabela de definicoes
                for(linha = 0; tabela_definicao[linha]; linha++){
                    op_const = tabela_definicao[linha][0];
                    if(strcmp(op_const, token) == 0){
                        // Operando encontrado
                        operando_encontrado = 1;
                        if(strcmp(tabela_definicao[linha][2], "D") == 0){ // Rótulo definido
                            // Rótulo já definido
                            codigo_objeto1 = realloc(codigo_objeto1, sizeof(int) * (endereco_atual + 1));
                            codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
                            if (!codigo_objeto1 || !codigo_objeto2) { parar_compilacao = 1; break; }

                            codigo_objeto1[endereco_atual] = atoi(tabela_definicao[linha][1]);
                            codigo_objeto2[endereco_atual] = atoi(tabela_definicao[linha][1]);
                        }else{
                            // Rótulo ainda não definido ("P")
                            int ultimo_endereco_pendente = atoi(tabela_definicao[linha][3]);
                            codigo_objeto1 = realloc(codigo_objeto1, sizeof(int) * (endereco_atual + 1));
                            codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
                            if (!codigo_objeto1 || !codigo_objeto2) { parar_compilacao = 1; break; }
                            
                            codigo_objeto1[endereco_atual] = ultimo_endereco_pendente;
                            codigo_objeto2[endereco_atual] = ultimo_endereco_pendente;
                            
                            free(tabela_definicao[linha][3]); // Libera pendencia antiga
                            tabela_definicao[linha][3] = malloc(5 * sizeof(char));
                            if (!tabela_definicao[linha][3]) { parar_compilacao = 1; break; }
                            sprintf(tabela_definicao[linha][3], "%d", endereco_atual);
                        }
                        break;
                    }
                }
                if (parar_compilacao) break;

                if(!operando_encontrado){
                    // Adiciona operando como pendente na tabela de definicoes
                    int nova_linha = tabela_definicao ? linha : 0;
                    tabela_definicao = realloc(tabela_definicao, sizeof(char **) * (nova_linha + 2));
                    if (!tabela_definicao) { parar_compilacao = 1; break; }
                    tabela_definicao[nova_linha] = calloc(5, sizeof(char *));
                    if (!tabela_definicao[nova_linha]) { parar_compilacao = 1; break; }

                    tabela_definicao[nova_linha][0] = strdup(token); // Rótulo
                    tabela_definicao[nova_linha][1] = strdup("-1"); // Endereço
                    tabela_definicao[nova_linha][2] = strdup("P"); // Pendente
                    tabela_definicao[nova_linha][3] = malloc(5 * sizeof(char));
                    sprintf(tabela_definicao[nova_linha][3], "%d", endereco_atual); // Última pendência
                    // NOVO: Armazena a linha do erro
                    tabela_definicao[nova_linha][4] = malloc(5 * sizeof(char));
                    sprintf(tabela_definicao[nova_linha][4], "%d", lin_atual + 1); // Linha da primeira pendencia
                    tabela_definicao[nova_linha + 1] = NULL;
                    
                    codigo_objeto1 = realloc(codigo_objeto1, sizeof(int) * (endereco_atual + 1));
                    codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
                    if (!codigo_objeto1 || !codigo_objeto2) { parar_compilacao = 1; break; }
                    codigo_objeto1[endereco_atual] = -1; // Fim da lista de pendencias
                    codigo_objeto2[endereco_atual] = -1; // Fim da lista
                }
            }else{
                // Primeira vez que adiciona um operando pendente
                tabela_definicao = malloc(sizeof(char **) * 2);
                if (!tabela_definicao) { parar_compilacao = 1; break; }
                tabela_definicao[0] = calloc(5, sizeof(char *));
                if (!tabela_definicao[0]) { parar_compilacao = 1; break; }

                tabela_definicao[0][0] = strdup(token); // Rótulo
                tabela_definicao[0][1] = strdup("-1"); // Endereço
                tabela_definicao[0][2] = strdup("P"); // Pendente
                tabela_definicao[0][3] = malloc(5 * sizeof(char));
                sprintf(tabela_definicao[0][3], "%d", endereco_atual); // Última pendência
                // Armazena a linha do erro
                tabela_definicao[0][4] = malloc(5 * sizeof(char));
                sprintf(tabela_definicao[0][4], "%d", lin_atual + 1); // Linha da primeira pendencia
                tabela_definicao[1] = NULL;
                
                codigo_objeto1 = realloc(codigo_objeto1, sizeof(int) * (endereco_atual + 1));
                codigo_objeto2 = realloc(codigo_objeto2, sizeof(int) * (endereco_atual + 1));
                if (!codigo_objeto1 || !codigo_objeto2) { parar_compilacao = 1; break; }
                codigo_objeto1[endereco_atual] = -1;
                codigo_objeto2[endereco_atual] = -1;
            }
            endereco_atual++;
        }
        lin_atual++;
    }

    // imprimir os arquivos objeto (somente se não houve erro fatal)
    if (!parar_compilacao) {
        imprimeArquivoObjeto(codigo_objeto1, endereco_atual, "output.o1");
        imprimeArquivoObjeto(codigo_objeto2, endereco_atual, "output.o2");
    }

    // Libera a memória dos vetores de código objeto
    free(codigo_objeto1);
    free(codigo_objeto2);

    return tabela_definicao; // Retorna a tabela para checagem final
}

// Funções de limpeza de memória:

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

// Remover se houver mais de um espaco/tab/quebra de linha
char *limparEspaco(char *arquivo_pre){
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
    for(int i = 0; arquivo[i] != '\0'; i++)
        arquivo[i] = tolower(arquivo[i]);
}

char ***separaTokens(char *arquivo_pre){

    int cont_car = 0;
    char ***tabela_tokens = malloc(sizeof(char **) * (MAX_LINHAS + 1));
    if (!tabela_tokens) { parar_compilacao = 1; return NULL; }
    char *linha_atual = malloc(1);
    if (!linha_atual) { parar_compilacao = 1; free(tabela_tokens); return NULL; }
    char **linhas_arquivo = malloc(sizeof(char *) * (MAX_LINHAS + 1));
    if (!linhas_arquivo) { parar_compilacao = 1; free(tabela_tokens); free(linha_atual); return NULL; }
    int cont_lin = 0;
    int cont_car_lin = 0;

    // Separa as linhas
    do{
        // Garante que não ultrapasse o limite teórico (embora raro)
        if (cont_lin >= MAX_LINHAS) {
            adicionarErro(cont_lin + 1, ERRO_LEXICO, "Limite de MAX_LINHAS excedido.");
            break;
        }

        char *temp_linha = realloc(linha_atual, sizeof(char) * (cont_car_lin + 2)); // +2 para o caractere atual + '\0'
        if (!temp_linha) { parar_compilacao = 1; break; }
        linha_atual = temp_linha;

        if(arquivo_pre[cont_car] == '\n' || arquivo_pre[cont_car] == '\0'){
            linha_atual[cont_car_lin] = '\0';
            linhas_arquivo[cont_lin] = linha_atual;
            cont_lin++;
            cont_car_lin = 0;
            
            // Realoque linhas_arquivo apenas se for realmente necessário (ou seja, se cont_lin <= MAX_LINHAS)
            if (cont_lin < MAX_LINHAS) {
                 char **temp_linhas = realloc(linhas_arquivo, sizeof(char *) * (cont_lin + 1));
                 if (!temp_linhas) { parar_compilacao = 1; break; }
                 linhas_arquivo = temp_linhas;
            }

            linha_atual = malloc(1);
            if (!linha_atual) { parar_compilacao = 1; break; }

        }
        else{
            linha_atual[cont_car_lin] = arquivo_pre[cont_car];
            cont_car_lin++;
        }
    }while(arquivo_pre[cont_car++] != '\0');
    
    if (parar_compilacao) {
        // Liberação de memória em caso de falha de alocação
        if (linha_atual) free(linha_atual);
        for(int i = 0; i < cont_lin; i++) free(linhas_arquivo[i]);
        if (linhas_arquivo) free(linhas_arquivo);
        if (tabela_tokens) free(tabela_tokens);
        return NULL;
    }
    
    free(linha_atual); // Libera a última alocação extra
    linhas_arquivo[cont_lin] = NULL; // Marca o fim do array de linhas

    // Inicializa a tabela de tokens
    for(int i = 0; i < cont_lin; i++){
        tabela_tokens[i] = calloc(4, sizeof(char *));
        if (!tabela_tokens[i]) { parar_compilacao = 1; break; }
    }
    tabela_tokens[cont_lin] = NULL;
    
    if (parar_compilacao) {
        // Libera tokens alocados até agora
        for(int i = 0; i <= cont_lin; i++) if (tabela_tokens[i]) free(tabela_tokens[i]);
        if (tabela_tokens) free(tabela_tokens);
        // Libera as linhas do arquivo
        for(int i = 0; i < cont_lin; i++) free(linhas_arquivo[i]);
        if (linhas_arquivo) free(linhas_arquivo);
        return NULL;
    }


    // separa os tokens
    for(int i = 0; i < cont_lin; i++){
        char *line = linhas_arquivo[i];
        if(line == NULL || line[0] == '\0') continue;

        char *copy = strdup(line);
        if (!copy) { parar_compilacao = 1; break; }
        char *ptr = copy;

        // Verifica se há rótulo (':')
        char *label = NULL;
        char *colon = strchr(ptr, ':');
        if(colon){
            *colon = '\0';
            label = strdup(ptr);
            if (!label) { parar_compilacao = 1; free(copy); break; }
            ptr = colon + 1;

            // NOVO: Checagem Lexical do Rótulo
            if (label[0] != '\0') { // Rótulo não vazio
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
            } else {
                free(label);
                label = NULL;
            }
        }

        // Trim inicial
        while(*ptr == ' ' || *ptr == '\t') ptr++;

        // NOVO: Checagem de dois rótulos (Erro Sintático)
        if (*ptr == ':') {
            adicionarErro(i + 1, ERRO_SINTATICO, "Caracter invalido ou dois rotulos na mesma linha (erro sintatico/lexico).");
            // Tenta se recuperar pulando o ':'
            ptr = ptr + 1;
            while(*ptr == ' ' || *ptr == '\t') ptr++;
        }

        char *instr = NULL;
        char *op1 = NULL;
        char *op2 = NULL;

        char *saveptr;
        char *tok = strtok_r(ptr, " \t\n", &saveptr);
        if(tok) {
             instr = strdup(tok);
             if (!instr) { parar_compilacao = 1; free(copy); break; }
        }

        if(instr){
            char *rest = saveptr;
            if(rest){
                char *o1 = strtok_r(rest, ",\t\n", &saveptr);
                if(o1){
                    while(*o1 == ' ') o1++;
                    if (*o1 != '\0') {
                        op1 = strdup(o1); // Evita duplicar strings vazias
                        if (!op1) { parar_compilacao = 1; free(copy); break; }
                    }
                }
                char *o2 = strtok_r(NULL, ",\t\n", &saveptr);
                if(o2){
                    while(*o2 == ' ') o2++;
                    if (*o2 != '\0') {
                        op2 = strdup(o2); // Evita duplicar strings vazias
                        if (!op2) { parar_compilacao = 1; free(copy); break; }
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
    
    // Libera o array de linhas temporário
    for(int i = 0; i < cont_lin; i++) {
        free(linhas_arquivo[i]);
    }
    free(linhas_arquivo);

    if (parar_compilacao) {
        liberarTabelaTokens(tabela_tokens);
        return NULL;
    }
    
    return tabela_tokens;
}


int main(void){
    char *nome_arquivo_entrada = "saida.pre";
    char *nome_arquivo_anotado = "saida_anotada.pre";
    
    // 1. Abre o arquivo e lê o conteúdo original
    char *conteudo = abreArquivo(nome_arquivo_entrada);
    if(conteudo == NULL){
        return 1;
    }
    printf("____________________ documento original ____________________\n");
    printf("%s\n\n", conteudo);

    // Salva o conteúdo original para a anotação
    char *conteudo_original_copia = strdup(conteudo);
    if (!conteudo_original_copia) {
        printf("Erro ao alocar cópia do conteúdo.\n");
        free(conteudo);
        return 1;
    }


    // 2. Pré-processamento e Tokenização
    converterMinusculo(conteudo);
    printf("____________________ documento em minusculo ____________________\n");
    printf("%s\n\n", conteudo);

    char *conteudoLimpo = limparEspaco(conteudo);
    free(conteudo);
    if(conteudoLimpo == NULL){
        free(conteudo_original_copia);
        return 1;
    }
    printf("____________________ documento limpo ____________________\n");
    printf("%s\n\n", conteudoLimpo);

    char ***tokens = separaTokens(conteudoLimpo);
    free(conteudoLimpo);
    
    if(tokens == NULL || parar_compilacao){
        printf("Erro fatal durante a tokenizacao/alocacao. Terminando.\n");
        free(conteudo_original_copia);
        liberarErros();
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
    // 3. Passagem Única
    char ***tabela_defs = passagemUnica(tokens);

    // 4. Checagem final de erros e impressão de relatório
    if (!parar_compilacao) {
        checarPendencias(tabela_defs);
    }
    
    imprimirErros();

    // 5. Anotação e escrita do arquivo de saída
    if (num_erros > 0) {
        escreverArquivoAnotado(nome_arquivo_entrada, nome_arquivo_anotado);
    } else {
        printf("\nNao houve erros para anotar no arquivo.\n");
    }

    // 6. Liberação da memória
    liberarTabelaTokens(tokens);
    liberarTabelaDefinicao(tabela_defs);
    liberarErros();
    free(conteudo_original_copia);

    return 0;
}
