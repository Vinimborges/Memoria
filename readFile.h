#ifndef READFILE_H
#define READFILE_H

#define MAX_LINE_LENGTH 256
#define MAX_PROCESSES 100

// Definir a estrutura com um nome específico
typedef struct DadosProcessos {
    char nome_processo[50];
    int id;
    int tempo_execucao;
    int prioridade;
    int qtdMemoria;
    int sequencia[50];
    int tamanho_sequencia;
    int estadoSequencia;
    int trocasDePaginas;
    int latencia;  // Add this member if not already present
} DadosProcessos;

int read_process(const char *nome_arquivo, DadosProcessos *listaP);
void show_process(DadosProcessos *listaP, int numeroProcessos);

#endif