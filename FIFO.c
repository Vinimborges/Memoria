#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "escalonadorPrioridade.h"

#define MAX_LINE_LENGTH 256
#define MAX_PROCESSES 100
#define MAX_SEQ_LENGTH 100

// Dados dos processos
typedef struct {
    char nome_processo[50];
    int id,
        prioridade,
        tempo_execucao,
        qtdMemoria,
        sequencia[MAX_SEQ_LENGTH],
        tamanho_sequencia;
} DadosProcessos;

char algDeEscalonamento[50];
char politicaDeMemoria[50];
int tamanhoMemoria, tamanhoPagina, percentualAlocacao, acessoPorCiclo;

DadosProcessos *listaP = NULL; // Ponteiro para a lista de processos

// Função para ler os processos do arquivo
int read_process(const char *nome_arquivo, DadosProcessos *listaP) {
    FILE *file = fopen(nome_arquivo, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        return -1;
    }

    int i = 0;
    int lineCount = 0;
    char line[MAX_LINE_LENGTH];
    int controlador = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        // Remover o caractere de nova linha (\n), se existir
        line[strcspn(line, "\n")] = 0;

        if (controlador == 0) {
            // Ler as informações de configuração
            strcpy(algDeEscalonamento, strtok(line, "|"));
            int clock = atoi(strtok(NULL, "|"));
            strcpy(politicaDeMemoria, strtok(NULL, "|"));
            tamanhoMemoria = atoi(strtok(NULL, "|"));
            tamanhoPagina = atoi(strtok(NULL, "|"));
            percentualAlocacao = atoi(strtok(NULL, "|"));
            acessoPorCiclo = atoi(strtok(NULL, "|"));
            controlador++;
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
    return lineCount;
}

void show_process(DadosProcessos *listaP, int numeroProcessos) {
    for (int j = 0; j < numeroProcessos; j++) {
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
}

int main() {
    printf("Algoritmo FIFO\n");

    // Alocar memória para a lista de processos
    listaP = (DadosProcessos *)malloc(MAX_PROCESSES * sizeof(DadosProcessos));
    if (listaP == NULL) {
        perror("Erro ao alocar memória para a lista de processos");
        return EXIT_FAILURE;
    }

    // Chamar a função para ler processos do arquivo
    int numeroProcessos = read_process("entradaMemoria.txt", listaP);
    if (numeroProcessos == -1) {
        free(listaP); // Liberar a memória alocada
        return EXIT_FAILURE;
    }

    show_process(listaP, numeroProcessos);

    // Liberar a memória alocada
    free(listaP);

    return 0;
}
