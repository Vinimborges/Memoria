#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include "readFile.h"

char algDeEscalonamento[50];
int clockCPU;
char politicaDeMemoria[50];
int tamanhoMemoria;
int tamanhoPagina;
int percentualAlocacao;
int acessoPorCiclo;
int acessosNaMemoria;
int *primaryMemory = NULL;

#define MAX_LINE_LENGTH 256
#define MAX_PROCESSES 100
#define MAX_SEQ_LENGTH 100

int iterador = 0;

typedef struct {
    int id,
        start,
        end;
} ProcessInMemory;

ProcessInMemory *memoryProcesses = NULL;  // Lista de processos atualmente na memoria
ProcessInMemory *oldestProcess = NULL;    // Primeiro a entrar na fila
ProcessInMemory *newestProcess = NULL;    // Ultimo a entrar na fila
int nOfProcessesMemory = 0;

DadosProcessos *listaP; // Ponteiro para a lista de processos

//------------------------------------------- Algortimo de MEMORIA FIFO --------------------------------------------------------------------------------------------

// Imprime os processos que estão na memoria principal com a posição de inicio e fim
int print_memory_processes() {
    printf("\n --------- Processos na Memória Pricipal -------------\n");
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
    printf("------------- --- Memória Pricipal ------------------------\n");
    for(int j = 0; j < (tamanhoMemoria / tamanhoPagina); j++){
        printf("%d",primaryMemory[j]);
    }
    return 0;
}

// Adiciona valores na memoria, poderiam ser dados ao inves de numeros em ordem
void addIntoMemory(int start, int end){
    int totalMolduras = tamanhoMemoria/tamanhoPagina;
    int k = 1; 

    if(end < start ){ //significa que e circular
        for (int i = start; i < totalMolduras; i++) {
            primaryMemory[i] = k;
            k++;
        }
        for (int i = 0; i <= end; i++) {
            primaryMemory[i] = k;
            k++;
        }
    }else {
        for(int i = start; i <= end; i++){
            primaryMemory[i] = k;
            k++;
        }
    }
}

// Remove dados da memoria principal
void DeleteFromMemory(int start, int end){
    int totalMolduras = tamanhoMemoria/tamanhoPagina;
    int k = 1; 
    if(end < start ){ //significa que e circular
        for (int i = start; i < totalMolduras; i++) {
            primaryMemory[i] = 0;
        }
        for (int i = 0; i <= end; i++) {
            primaryMemory[i] = 0;
        }
    }else {
        for(int i = start; i <= end; i++){
            primaryMemory[i] = 0;
        }
    }
    printf("Memoria liberada de %d a %d\n",start,end);
}

// Verifica quantas mmolduras existem disponiveis na memoria
int availableMemory(){
    int empty = 0;
    for (int i = 0; i < (tamanhoMemoria / tamanhoPagina); i++) {
        if (primaryMemory[i] == 0) {  // Considera 0 como um espaço vazio
            empty++;
        }
    }
    return empty;
}

void initOldestProcess(int id, int start, int end) {
if (oldestProcess == NULL) {
    oldestProcess = (ProcessInMemory *)malloc(sizeof(ProcessInMemory));
    if (oldestProcess == NULL) {
        printf("Erro ao alocar memória para oldestProcess.\n");
        exit(1);
    }
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

int pageInMemory(int id, int page){
    ProcessInMemory processData = findProcessByIdData(memoryProcesses, nOfProcessesMemory, id);
    if(processData.end < processData.start){
        for (int i = processData.start; i <=  (tamanhoMemoria/tamanhoPagina); i++) {
            if (primaryMemory[i] == page) {  
                return 1;
            }
        }
        for (int i = 0; i <=  processData.end; i++) {
            if (primaryMemory[i] == page) {  
                return 1;
            }
        }
    }else{
        for (int i = processData.start; i <= processData.end; i++) {
            if (primaryMemory[i] == page) {  
                return 1;
            }
        }
    }
    printf("Pagina %d nao esta na memoria\n",page);
    return 0;
}

void FIFO(DadosProcessos *listaP, int posicao, int tamanhoMemoria) {
    int molduras = ((listaP[posicao].qtdMemoria / tamanhoPagina * percentualAlocacao) / 100);
    int totalMolduras = tamanhoMemoria/tamanhoPagina;

    if (!findProcessById(memoryProcesses, nOfProcessesMemory, listaP[posicao].id)) { // Caso o processo não esteja na memória
        if ((tamanhoMemoria - (listaP[posicao].qtdMemoria * percentualAlocacao) / 100) >= 0) {  // Verifica se o tamanho necessário do processo cabe na memória com o percentual de alocação
            if (memoryProcesses == NULL) {
                ProcessInMemory process;
                process.id = listaP[posicao].id;
                process.start = 0;
                process.end = molduras - 1;

                ProcessInMemory *temp = realloc(memoryProcesses, (nOfProcessesMemory + 1) * sizeof(ProcessInMemory));
                if (temp == NULL) {
                    perror("Erro ao realocar memória");
                    exit(EXIT_FAILURE);
                }
                memoryProcesses = temp;

                initOldestProcess(process.id, process.start, process.end); // Armazena o primeiro processo adicionado
                initNewestProcess(process.id, process.start, process.end); // Armazena o último processo adicionado
                memoryProcesses[nOfProcessesMemory] = process;              // Adiciona na lista de processos na memória
                nOfProcessesMemory++;

                addIntoMemory(process.start, process.end);
            } else {
                if (availableMemory() >= (molduras)) { // Não precisa remover nenhum processo
                    ProcessInMemory process;
                    process.id = listaP[posicao].id;
                    process.start = newestProcess->end + 1;
                    process.end = process.start + molduras - 1;
                        // verificar se esta circular

                    initNewestProcess(process.id, process.start, process.end); // Atualiza o último processo
                    memoryProcesses = realloc(memoryProcesses, (nOfProcessesMemory + 1) * sizeof(ProcessInMemory));
                    memoryProcesses[nOfProcessesMemory] = process;              // Adiciona na lista de processos na memória
                    nOfProcessesMemory++;
                    addIntoMemory(process.start, process.end);
                } else {
                    printf("\n-Sem espaço na memória\n ");

                    if (strcmp(politicaDeMemoria, "global") == 0) {
                        printf("-Removendo processo mais antigo (ID: %d) para liberar espaço.\n", oldestProcess->id);

                        // Remover o processo mais antigo
                        ProcessInMemory *temp = malloc((nOfProcessesMemory - 1) * sizeof(ProcessInMemory)); //menos um processo na memoria
                        if (temp == NULL) {
                            perror("Erro ao realocar memória");
                            exit(EXIT_FAILURE);
                        }
                        
                        // Copia todos os processos, exceto o mais antigo
                        int j = 0;
                        for (int i = 0; i < nOfProcessesMemory; i++) {
                            if (memoryProcesses[i].id != oldestProcess->id) {
                                temp[j] = memoryProcesses[i];
                                j++;
                            }
                        }
                        // DeleteFromProcessesMemory(nOfProcessesMemory);
                        DeleteFromMemory(oldestProcess->start,oldestProcess->end);
                        int lastOldestID = oldestProcess->id;

                        free(memoryProcesses);  // Libera a memória antiga
                        memoryProcesses = temp; // Remove o processo mais antigo da memoria
                        nOfProcessesMemory--;

                        // Caso oldest id seja igual a zero newest tbm precisa ser zero
                        if (newestProcess->id == oldestProcess->id){
                            initNewestProcess(-1, 0, 0);
                        }

                        ProcessInMemory process;
                        process.id = listaP[posicao].id;
                        printf("%d %d\n",lastOldestID,newestProcess->id );
                        if(newestProcess->end == 0 && newestProcess->id == -1){
                            process.start = newestProcess->end ;
                            printf("tem que ser zero\n");
                        }else {
                            process.start = newestProcess->end + 1;
                        }
                        process.end = (process.start + molduras - 1) % totalMolduras;

                        initNewestProcess(process.id, process.start, process.end);
                        // printf("\n ------------------nOfProcessesMemory %d\n", nOfProcessesMemory);
                        if(nOfProcessesMemory == 0){
                            initOldestProcess(process.id, process.start, process.end);
                        }else{
                            // printf("New Oldest ID %d\n",oldestProcess->id);
                            *oldestProcess = memoryProcesses[0];
                        }

                        memoryProcesses = realloc(memoryProcesses, (nOfProcessesMemory + 1) * sizeof(ProcessInMemory));
                        memoryProcesses[nOfProcessesMemory] = process;
                        nOfProcessesMemory++;
                        addIntoMemory(process.start, process.end);
                        
                    }
                }
            }
        }
    }

    print_memory_processes();
    print_primary_memory();

    ProcessInMemory processData = findProcessByIdData(memoryProcesses, nOfProcessesMemory, listaP[posicao].id);
    
    printf("\nAcesso a memoria: ");
    for(int i = listaP[posicao].estadoSequencia; i < (listaP[posicao].estadoSequencia + acessosNaMemoria); i++){
        int page = listaP[posicao].sequencia[i];
        if(page != 0 && pageInMemory(listaP[posicao].id,page)){
            printf("%d ",page);
        }
    }
    listaP[posicao].estadoSequencia += acessosNaMemoria;
    printf("\n");
    // TUDO CERTO ATE AQUI
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

            printf("\nPROCESSO ID %d NA CPU\nTempo de execucao: %d\nPrioridade: %d\nLatencia: %d\nqtdMemoria: %d\n", 
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
    DadosProcessos *listaP = malloc(sizeof(DadosProcessos) * MAX_PROCESSES);
    if (listaP == NULL) {
        perror("Erro ao alocar memória para a lista de processos");
        return EXIT_FAILURE;
    }

    // Chamar a função para ler processos do arquivo
    // int numeroProcessos = read_process("entradaMemoria.txt", listaP);
    // if (numeroProcessos == -1) {
    //     free(listaP);
    //     return EXIT_FAILURE;
    // }
    int numeroProcessos = 5;
    // Imprime os valores dos processos
    show_process(listaP, numeroProcessos);

    printf("\nAlgoritmo FIFO\n");
    int iterador = numeroProcessos;

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
