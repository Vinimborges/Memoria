#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include "escalonadorPrioridade.h"

#define MAX_LINE_LENGTH 256
#define MAX_PROCESSES 100
#define MAX_SEQ_LENGTH 100

int iterador = 0;

// Dados dos processos
typedef struct {
    char nome_processo[50];
    int id,
        prioridade,
        tempo_execucao,
        qtdMemoria,
        sequencia[MAX_SEQ_LENGTH],
        tamanho_sequencia,
        estadoSequencia,
        latencia,
        trocasDePaginas;
} DadosProcessos;

char algDeEscalonamento[50];
char politicaDeMemoria[50];
int clockCPU, tamanhoMemoria, tamanhoPagina, percentualAlocacao, acessoPorCiclo, acessosNaMemoria;
int *primaryMemory;

typedef struct {
    int id,
        start,
        end;
} ProcessInMemory;

ProcessInMemory *memoryProcesses = NULL;  // Lista de processos atualmente na memoria
ProcessInMemory *oldestProcess = NULL;    // Primeiro a entrar na fila
ProcessInMemory *newestProcess = NULL;    // Ultimo a entrar na fila
int nOfProcessesMemory = 0;

DadosProcessos *listaP = NULL; // Ponteiro para a lista de processos

//------------------------------------------- Algortimo de MEMORIA FIFO --------------------------------------------------------------------------------------------

// Imprime os processos que estão na memoria principal com a posição de inicio e fim
int print_memory_processes() {
    printf("\nProcessos na memória pricipal:\n");
    if (memoryProcesses == NULL || nOfProcessesMemory == 0) {
        printf("Nenhum processo está atualmente na memória.\n");
        return 0;
    }
    for (int i = 0; i < nOfProcessesMemory; i++) {
        printf("Process ID: %d, Start: %d, End: %d\n", 
            memoryProcesses[i].id, 
            memoryProcesses[i].start, 
            memoryProcesses[i].end);
    }
    return 0;
}

// Imprime o conteudo da memoria principal
int print_primary_memory() {
    printf("\nMemória Pricipal:\n");
    for(int j = 0; j < (tamanhoMemoria / tamanhoPagina); j++){
        printf("%d",primaryMemory[j]);
    }
    return 0;
}

// Adiciona valores na memoria, poderiam ser dados ao inves de numeros em ordem
void addIntoMemory(int start, int end){
    int k = 1;
    for(int i = start; i <= end; i++){
        primaryMemory[i] = k;
        k++;
    }
}

// Verifica o quanto ha espaco na memoria
int availableMemory(){
    // printf("\nOldest Process ID: %d, Start: %d, End: %d\n", oldestProcess->id, oldestProcess->start, oldestProcess->end);
    int empty = 0;
    for (int i = 0; i < (tamanhoMemoria / tamanhoPagina); i++) {
        if (primaryMemory[i] == 0) {  // Considera 0 como um espaço vazio
            empty++;
        }
    }
    printf("Vazio %d\n",empty);
    return empty;
}

void initOldestProcess(int id, int start, int end) {
    oldestProcess = (ProcessInMemory *)malloc(sizeof(ProcessInMemory));
    if (oldestProcess == NULL) {
        printf("Erro ao alocar memória.\n");
        exit(1); 
    }
    oldestProcess->id = id;
    oldestProcess->start = start;
    oldestProcess->end = end;
}

void initNewestProcess(int id, int start, int end) {
    newestProcess = (ProcessInMemory *)malloc(sizeof(ProcessInMemory));
    if (newestProcess == NULL) {
        printf("Erro ao alocar memória.\n");
        exit(1);
    }
    newestProcess->id = id;
    newestProcess->start = start;
    newestProcess->end = end;
}

int findProcessById(ProcessInMemory *processList, int size, int id) {
    for (int i = 0; i < size; i++) {
        if (processList[i].id == id) {
            return 1;
        }
    }
    return 0;
}

ProcessInMemory findProcessByIdData(ProcessInMemory *processList, int size, int id) {
    ProcessInMemory result = {-1, -1, -1};  // Caso não encontre o processo, retorna valores inválidos.
    for (int i = 0; i < size; i++) {
        if (processList[i].id == id) {
            result = processList[i];  // Atribui a estrutura encontrada à variável result
            break;
        }
    }
    return result;  // Retorna a estrutura
}

void FIFO(DadosProcessos *listaP, int posicao, int tamanhoMemoria){
    int paginas = listaP[posicao].qtdMemoria / tamanhoPagina - 1;
    ProcessInMemory *oldestProcess = (ProcessInMemory *)malloc(sizeof(ProcessInMemory));
    
    if(!findProcessById(memoryProcesses, nOfProcessesMemory, listaP[posicao].id)){ // caso o processo nao esteja na memoria
        if ((tamanhoMemoria - listaP[posicao].qtdMemoria) >= 0){
            printf("Coube na memoria\n");
            if (memoryProcesses == NULL){
                printf("Memoria vazia\n");
                
                ProcessInMemory process;
                process.id = listaP[posicao].id;
                process.start = 0;
                process.end = paginas;

                ProcessInMemory *temp = realloc(memoryProcesses, (nOfProcessesMemory + 1) * sizeof(ProcessInMemory));
                if (temp == NULL) {
                    perror("Erro ao realocar memória");
                    exit(EXIT_FAILURE);
                }
                memoryProcesses = temp;
                
                initOldestProcess(process.id, process.start,  process.end); // Armazena o ultimo processo adicionado
                initNewestProcess(process.id, process.start,  process.end); // Armazena o ultimo processo adicionado
                memoryProcesses[nOfProcessesMemory] = process;              // adiciona na lista dos processos atuais na memoria
                nOfProcessesMemory++;

                addIntoMemory(process.start,process.end);
            }
            else{
                if(availableMemory() >= (listaP[posicao].qtdMemoria/tamanhoPagina)){ // Nao precisa remover nenhum processo
                    ProcessInMemory process;
                    process.id = listaP[posicao].id;
                    process.start = newestProcess->end + 1;
                    process.end = process.start + paginas;

                    initNewestProcess(process.id, process.start,  process.end); // Atualiza o último processo
                    memoryProcesses = realloc(memoryProcesses, (nOfProcessesMemory + 1) * sizeof(ProcessInMemory));
                    memoryProcesses[nOfProcessesMemory] = process;              // Adiciona na lista de processos na memória
                    nOfProcessesMemory++;
                    addIntoMemory(process.start,process.end);
                }
            }
        }
    } 
    print_memory_processes();
    // print_primary_memory();

    ProcessInMemory processData = findProcessByIdData(memoryProcesses, nOfProcessesMemory, listaP[posicao].id);
    // for(int i = processData.start; i <= processData.end; i++){
    //     printf("%d",primaryMemory[i]);
    // }
    printf("Acesso a memoria: ");
    for(int i = listaP[posicao].estadoSequencia; i < (listaP[posicao].estadoSequencia + acessosNaMemoria); i++){
        if(listaP[posicao].sequencia[i] != 0){
            printf("%d ",listaP[posicao].sequencia[i]);
        }
    }
    listaP[posicao].estadoSequencia += acessosNaMemoria;
    printf("\n");
    // TUDO CERTO ATE AQUI
}

//------------------------------------------- LEITURA DO ARQUIVO DE ENTRADA E MANIPULACAO --------------------------------------------------------------------------------------------
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
            clockCPU = atoi(strtok(NULL, "|"));
            strcpy(politicaDeMemoria, strtok(NULL, "|"));
            tamanhoMemoria = atoi(strtok(NULL, "|"));
            tamanhoPagina = atoi(strtok(NULL, "|"));
            percentualAlocacao = atoi(strtok(NULL, "|"));
            acessoPorCiclo = atoi(strtok(NULL, "|"));

            acessosNaMemoria = clockCPU * acessoPorCiclo;
            controlador++;

            int *temp = realloc(primaryMemory,( tamanhoMemoria * sizeof(int)));
            primaryMemory = temp;
            printf("Memoria Principal com capacidade de %d bytes\n",tamanhoMemoria);
        } else {
            // Processar informações de cada processo
            strcpy(listaP[i].nome_processo, strtok(line, "|"));
            listaP[i].id = atoi(strtok(NULL, "|"));
            listaP[i].tempo_execucao = atoi(strtok(NULL, "|"));
            listaP[i].prioridade = atoi(strtok(NULL, "|"));
            listaP[i].qtdMemoria = atoi(strtok(NULL, "|"));
            listaP[i].estadoSequencia = 0;
            listaP[i].trocasDePaginas = 0;

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
        printf("Sequência Acessos: ");
        for (int k = 0; k < listaP[j].tamanho_sequencia; k++) {
            printf("%d ", listaP[j].sequencia[k]);
        }
        printf("\n");
    }
}

//------------------------------------------- ESCALONADOR DE PROCESSO POR PRIORIDADE ---------------------------------------------------------------------------------------------
pthread_mutex_t mutex_prioridade;
//Função para criar um arquivo txt onde estarão armazenados o Id e a Latência de cada processo.
void criando_arquivo(){
    int i = 0;
    FILE* arquivo_3 = fopen("SaidaPrioridade.txt", "w");
    fprintf(arquivo_3, "ID | LATÊNCIA\n");

    while (i < iterador){
        if (listaP[i].id >= 0){
            fprintf(arquivo_3, "%d | ", listaP[i].id);
            fprintf(arquivo_3, "%d\n", listaP[i].latencia);
        }
        i++;
    }
}

//Função que recebe a lista de processos e executa-os. Durante o procedimento de executar um novo processo ele "trava" a função recebe_novos_processos usando um mutex.
void *executando_processos(void* arg){
    int latencia = 0, id_processo_anterior;
    int cpuClock = *(int*)arg;

    while(true){
        int maior_prioridade = 0, j = 0, k = 0, posicao = 0,  clock = *(int*)arg;;
        while(j < iterador){
            if (maior_prioridade < listaP[j].prioridade && listaP[j].tempo_execucao > 0){
                maior_prioridade = listaP[j].prioridade;
                posicao = j;
            }
            j++;     
        }

        if (maior_prioridade > 0){
            pthread_mutex_lock(&mutex_prioridade);

            if (listaP[posicao].tempo_execucao - clock >= 0){
                listaP[posicao].tempo_execucao = listaP[posicao].tempo_execucao - clock;
            }
            else{
                clock = listaP[posicao].tempo_execucao;
                listaP[posicao].tempo_execucao = 0;
            }

            while(k < iterador){
                if (listaP[k].prioridade > 0){
                listaP[k].latencia += clock;
                }
                k++;
            }
         
            if(listaP[posicao].tempo_execucao <= 0){ //Processo encerrou

                listaP[posicao].prioridade = 0;          
            }

            printf("Id: %d\nTempo de execucao: %d\nPrioridade: %d\nLatencia: %d\nqtdMemoria: %d\n", 
            listaP[posicao].id, 
            listaP[posicao].tempo_execucao, 
            listaP[posicao].prioridade, 
            listaP[posicao].latencia,
            listaP[posicao].qtdMemoria);

            // Aplicacao do algoritmo de gerenciamento de memoria FIFO
            FIFO(listaP, posicao, tamanhoMemoria);

            pthread_mutex_unlock(&mutex_prioridade); 

            printf("\n");
            sleep(1);
        }
        else{
            if ( listaP[iterador-1].id == -1 ){
                //printf("Thread executar encerrou \n");
                break;
            }
            printf("Todos os processos foram executados. Deseja encerrar? S \n");
            sleep(3);
        }
    }
}

//Função que recebe novos processos e armazena numa lista. Durante o procedimento de adicionar um novo processo ele "trava" a função executando_processos usando um mutex.
void *recebe_novos_processos(void* arg){
    char resposta, linha[50];
    int teste = 0;
    
    iterador = *(int*)arg;
    printf("Caso deseja inserir um novo processo siga o padrão: \nnome do processo|Id|Tempo de execução|Prioridade|QtdMemoria|Seq de Acessos\n");

    while (true){
        fgets(linha, sizeof(linha), stdin);

        pthread_mutex_lock(&mutex_prioridade);  

        iterador++;

        listaP = realloc(listaP, iterador * sizeof(DadosProcessos));
        
        char nome[50];
        int id, tempo, prioridade, qtdMemoria;
        char* sequencia_str;
        
        // Tenta ler os 5 campos principais
        int result = sscanf(linha, "%[^|]|%d|%d|%d|%d|%m[^\n]", nome, &id, &tempo, &prioridade, &qtdMemoria, &sequencia_str); 
        
        if (result == 6) {  // Certifique-se de que 6 campos foram lidos corretamente
            strcpy(listaP[iterador - 1].nome_processo, nome);
            listaP[iterador - 1].id = id;
            listaP[iterador - 1].tempo_execucao = tempo;
            listaP[iterador - 1].prioridade = prioridade;
            listaP[iterador - 1].qtdMemoria = qtdMemoria;
            listaP[iterador - 1].latencia = 0;
            listaP[iterador - 1].tamanho_sequencia = 0;

            // Lê a sequência de acessos se existir
            if (sequencia_str != NULL) {
                char *token = strtok(sequencia_str, " ");
                while (token != NULL) {
                    listaP[iterador - 1].sequencia[listaP[iterador - 1].tamanho_sequencia++] = atoi(token);
                    token = strtok(NULL, " ");
                }
                free(sequencia_str);  // Libera a memória alocada pelo sscanf
            }

            // Exibe o processo adicionado
            printf("Novo processo adicionado: %s\n", listaP[iterador - 1].nome_processo);
            printf("Id: %d \n", listaP[iterador - 1].id);
            printf("Clock: %d \n", listaP[iterador - 1].tempo_execucao);
            printf("Prioridade: %d \n", listaP[iterador - 1].prioridade);
            printf("QtdMemoria: %d \n", listaP[iterador - 1].qtdMemoria);
            printf("Sequência de Acessos: ");
            for (int i = 0; i < listaP[iterador - 1].tamanho_sequencia; i++) {
                printf("%d ", listaP[iterador - 1].sequencia[i]);
            }
            printf("\n\n");
        }
        else {
            int result = sscanf(linha, "%s", nome);

            if(result == 1 && strcmp(nome, "s") == 0){
                listaP[iterador - 1].id = -1;
                //printf("Thread Adiconar encerrou \n");
                break;
            }
        }
        pthread_mutex_unlock(&mutex_prioridade);
    }
}
        


int main() {
    // Alocar memória para a lista de processos
    listaP = (DadosProcessos *)malloc(MAX_PROCESSES * sizeof(DadosProcessos));
    if (listaP == NULL) {
        perror("Erro ao alocar memória para a lista de processos");
        return EXIT_FAILURE;
    }

    // Chamar a função para ler processos do arquivo
    int numeroProcessos = read_process("entradaMemoria.txt", listaP);
    if (numeroProcessos == -1) {
        free(listaP);
        return EXIT_FAILURE;
    }

    // Imprime os valores dos processos
    show_process(listaP, numeroProcessos);

    printf("\nAlgoritmo FIFO\n");
    iterador = numeroProcessos;

    pthread_t executando_processo, lendo_novo_processo;
    pthread_mutex_init(&mutex_prioridade, NULL);

    pthread_create(&lendo_novo_processo, NULL, &recebe_novos_processos, &iterador);
    pthread_create(&executando_processo, NULL, &executando_processos, &clockCPU);

    pthread_join(executando_processo, NULL);
    pthread_cancel(lendo_novo_processo);

    pthread_mutex_destroy(&mutex_prioridade);

    free(listaP);
    free(oldestProcess);

    return 0;
}
