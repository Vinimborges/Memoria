#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "escalonadorPrioridade.h"

#define MAX_LINE_LENGTH 256
#define MAX_PROCESSES 100
#define MAX_SEQ_LENGTH 100

char line[MAX_LINE_LENGTH];
int lineCount = 0;

//Dados dos processos
typedef struct{
    char nome_processo[50];
    int id,
        prioridade,
        tempo_execucao,
        qtdMemoria,
        sequencia[MAX_SEQ_LENGTH],
        tamanho_sequencia;
} DadosProcessos;

char politicaDeMemoria[50];
int tamanhoMemoria, tamanhoPagina, percentualAlocacao, acessoPorCiclo;

DadosProcessos *listaP = NULL; //Criando a lista onde os processos vão ser armazenados.
int main() {    
    printf("Algoritmo FIFO\n");

    FILE *file = fopen("entradaMemoria.txt", "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        exit(EXIT_FAILURE);
    }

    DadosProcessos listaP[MAX_PROCESSES];
    int lineCount = 0;
    char line[MAX_LINE_LENGTH];
    int i = 0;
    int controlador = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        // Remova o caractere de nova linha (\n) se existir
        line[strcspn(line, "\n")] = 0;

        if (controlador == 0) {
            // Ler as informações de configuração
            strtok(line, "|");
            int clock = atoi(strtok(NULL, "|"));
            strcpy(politicaDeMemoria, strtok(NULL, "|"));
            tamanhoMemoria = atoi(strtok(NULL, "|"));
            tamanhoPagina = atoi(strtok(NULL, "|"));
            percentualAlocacao = atoi(strtok(NULL, "|"));
            acessoPorCiclo = atoi(strtok(NULL, "|"));
            controlador++;

            printf("Clock: %d\n", clock);
            printf("Política de Memória: %s\n", politicaDeMemoria);
            printf("Tamanho Memória: %d\n", tamanhoMemoria);
            printf("Tamanho Página: %d\n", tamanhoPagina);
            printf("Percentual de Alocação: %d\n", percentualAlocacao);
            printf("Acesso por Ciclo: %d\n", acessoPorCiclo);
        } else {
            // Processar informações de cada processo
            strcpy(listaP[i].nome_processo, strtok(line, "|"));
            listaP[i].id = atoi(strtok(NULL, "|"));
            listaP[i].tempo_execucao = atoi(strtok(NULL, "|"));
            listaP[i].prioridade = atoi(strtok(NULL, "|"));
            listaP[i].qtdMemoria = atoi(strtok(NULL, "|"));

            // Ler e armazenar a sequência
            char *sequencia_str = strtok(NULL, "|");
            char *token = strtok(sequencia_str, " ");
            listaP[i].tamanho_sequencia = 0;

            while (token != NULL) {
                listaP[i].sequencia[listaP[i].tamanho_sequencia++] = atoi(token);
                token = strtok(NULL, " ");
            }

            i++;
            lineCount++;
        }
    }

    fclose(file);

    // Exibir informações dos processos
    for (int j = 0; j < lineCount; j++) {
        printf("\nProcesso: %s\n", listaP[j].nome_processo);
        printf("ID: %d\n", listaP[j].id);
        printf("Tempo de Execução: %d\n", listaP[j].tempo_execucao);
        printf("Prioridade: %d\n", listaP[j].prioridade);
        printf("Qtd Memória: %d\n", listaP[j].qtdMemoria);
        printf("Sequência de números: ");
        for (int k = 0; k < listaP[j].tamanho_sequencia; k++) {
            printf("%d ", listaP[j].sequencia[k]);
        }
        printf("\n");
    }

    return 0;
}