#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 256
#define MAX_PROCESSES 100
#define MAX_SEQ_LENGTH 100

int iterador = 0;

// Dados dos processos
typedef struct{
    char nome_processo[50];
    int id,
        prioridade,
        tempo_execucao,
        qtdMemoria,
        sequencia[MAX_SEQ_LENGTH],
        tamanho_sequencia,
        estadoSequencia,
        latencia,
        trocasDePaginas,
        moldurasUsadas;
} DadosProcessos;

// Variaveis globais
char algDeEscalonamento[50];
char politicaDeMemoria[50];
int clockCPU, 
    tamanhoMemoria, 
    tamanhoPagina, 
    percentualAlocacao, 
    acessoPorCiclo, 
    acessosNaMemoria,
    moldurasTotais;

DadosProcessos *listaP = NULL; // Ponteiro para a lista de processos

//------------------------------------------- Algortimo de MEMORIA FIFO --------------------------------------------------------------------------------------------
typedef struct {
    int id,
        page,
        localPosition,
        globalPosition;
} Memory;

Memory *primaryMemory = NULL; // Ponteiro para a lista de processos
int nOfPagesInMemory = 0;
int trocasDePaginas = 0;

// Imprime o conteudo da memoria principal
void print_primary_memory() {
    printf("|----------------------------------------------------------------|\n");
    printf("|--------------------------- Memória Pricipal -------------------|\n");
    for (int i = 0; i < moldurasTotais; i++){
        printf("Posicao: %d | ID: %d | Page: %d | localPosition %d | globalPosition %d\n",i,primaryMemory[i].id, primaryMemory[i].page, primaryMemory[i].localPosition, primaryMemory[i].globalPosition);
    }
}

void initMemory(){
    printf("Memoria inicialmente\n");
    for (int i = 0; i < moldurasTotais; i++){
        primaryMemory[i].id = -1;
        primaryMemory[i].page = -1;
        primaryMemory[i].localPosition = -1;
        primaryMemory[i].globalPosition = -1;
    }
    print_primary_memory();
}

int pageInMemory(int id, int page){
    for (int i = 0; i < moldurasTotais; i++){
        if(primaryMemory[i].id == id && primaryMemory[i].page == page ){
            return 1;
        }
    }
    return 0;
}

int processInMemory(int id){
    for (int i = 0; i < moldurasTotais; i++){
        if(primaryMemory[i].id == id){
            return 1;
        }
    }
    return 0;
}

int LocalPriority(int id){  // Percorre a memoria e calcula quantos locais ja tem aquele processo
    int count = 1;
    for (int i = 0; i < moldurasTotais; i++){
        if(primaryMemory[i].id == id){
            count++;
        }
    }
    // printf("Prioridade do processo: %d \n",count);
    return count;
}

int freePosition(){  // Percorre a memoria e calcula quantos locais ja tem aquele processo
    for (int position = 0; position < moldurasTotais; position++){
        if(primaryMemory[position].id == -1){
            // printf("free: %d \n", position);
            return position;
        }
    }
    return -1;
}

void reordenaLocalPrioridade(int id){
    for (int i = 0; i < moldurasTotais; i++){
        if(primaryMemory[i].id == id){
            primaryMemory[i].localPosition--;
        }
    }
}

void reordenaGlobalPrioridade(int posicao){
    for (int i = 0; i < moldurasTotais; i++){
        if(primaryMemory[i].id != -1 && primaryMemory[i].globalPosition >= posicao){
            primaryMemory[i].globalPosition--;
        }
    }
}

void addLocalMemory(int id, int page, int posicao){
    int pos = LocalPriority(id); // Calcula a prioridade do novo processo

    reordenaLocalPrioridade(id); // Diminui uma prioridade de todos 
    pos--;
    
    reordenaGlobalPrioridade(posicao); // Diminui uma prioridade de todos 

    int oldpage = primaryMemory[posicao].page;
    primaryMemory[posicao].page = page;
    primaryMemory[posicao].localPosition = pos;
    primaryMemory[posicao].globalPosition = nOfPagesInMemory;
    printf("Substituido página %d por pagina %d do processo %d na posição %d da memória\n", oldpage, page, id, posicao);
}

void libera_memoria(int id){
    for (int i = 0; i < moldurasTotais; i++){
        if(primaryMemory[i].id == id){
            primaryMemory[i].id = -1;
            primaryMemory[i].page = -1;
            primaryMemory[i].localPosition = -1;
            primaryMemory[i].globalPosition = -1;
            nOfPagesInMemory--;
        }
    }
}

int addPageIntoMemory(int id, int page){
    int freeP = freePosition();
    int pos = LocalPriority(id); 
    nOfPagesInMemory++;

    printf("Adicionando página %d do processo %d na posição %d da memória\n", page, id, freeP);
    primaryMemory[freeP].id = id;
    primaryMemory[freeP].page = page;
    primaryMemory[freeP].localPosition = pos;
    primaryMemory[freeP].globalPosition = nOfPagesInMemory;
    return freeP; //retorna a posicao alocada
}

int findOldestLocal(int id) {
    int current = -1;
    int smallest = 0;
    int key = 0;
    for (int i = 0; i < moldurasTotais; i++){
        if(primaryMemory[i].id == id){
            current = primaryMemory[i].localPosition;
            if(!key){
                key = 1;
                smallest = primaryMemory[i].localPosition;
            }else{
                if(current < smallest){
                    smallest = current;
                }   
            }
        }
    }
    for (int i = 0; i < moldurasTotais; i++){
        if(primaryMemory[i].id == id && primaryMemory[i].localPosition == smallest){
            return i; // retorna a posicao desse com menor prioridade (mais velho)
        }
    }

    return -1;
}

int findOldestGlobal() {
    int current = -1;
    int smallest = primaryMemory[0].globalPosition;
    int pos = 0;
    for (int i = 1; i < moldurasTotais; i++){
        current = primaryMemory[i].globalPosition;
        if(current < smallest){
            smallest = current;
            pos = i;
        }
    }

    return pos;
}

void addGlobalMemory(int id, int page, int pos,int posicaoP){
    int posicaoOldProcess;
    int newPosition = 1;
    for (int i = 0; i < moldurasTotais; i++){
        if(i == pos){
            posicaoOldProcess = i;
        }
        if(id == primaryMemory[i].id){
            newPosition++;
        }
    }
    int oldId = primaryMemory[posicaoOldProcess].id;
    for (int i = 0; i < MAX_PROCESSES; i++){
        if(oldId == listaP[i].id){
            listaP[i].moldurasUsadas--; //Libera uma moldura do processo que estava naquela posicao
            // printf("id %d mold usadas %d\n",listaP[i].id, listaP[i].moldurasUsadas); //Libera uma moldura do processo que estava naquela posicao
        }
    }

    primaryMemory[pos].id = id;
    primaryMemory[pos].page = page;
    primaryMemory[pos].localPosition = newPosition;
    primaryMemory[pos].globalPosition = nOfPagesInMemory;
    listaP[posicaoP].moldurasUsadas++;

    for (int i = 0; i < moldurasTotais; i++){
        if(i != pos){
            primaryMemory[i].globalPosition--;
        }
        if (oldId == primaryMemory[i].id){
            primaryMemory[i].localPosition--;
        }
    }
}

void politicaLocal(int id, int page){
    int pos = findOldestLocal(id);
    printf("pos oldest local %d \n",pos);
    addLocalMemory(id,page,pos);
}

void politicaGlobal(int id, int page,int posicaoP){
    int pos = findOldestGlobal(id);
    addGlobalMemory(id,page,pos,posicaoP);
}


void FIFO(DadosProcessos *listaP, int posicao) {
    int id = listaP[posicao].id;
    int qtdMemoria = listaP[posicao].qtdMemoria;
    int moldurasDisponiveis = (((qtdMemoria / tamanhoPagina) * percentualAlocacao) / 100);
    if (moldurasDisponiveis > moldurasTotais){
        moldurasDisponiveis = moldurasTotais;
    }
    // Caso nao esteja na memoria, o processo sera adicionado
    printf("\nSequencia de acessos a memoria: ");
    for(int i = listaP[posicao].estadoSequencia; i < (listaP[posicao].estadoSequencia + acessosNaMemoria); i++){
        int page = listaP[posicao].sequencia[i];
        if(page != -1 && pageInMemory(id,page)){
            printf("%d ",page);
        } 
        else if(page != -1 && page!= 0 && !pageInMemory(id,page)){ // Pagina nao esta na memoria
            printf("\nPagina %d nao esta na memoria\n",page);
            if(!processInMemory(id)){
                if (freePosition() != -1){
                    int posicaoAlocada = addPageIntoMemory(id,page);
                    listaP[posicao].moldurasUsadas++;
                    print_primary_memory();
                }else{
                    printf("Não é possível armazenar Memoria cheia\n");
                }
            }
            else{ //Processo ja esta na memoria
                if(listaP[posicao].moldurasUsadas < moldurasDisponiveis && freePosition() != -1){// Aloca em um novo espaco
                    int posicaoAlocada = addPageIntoMemory(id,page);
                    listaP[posicao].moldurasUsadas++;
                }
                else{  // Memoria cheia e precisa de troca de pagina
                    if (strcmp(politicaDeMemoria, "local") == 0) {
                        if(!processInMemory(id) && freePosition() == -1){
                            printf("Não é possível armazenar\n");
                        }else{
                            politicaLocal(id,page);
                            listaP[posicao].trocasDePaginas++;
                        }
                    }else{
                        if(listaP[posicao].moldurasUsadas >= moldurasDisponiveis){ //se torna local
                            politicaLocal(id,page); //Deve SER LOCAL AGORA
                            listaP[posicao].trocasDePaginas++;
                        }else{ 
                            politicaGlobal(id,page,posicao);
                            listaP[posicao].trocasDePaginas++;
                        }
                    }
                }
                print_primary_memory();
            }
        }
    }
    listaP[posicao].estadoSequencia += acessosNaMemoria;
    printf("\n");
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
            //Variaveis globais
            strcpy(algDeEscalonamento, strtok(line, "|"));
            clockCPU = atoi(strtok(NULL, "|"));
            strcpy(politicaDeMemoria, strtok(NULL, "|"));
            tamanhoMemoria = atoi(strtok(NULL, "|"));
            tamanhoPagina = atoi(strtok(NULL, "|"));
            percentualAlocacao = atoi(strtok(NULL, "|"));
            acessoPorCiclo = atoi(strtok(NULL, "|"));
            
            acessosNaMemoria = clockCPU * acessoPorCiclo;
            moldurasTotais = tamanhoMemoria / tamanhoPagina;
            controlador++;

            int tamanhoUsado = tamanhoMemoria * (percentualAlocacao / 100);
            // Alocar memória para a lista de processos
            primaryMemory = (Memory *)malloc(moldurasTotais * sizeof(Memory));

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
            listaP[i].moldurasUsadas = 0;

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

            printf("\nPROCESSO ID %d NA CPU\nTempo de execucao: %d\nPrioridade: %d\nLatencia: %d\nqtdMemoria: %d\n", 
            listaP[posicao].id, 
            listaP[posicao].tempo_execucao, 
            listaP[posicao].prioridade, 
            listaP[posicao].latencia,
            listaP[posicao].qtdMemoria);

            // Aplicacao do algoritmo de gerenciamento de memoria FIFO
            FIFO(listaP, posicao);

            if (listaP[posicao].prioridade == 0){
                // libera_memoria(listaP[posicao].id);
                printf("Trocas no processo %d : %d \n",listaP[posicao].id, listaP[posicao].trocasDePaginas);
                trocasDePaginas += listaP[posicao].trocasDePaginas;
            }

            pthread_mutex_unlock(&mutex_prioridade); 

            printf("\n");
            sleep(1);
        }
        else{
            if ( listaP[iterador-1].id == -1 ){
                //printf("Thread executar encerrou \n");
                break;
            }
            printf("Trocas total de paginas: %d \n",trocasDePaginas);
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

// Função para ler o arquivo e preencher os valores dos algoritmos
void lerArquivoEAtualizar(const char *algoritmo_atual, int trocas_pagina_atual) {
    FILE *arquivo = fopen("resultados.txt", "r+");
    char linha[MAX_LINE_LENGTH];
    int otimo = -1, fifo = -1, nuf = -1, mru = -1;
    char *token;

    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo.\n");
        return;
    }

    // Ler a linha do arquivo que contém os resultados
    if (fgets(linha, MAX_LINE_LENGTH, arquivo) != NULL) {
        // Quebrar a linha em tokens para identificar os valores de cada algoritmo
        token = strtok(linha, "|");

        while (token != NULL) {
            if (strstr(token, "OTIMO:") != NULL) {
                sscanf(token, "OTIMO: %d", &otimo);
            } else if (strstr(token, "FIFO:") != NULL) {
                sscanf(token, "FIFO: %d", &fifo);
            } else if (strstr(token, "NUF:") != NULL) {
                sscanf(token, "NUF: %d", &nuf);
            } else if (strstr(token, "MRU:") != NULL) {
                sscanf(token, "MRU: %d", &mru);
            }
            token = strtok(NULL, "|");
        }

        // Atualizar o valor do algoritmo atual, sem modificar os outros
        if (strcmp(algoritmo_atual, "FIFO") == 0) {
            fifo = trocas_pagina_atual;
        } else if (strcmp(algoritmo_atual, "NUF") == 0) {
            nuf = trocas_pagina_atual;
        } else if (strcmp(algoritmo_atual, "MRU") == 0) {
            mru = trocas_pagina_atual;
        } else if (strcmp(algoritmo_atual, "OTIMO") == 0) {
            otimo = trocas_pagina_atual;
        }
    }

    // Reescrever os valores atualizados na primeira linha do arquivo
    rewind(arquivo);
    fprintf(arquivo, "OTIMO: %d |FIFO: %d |NUF: %d |MRU: %d\n", 
            otimo != -1 ? otimo : -1, 
            fifo != -1 ? fifo : -1, 
            nuf != -1 ? nuf : -1, 
            mru != -1 ? mru : -1);

    // Comparar os valores para encontrar o algoritmo mais próximo de OTIMO
    int dif_fifo = (fifo != -1) ? abs(fifo - otimo) : -1;
    int dif_nuf = (nuf != -1) ? abs(nuf - otimo) : -1;
    int dif_mru = (mru != -1) ? abs(mru - otimo) : -1;

    int menor_diferenca = -1;
    const char *algoritmo_mais_proximo = NULL;

    // Definir o algoritmo mais próximo e verificar empates
    if (dif_fifo != -1) {
        menor_diferenca = dif_fifo;
        algoritmo_mais_proximo = "FIFO";
    }
    if (dif_nuf != -1 && (menor_diferenca == -1 || dif_nuf < menor_diferenca)) {
        menor_diferenca = dif_nuf;
        algoritmo_mais_proximo = "NUF";
    } else if (dif_nuf == menor_diferenca) {
        algoritmo_mais_proximo = "EMPATE";
    }
    if (dif_mru != -1 && (menor_diferenca == -1 || dif_mru < menor_diferenca)) {
        menor_diferenca = dif_mru;
        algoritmo_mais_proximo = "MRU";
    } else if (dif_mru == menor_diferenca && strcmp(algoritmo_mais_proximo, "EMPATE") != 0) {
        algoritmo_mais_proximo = "EMPATE";
    }

    // Escrever a segunda linha no arquivo com o resultado
    fprintf(arquivo, "Algoritmo mais próximo do OTIMO: %s\n", algoritmo_mais_proximo);

    fclose(arquivo);

    printf("Arquivo atualizado com sucesso.\n");
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

    char algoritmo_atual[] = "FIFO";

    // Imprime os valores dos processos
    show_process(listaP, numeroProcessos);

    iterador = numeroProcessos;

    initMemory();

    pthread_t executando_processo, lendo_novo_processo;
    pthread_mutex_init(&mutex_prioridade, NULL);

    pthread_create(&lendo_novo_processo, NULL, &recebe_novos_processos, &iterador);
    pthread_create(&executando_processo, NULL, &executando_processos, &clockCPU);

    pthread_join(executando_processo, NULL);
    pthread_cancel(lendo_novo_processo);

    // Saida de troca de paginas
    lerArquivoEAtualizar(algoritmo_atual,trocasDePaginas);

    pthread_mutex_destroy(&mutex_prioridade);

    free(listaP);
    // free(oldestProcess);

    return 0;
}
