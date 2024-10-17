#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include "escalonadorPrioridade.h"

#define TAM_LINHA 500
#define PROCESSOS 100
#define TAM_SEQUENCIA 100


// Dados dos processos
typedef struct{
    char nome_processo[50];
    int id,
        tempo_execucao,
        prioridade,
        qtd_memoria,
        sequencia[TAM_SEQUENCIA],
        tamanho_sequencia,
        pos_ini_memoria,
        pos_fim_memoria,
        na_memoria,
        latencia;
} DadosProcessos;

typedef struct{
    int numero_pagina,  
        id_processo,   
        ultimo_acesso;
} Paginas;


pthread_mutex_t mutex_PR;

char alg_escalonamento[50];
char politica_memoria[50];
int clock_cpu, tam_memoria, tam_pagina, percentual_alocacao, acesso_por_ciclo;
int iterador = 0, tempo_atual = 0, posicao_ocuapada_inicial = 0, posicao_ocupada_final = 0;
int molduras_memoria = tam_memoria / tam_pagina;




DadosProcessos *lista_processos; // Ponteiro para a lista de processos
Paginas *paginas_na_memoria;

//------------------------------------------- LEITURA DO ARQUIVO DE ENTRADA E MANIPULACAO --------------------------------------------------------------------------------------------
// Função para ler os processos do arquivo
int recebe_processos(const char *nome_arquivo, DadosProcessos *lista_processos) {
    FILE *file = fopen(nome_arquivo, "r");

    int numero_processos = 0, controlador = 0, i = 0;
    char line[TAM_LINHA];
  
    while (fgets(line, sizeof(line), file) != NULL) {

        if (controlador == 0) {
            // Ler as informações de configuração
            strcpy(alg_escalonamento, strtok(line, "|"));
            clock_cpu = atoi(strtok(NULL, "|"));
            strcpy(politica_memoria, strtok(NULL, "|"));
            tam_memoria = atoi(strtok(NULL, "|"));
            tam_pagina = atoi(strtok(NULL, "|"));
            percentual_alocacao = atoi(strtok(NULL, "|"));
            acesso_por_ciclo = atoi(strtok(NULL, "|"));
            controlador++;

            printf("Memoria Principal com capacidade de %d bytes\n",tam_memoria);

        } else {
            // Processar informações de cada processo
            strcpy(lista_processos[i].nome_processo, strtok(line, "|"));
            lista_processos[i].id = atoi(strtok(NULL, "|"));
            lista_processos[i].tempo_execucao = atoi(strtok(NULL, "|"));
            lista_processos[i].prioridade = atoi(strtok(NULL, "|"));
            lista_processos[i].qtd_memoria = atoi(strtok(NULL, "|"));
            lista_processos[i].latencia = 0;

            // Ler e armazenar a sequência
            char *sequencia_str = strtok(NULL, "|");
            char *token = strtok(sequencia_str, " ");
            lista_processos[i].tamanho_sequencia = 0;

            while (token != NULL) {
                lista_processos[i].sequencia[lista_processos[i].tamanho_sequencia++] = atoi(token);
                token = strtok(NULL, " ");
            }

            i++;
            numero_processos++;
        }
    }

    fclose(file);
    return numero_processos;
}

void imprimir_processos(DadosProcessos *lista_processos, int numeroProcessos) {
    for (int j = 0; j < numeroProcessos; j++) {
        printf("\nProcesso: %s\n", lista_processos[j].nome_processo);
        printf("ID: %d\n", lista_processos[j].id);
        printf("Tempo de Execução: %d\n", lista_processos[j].tempo_execucao);
        printf("Prioridade: %d\n", lista_processos[j].prioridade);
        printf("Qtd Memória: %d\n", lista_processos[j].qtd_memoria);
        printf("Sequência Acessos: ");
        for (int k = 0; k < lista_processos[j].tamanho_sequencia; k++) {
            printf("%d ", lista_processos[j].sequencia[k]);
        }
        printf("\n");
    }
}

void inicializar_pagina(Paginas *pagina) {
    pagina -> numero_pagina = -1; 
    pagina -> id_processo = -1;    
    pagina -> ultimo_acesso = 0;   
}

//Função para criar um arquivo txt onde estarão armazenados o Id e a Latência de cada processo.
void criando_arquivo(){
    int i = 0;
    FILE* arquivo_3 = fopen("SaidaPrioridade.txt", "w");
    fprintf(arquivo_3, "ID | LATÊNCIA\n");

    while (i < iterador){
        if (lista_processos[i].id >= 0){
            fprintf(arquivo_3, "%d | ", lista_processos[i].id);
            fprintf(arquivo_3, "%d\n", lista_processos[i].latencia);
        }
        i++;
    }
}

void MRU (int posicao, int clock_cpu){
    int num_paginas_recebidos = clock_cpu * acesso_por_ciclo;
    int contador = 0, pos_inicial = 0, pos_final = 0;
    int qtd_processo_memoria = (lista_processos[posicao].qtd_memoria * 0.75) / tam_pagina;
    int acesso = 0, menos_acessada;

    if(strcmp(politica_memoria, "local") == 0){
        if(posicao_ocupada_final + num_paginas_recebidos <= molduras_memoria - 1 && lista_processos[posicao].na_memoria == 0){ //Verifica se tem espaço contigo na memória e se o processo não está na memória.
            
            lista_processos[posicao].na_memoria = 1; // Flag para indicar que o processo está na memória.
            
            for (int i = posicao_ocupada_final; i < molduras_memoria; i++){ //Acha, na memória, as posições onde as páginas do processo vão ser alocadas.
                if (paginas_na_memoria[i].numero_pagina == -1 ){
                    contador++;
                    if (contador == 1){
                        pos_inicial = i;
                    }

                } else{
                    contador = 0;
                }

                if (contador == qtd_processo_memoria){
                    pos_final = contador;
                }
            }

            lista_processos[posicao].pos_ini_memoria = pos_inicial;
            lista_processos[posicao].pos_fim_memoria = pos_final;

            for (int j = 0; j < num_paginas_recebidos; j++){ //Pegando todos os valores que devem ser pegos durante o clock.
                int flag_subs = 0;

                for (int k = pos_inicial; k < pos_final; k++){ //Verificando se a página está na memória. Caso sim, só substitui o valor de último acesso. Caso não, salva o índice de quem teve mais tempo sem ser acessado. 
                    if(paginas_na_memoria[k].id_processo == -1){
                        paginas_na_memoria[k].id_processo = lista_processos[posicao].id;
                        paginas_na_memoria[k].numero_pagina = lista_processos[posicao].sequencia[j] ;
                        paginas_na_memoria[k].ultimo_acesso = tempo_atual + (j / 2);
                        flag_subs = 0;

                    }else if (lista_processos[posicao].id == paginas_na_memoria[k].id_processo){
                        paginas_na_memoria[k].ultimo_acesso = tempo_atual + (j / 2);
                        flag_subs = 0;

                    }else if (paginas_na_memoria[k].ultimo_acesso >= acesso){
                        acesso = paginas_na_memoria[k].ultimo_acesso;
                        menos_acessada = k;
                        flag_subs = 1;
                    }
                }
            
                if (flag_subs == 1){ //Troca a página que ficou mais tempo sem ser acessada.
                    paginas_na_memoria[menos_acessada].id_processo = lista_processos[posicao].id;
                    paginas_na_memoria[menos_acessada].numero_pagina = lista_processos[posicao].sequencia[j] ;
                    paginas_na_memoria[menos_acessada].ultimo_acesso = tempo_atual + (j / 2);
                    
                }
            }

            ocupacao_memoria();

            
        }else if(lista_processos[posicao].na_memoria == 1){ //Verifica se o processo está na memória.

                for (int j = 0; j < num_paginas_recebidos; j++){ //Pegando todos os valores que devem ser pegos durante o clock.
                    int flag_subs = 0;

                    for (int k = lista_processos[posicao].pos_ini_memoria; k < lista_processos[posicao].pos_fim_memoria; k++){ //Verificando se a página está na memória. Caso sim, só substitui o valor de último acesso. Caso não, salva o índice de quem teve mais tempo sem ser acessado. 
                        if(paginas_na_memoria[k].id_processo == -1){
                            paginas_na_memoria[k].id_processo = lista_processos[posicao].id;
                            paginas_na_memoria[k].numero_pagina = lista_processos[posicao].sequencia[j] ;
                            paginas_na_memoria[k].ultimo_acesso = tempo_atual + (j / 2);
                            flag_subs = 0;

                        }else if (lista_processos[posicao].id == paginas_na_memoria[k].id_processo){
                            paginas_na_memoria[k].ultimo_acesso = tempo_atual + (j / 2);
                            flag_subs = 0;

                        }else if (paginas_na_memoria[k].ultimo_acesso >= acesso){
                            acesso = paginas_na_memoria[k].ultimo_acesso;
                            menos_acessada = k;
                            flag_subs = 1;
                        }
                    }
                
                    if (flag_subs == 1){ //Troca a página que ficou mais tempo sem ser acessada.
                        paginas_na_memoria[menos_acessada].id_processo = lista_processos[posicao].id;
                        paginas_na_memoria[menos_acessada].numero_pagina = lista_processos[posicao].sequencia[j] ;
                        paginas_na_memoria[menos_acessada].ultimo_acesso = tempo_atual + (j / 2);
                        
                    }
                }



        }else{
            printf("\nSem espaço na memória\n");

        }
    } else{




    }

}

void ocupacao_memoria(){
    int menor = 0, maior = 0, flag_menor = 0;
    for (int i = 0; i < molduras_memoria; i++){
        if (paginas_na_memoria[i].id_processo != -1 && flag_menor == 0){
            menor = i;
            flag_menor = 1;

        }
        if (paginas_na_memoria[i].id_processo != -1 && i >= maior){
            maior = i;
        }

    }

    posicao_ocuapada_inicial = menor;
    posicao_ocupada_final = maior;

}

void libera_memoria(int posicao){
    for (int i = lista_processos[posicao].pos_ini_memoria; i < lista_processos[posicao].pos_fim_memoria; i++){
        paginas_na_memoria[i].id_processo = -1;
    }
    ocupacao_memoria();
}

//Função que recebe a lista de processos e executa-os. Durante o procedimento de executar um novo processo ele "trava" a função recebe_novos_processos usando um mutex.
void *executando_processos(void* arg){
    int latencia = 0;
    int clock_cpu = *(int*)arg;

    while(true){
        int maior_prioridade = 0, j = 0, k = 0, posicao = 0;
        while(j < iterador){
            if (maior_prioridade < lista_processos[j].prioridade && lista_processos[j].tempo_execucao > 0){
                maior_prioridade = lista_processos[j].prioridade;
                posicao = j;
            }
            j++;     
        }

       

        if (maior_prioridade > 0){
            pthread_mutex_lock(&mutex_PR);

            if (lista_processos[posicao].tempo_execucao - clock_cpu >= 0){
                lista_processos[posicao].tempo_execucao = lista_processos[posicao].tempo_execucao - clock_cpu;
            }
            else{
                clock_cpu = lista_processos[posicao].tempo_execucao;
                lista_processos[posicao].tempo_execucao = 0;
            }

            while(k < iterador){
                if (lista_processos[k].prioridade > 0){
                lista_processos[k].latencia += clock_cpu;
                }
                k++;
            }
         
            if(lista_processos[posicao].tempo_execucao <= 0){ //Processo encerrou

                lista_processos[posicao].prioridade = 0;          
            }

            MRU(posicao, clock_cpu);
            tempo_atual = clock_cpu;

            if (lista_processos[posicao].prioridade == 0){
                libera_memoria(posicao);
            }

            pthread_mutex_unlock(&mutex_PR); 

            printf("\n");
            sleep(1);
        }
        else{
            if ( listaP[iterador-1].id == -1 ){
                //printf("Thread executar encerrou \n");
                break;
            }
            printf("Todos os processos foram executados. Deseja encerrar?\n");
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

        pthread_mutex_lock(&mutex_PR);  

        iterador++;

        lista_processos = realloc(lista_processos, iterador * sizeof(DadosProcessos));
        
        char nome[50];
        int id, tempo, prioridade, qtd_memoria;
        char* sequencia_str;
        
        // Tenta ler os 5 campos principais
        int result = sscanf(linha, "%[^|]|%d|%d|%d|%d|%m[^\n]", nome, &id, &tempo, &prioridade, &qtd_memoria, &sequencia_str); 
        
        if (result == 6) {  // Certifique-se de que 6 campos foram lidos corretamente
            strcpy(listaP[iterador - 1].nome_processo, nome);
            lista_processos[iterador - 1].id = id;
            lista_processos[iterador - 1].tempo_execucao = tempo;
            lista_processos[iterador - 1].prioridade = prioridade;
            lista_processos[iterador - 1].qtd_memoria = qtd_memoria;
            lista_processos[iterador - 1].latencia = 0;
            lista_processos[iterador - 1].tamanho_sequencia = 0;

            // Lê a sequência de acessos se existir
            if (sequencia_str != NULL) {
                char *token = strtok(sequencia_str, " ");
                while (token != NULL) {
                    lista_processos[iterador - 1].sequencia[lista_processos[iterador - 1].tamanho_sequencia++] = atoi(token);
                    token = strtok(NULL, " ");
                }
                free(sequencia_str);  // Libera a memória alocada pelo sscanf
            }

            // Exibe o processo adicionado
            printf("Novo processo adicionado: %s\n", lista_processos[iterador - 1].nome_processo);
            printf("Id: %d \n", lista_processos[iterador - 1].id);
            printf("Clock: %d \n", lista_processos[iterador - 1].tempo_execucao);
            printf("Prioridade: %d \n", lista_processos[iterador - 1].prioridade);
            printf("QtdMemoria: %d \n", lista_processos[iterador - 1].qtd_memoria);
            printf("Sequência de Acessos: ");
            for (int i = 0; i < lista_processos[iterador - 1].tamanho_sequencia; i++) {
                printf("%d ", lista_processos[iterador - 1].sequencia[i]);
            }
            printf("\n\n");
        }
        else {
            int result = sscanf(linha, "%s", nome);

            if(result == 1 && strcmp(nome, "s") == 0){
                lista_processos[iterador - 1].id = -1;
                //printf("Thread Adiconar encerrou \n");
                break;
            }
        }
        pthread_mutex_unlock(&mutex_PR);
    }
}
        

int main() {
    // Alocar memória para a lista de processos
    lista_processos = (DadosProcessos *)malloc(PROCESSOS * sizeof(DadosProcessos));
    paginas_na_memoria =  (Paginas *)malloc(molduras * sizeof(Paginas));

    // Chamar a função para ler processos do arquivo
    int numero_processos = recebe_processos("entradaMemoria.txt", lista_processos);

    // Imprime os valores dos processos
    imprimir_processos(lista_processos, numero_processos);

    printf("\nAlgoritmo MRU\n");
    iterador = numero_processos;

    for (int i = 0; i < molduras_memoria; i++) {
        inicializar_pagina(&paginas_na_memoria[i]);
    }

    pthread_t executando_processo, lendo_novo_processo;
    pthread_mutex_init(&mutex_PR, NULL);

    pthread_create(&lendo_novo_processo, NULL, &recebe_novos_processos, &iterador);
    pthread_create(&executando_processo, NULL, &executando_processos, &clock_cpu);

    pthread_join(executando_processo, NULL);
    pthread_cancel(lendo_novo_processo);

    pthread_mutex_destroy(&mutex_PR);

    free(lista_processos);


    return 0;
}