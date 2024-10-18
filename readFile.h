#ifndef READFILE_H
#define READFILE_H

#define MAX_LINE_LENGTH 256
#define MAX_PROCESSES 100
#define MAX_SEQ_LENGTH 100

// Global variables
extern char algDeEscalonamento[50];
extern int clockCPU;
extern char politicaDeMemoria[50];
extern int tamanhoMemoria;
extern int tamanhoPagina;
extern int percentualAlocacao;
extern int acessoPorCiclo;
extern int acessosNaMemoria;
extern int *primaryMemory;

// Struct definition
typedef struct {
    char nome_processo[50];
    int id,
        prioridade,
        tempo_execucao,
        qtdMemoria,
        sequencia[MAX_SEQ_LENGTH],
        tamanho_sequencia,
        estadoSequencia,
        oldestPagAdded,
        latencia,
        trocasDePaginas;
} DadosProcessos;

int read_process(const char *nome_arquivo, DadosProcessos *listaP);
void show_process(DadosProcessos *listaP, int numeroProcessos);

#endif