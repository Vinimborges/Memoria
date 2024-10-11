#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>

//Dados dos processos
typedef struct{
    char nome_processo[50];
    int id,
        prioridade,
        tempo_execucao,
        latencia,
        qtdMemoria;
    int *seqAcessoPP;  // Ponteiro para a sequência de acessos por página
}DadosProcessos;

char politicaDeMemoria[50];
int tamanhoMemoria, 
    tamanhoPagina,
    tamanhoMemoria, 
    percentualAlocacao,
    acessoPorCiclo;

pthread_mutex_t mutex_prioridade; //Criando um mutex
DadosProcessos *listaP = NULL; //Criando a lista onde os processos vão ser armazenados.
int iterador; //Iterador para contar o número de processos.


//Função utilizada para contar os processos. Retorna o número de linhas (a partir da segunda) do arquivo txt. 
int conta_processos(){   
    char linha[50];
    FILE *arquivo_1 = fopen("entradaMemoria.txt", "r");

    int numero_processos = 0, controlador = 0;
 
    while (fgets(linha, sizeof(linha), arquivo_1) != NULL){
        if (controlador == 0){ 
            controlador++;
        }
        else{
           numero_processos++;
        }
    }   

    fclose(arquivo_1);
    return numero_processos;
    
}  


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

int contar_acessos(char *linha) {
    int contador = 0;
    char *token = strtok(linha, " ");
    
    // Conta os tokens (números separados por espaços)
    while (token != NULL) {
        contador++;
        token = strtok(NULL, " ");
    }
    return contador;
}

//Função que recebe novos processos e armazena numa lista. Durante o procedimento de adicionar um novo processo ele "trava" a função executando_processos usando um mutex.
void *recebe_novos_processos(void* arg){

    char resposta, linha[50];
    int teste = 0;
    
    iterador = *(int*)arg;
    printf("Caso deseja inserir um novo processo siga o padrão: nome do processo|Id|Tempo de execução|Prioridade\n");

    while (true){
        fgets(linha, sizeof(linha), stdin);

        pthread_mutex_lock(&mutex_prioridade);  

        iterador++;

        listaP = realloc(listaP, iterador * sizeof(DadosProcessos));
        
        char nome[50];
        int id , tempo, prioridade;
        int result = sscanf(linha, "%[^|]|%d|%d|%d", nome, &id, &tempo, &prioridade); 
        
        if (result == 4) { 
            strcpy(listaP[iterador - 1].nome_processo, nome);
            listaP[iterador - 1].id = id;
            listaP[iterador - 1].tempo_execucao = tempo;
            listaP[iterador - 1].prioridade = prioridade;
            listaP[iterador - 1].latencia = 0;

            printf("Novo processo adicionado: %s\n", listaP[iterador - 1].nome_processo);
            printf("Id: %d \n", listaP[iterador - 1].id);
            printf("Clock: %d \n", listaP[iterador - 1].tempo_execucao);
            printf("Prioridade: %d \n\n", listaP[iterador - 1].prioridade);
        
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
        
//Função que recebe a lista de processos e executa-os. Durante o procedimento de executar um novo processo ele "trava" a função recebe_novos_processos usando um mutex.
void *executando_processos(void* arg){
    
    int latencia = 0, id_processo_anterior;

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

            printf("Id do processo:%d\nTempo de execucao do processo:%d\nLatencia do processo:%d\n", 
            listaP[posicao].id, 
            listaP[posicao].tempo_execucao, 
            listaP[posicao].latencia);

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

//Fução main que rebece que inicia as threads, mutex, recebe os valores do arquivo txt. No final libera memória da lista, encerra as threads e destroi o mutex.
void escalonadorPrioridade(){
    char linha[50];
    int controlador = 0, numero_processos = 0, i = 0;
    int clock;
    numero_processos = conta_processos();
    printf("Numero de Processo: %d\n",numero_processos);

    listaP = malloc(numero_processos * sizeof(DadosProcessos)); //lista de processos

    FILE *arquivo_2 = fopen("entradaMemoria.txt", "r");

    while (fgets(linha, sizeof(linha), arquivo_2) != NULL){
        if (controlador == 0){ 
            strtok(linha, "|");
            clock = atoi(strtok(NULL, "|"));
            strcpy(politicaDeMemoria,strtok(NULL, "|"));
            tamanhoMemoria = atoi(strtok(NULL, "|"));
            tamanhoPagina = atoi(strtok(NULL, "|"));
            percentualAlocacao = atoi(strtok(NULL, "|"));
            acessoPorCiclo = atoi(strtok(NULL, "|"));
            controlador++;
            // printf("Clock: %s\n",clock);
            // printf("Politica de Memoria: %s\n",politicaDeMemoria);
            // printf("Tamanho Memoria: %d\n",tamanhoMemoria);
            // printf("Tamanho Pagina: %d\n",tamanhoPagina);
            // printf("Percentual de Alocação: %d\n",percentualAlocacao);
            // printf("Acesso por ciclo: %d\n",acessoPorCiclo);
            // até aqui tudo certo

        }
        else{          
            strcpy(listaP[i].nome_processo, strtok(linha, "|"));
            listaP[i].id = atoi(strtok(NULL, "|"));
            listaP[i].tempo_execucao = atoi(strtok(NULL, "|"));
            listaP[i].prioridade = atoi(strtok(NULL, "|"));
            // listaP[i].latencia = 0;
            listaP[i].qtdMemoria = atoi(strtok(NULL, "|"));

            printf("Nome Processo: %s\n",listaP[i].nome_processo);
            printf("id: %d\n",listaP[i].id);
            printf("Tempo de execucao: %d\n",listaP[i].tempo_execucao);
            printf("Prioridade: %d\n",listaP[i].prioridade);
            printf("Qtd Memoria: %d\n", listaP[i].qtdMemoria);

            char *acessos = strtok(NULL, "|"); // Pega a string dos acessos após a qtdMemoria
j
            // Inicializa contador
            int contador = 0;
            int numero;

            // Percorrer a string dos acessos até encontrar uma quebra de linha
            if (acessos != NULL) {  // Verifica se acessos não é NULL
                char *token = strtok(acessos, " \n");  // Usar espaço e quebra de linha como delimitadores

                while (token != NULL) {
                    numero = atoi(token);  // Converter o token para um número inteiro
                    printf("Número: %d\n", numero);  // Apenas para exibir o número
                    contador++;  // Incrementar o contador
                    token = strtok(NULL, " \n");  // Continuar a ler a próxima parte
                }
            }

            // Exibe o total de acessos contados
            printf("Total de acessos contados: %d\n", contador);


            // char *seqAcessos = strtok(NULL, "|");
            // char *copiaAcessos = strdup(seqAcessos);

            // int numero_acessos = contar_acessos(copiaAcessos);
            // free(copiaAcessos);
            
            // listaP[i].seqAcessoPP = malloc(numero_acessos * sizeof(int));
            // if (listaP[i].seqAcessoPP == NULL) {
            //     fprintf(stderr, "Erro ao alocar memória para seqAcessoPP\n");
            //     exit(EXIT_FAILURE);
            // }

            // int j = 0;
            // char *token = strtok(seqAcessos, " ");
            // while (token != NULL) {
            //     listaP[i].seqAcessoPP[j] = atoi(token);
            //     token = strtok(NULL, " ");
            //     j++;
            // }

            // for (int j = 0; j < numero_acessos; j++) {
            //     printf("%d ", listaP[i].seqAcessoPP[j]);
            // }
            // printf("\n");
            i++;
        }
    }
    fclose(arquivo_2);

    pthread_t executando_processo, lendo_novo_processo;
    pthread_mutex_init(&mutex_prioridade, NULL);

    pthread_create(&lendo_novo_processo, NULL, &recebe_novos_processos, &numero_processos);
    pthread_create(&executando_processo, NULL, &executando_processos, &clock);

    pthread_join(executando_processo,NULL);

    pthread_cancel(lendo_novo_processo);

    pthread_mutex_destroy(&mutex_prioridade);

    criando_arquivo();

    free(listaP);
}

