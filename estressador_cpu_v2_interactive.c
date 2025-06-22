#include <stdio.h> //print e scan - entrada e saida
#include <stdlib.h> // funcoes pra mexer com memoria
#include <pthread.h> //manipular threads, usado no main pra cada modo
#include <math.h> //funcoes matematicas, usadas pra estresse da CPU
#include <string.h> //manipulacao de strings e memoria, usado no memset pra set do "1" de bytes alocados
#include <windows.h> //funcs especificas do amigo windows, sleep de pausa na execucao, system_info pra ver numero de nucleos e arquivos
#include <time.h> //tempo e data padrao

/*
\033	É o caractere ESC (escape) → inicia a sequência ANSI
[1;31m	Comando para cor + estilo:
1 → negrito (ou alta intensidade)
31 → cor vermelha
\033[0m	Reset → retorna à formatação padrão do terminal

//comandos para compilacao
Parte	                            O que faz
gcc	                                chama o compilador
estressador_cpu_v2_interactive.c	é o seu arquivo fonte
-o estressador_v2.exe	            diz que o executável final deve se chamar estressador_v2.exe
-lpthread	                        linka a biblioteca de threads
-lm	                                linka a biblioteca matemática (math.h)
*/

int modo = 0; //modo a ser escolhido pelo usuario no cmd
int tempo_execucao = 10;

//objetivo nada mais é do que manter a unid de ponto flutuante da cpu ocupada com os calculos matematicos
// ------------------------ MODO 1: CPU ------------------------
void* estressar_cpu(void* arg) {
    time_t inicio = time(NULL); //marcamos o tempo de inicio do processo
    volatile double resultado = 0.0; //evitamos otimizacao do compiler

    while (time(NULL) - inicio < tempo_execucao) {
        for (int i = 0; i < 1000000; i++) {
            double a = rand() % 1000 + 1; //geracao de numeros random
            double b = rand() % 1000 + 1;
            resultado += sqrt(a) * log(b) * sin(a) * tan(b) + pow(a, b / 1000.0); //variavel que faz um somatorio de tudo que estou fazendo
        }
        printf("\033[1;33m[CPU] Loop ativo... soma: %.2f\033[0m\n", resultado); //exibicao do progresso
    }

    return NULL;
}

//simular uso intenso de RAM e forçar o gerenciador de memória
// ------------------------ MODO 2: MEMÓRIA ------------------------
#define MAX_BLOCOS 100 // pode alocar até 10 GB, teto máximo de alocação da memória //constante pra alocar no máximo 100 blocos de memória

void* estressar_memoria(void* arg) {
    time_t inicio = time(NULL);
    const size_t bloco = 100 * 1024 * 1024; // 100 MB
    void* blocos[MAX_BLOCOS];
    int i = 0;

    FILE* log = fopen("log_memoria.txt", "w");
    if (log == NULL) {
        printf("\033[1;31merro na criacao do log\033[0m\n");
        return NULL;
    }

    time_t agora = time(NULL);
    struct tm* tm_info = localtime(&agora);
    char data_hora[64];
    strftime(data_hora, sizeof(data_hora), "%d/%m/%Y %H:%M:%S", tm_info);
    fprintf(log, "==================== LOG DE USO DE MEMÓRIA ====================\n");
    fprintf(log, "inicio: %s\n", data_hora);
    fprintf(log, "tempo solicitado: %d segundos\n\n", tempo_execucao);

    while ((time(NULL) - inicio < tempo_execucao) && i < MAX_BLOCOS) {
        blocos[i] = malloc(bloco); //malloc = funcao que aloca um espaco na memoria dinamicamente!!!
        if (blocos[i] == NULL) {
            fprintf(log, "[%02d] falha ao alocar bloco %d (%.2f GB total)\n", i + 1, i + 1, (i * 100) / 1024.0);
            printf("\033[1;31m[MEM] falha ao alocar bloco %d!\033[0m\n", i + 1);
            break;
        }
        memset(blocos[i], 1, bloco); //memory set, basicamente preenche cada byte do bloco alocado com "1", nao somente alocada, mas preenchida
        fprintf(log, "[%02d] bloco alocado com sucesso (%.2f GB acumulado)\n", i + 1, ((i + 1) * 100) / 1024.0);
        printf("\033[1;35m[MEM] alocado bloco %d (100MB)\033[0m\n", i + 1);
        i++;
        Sleep(100);
    }

    int tempo_real = (int)(time(NULL) - inicio);
    fprintf(log, "\nTotal de blocos alocados: %d\n", i);
    fprintf(log, "Memoria total usada: %.2f GB\n", (i * 100) / 1024.0);
    fprintf(log, "Tempo real de execucao: %d segundos\n", tempo_real);
    fprintf(log, "Status final: %s\n", (i == MAX_BLOCOS || tempo_real >= tempo_execucao) ? "Finalizado com sucesso" : "Finalizado por falta de memoria");
    fprintf(log, "============================by_pedrinho===================================\n");
    fclose(log);

    Sleep(2000); // manter RAM ocupada por mais 2s

    for (int j = 0; j < i; j++) {
        free(blocos[j]);
    }

    return NULL;
}


//simular um processo que faz muita leitura/escrita em disco, como bkp ou banco de dados
// ------------------------ MODO 3: DISCO ------------------------
#define ARQUIVOS 6 //3 arquivos POR ITERAÇÃO, ou seja, 150 * 3 = 450MB por iteração
#define TAMANHO_BLOCO (150 * 1024 * 1024) // 150 MB por arquivo

void* estressar_disco(void* arg) {
    time_t inicio = time(NULL);
    char nome_arquivo[64];
    FILE* f;

    // aloca buffer com 150MB preenchido
    char* buffer = malloc(TAMANHO_BLOCO);
    if (buffer == NULL) {
        printf("\033[1;31m[DISCO] falha na alocacao de buffer :/\033[0m\n");
        return NULL;
    }
    memset(buffer, 'A', TAMANHO_BLOCO);

    int iteracao = 0;

    while (time(NULL) - inicio < tempo_execucao) {
        for (int i = 0; i < ARQUIVOS; i++) {
            sprintf(nome_arquivo, "stress_disk_%d.tmp", i);
            f = fopen(nome_arquivo, "wb");
            if (!f) continue;
            fwrite(buffer, 1, TAMANHO_BLOCO, f);
            fclose(f);
        }

        for (int i = 0; i < ARQUIVOS; i++) {
            sprintf(nome_arquivo, "stress_disk_%d.tmp", i);
            f = fopen(nome_arquivo, "rb");
            if (!f) continue;
            fread(buffer, 1, TAMANHO_BLOCO, f);
            fclose(f);
        }

        printf("\033[1;34m[DISCO] iterando... %d: escrita e leitura de %d arquivos de %.2fMB\033[0m\n",
               ++iteracao, ARQUIVOS, TAMANHO_BLOCO / (1024.0 * 1024.0));

        Sleep(100); // pausa de 100ms entre ciclos
    }

    // limpeza final
    for (int i = 0; i < ARQUIVOS; i++) {
        sprintf(nome_arquivo, "stress_disk_%d.tmp", i);
        remove(nome_arquivo);
    }

    free(buffer);
    return NULL;
}

// ------------------------ MAIN ------------------------
int main() {
    int continuar = 1;

    while (continuar) {
        system("cls");

        printf("\033[1;36mEstressador de Sistema v2\033[0m\n");
        printf("O que deseja estressar hoje?:\n");
        printf("[1] CPU\n[2] Memoria\n[3] Disco\n[0] Sair\n> ");
        scanf("%d", &modo);

        if (modo == 0) {
            printf("\033[1;32mEncerrando o programa...\033[0m\n");
            break;
        }

        printf("Digite o tempo de execucao (segundos): ");
        scanf("%d", &tempo_execucao);

        pthread_t t;
        switch (modo) {
            case 1: pthread_create(&t, NULL, estressar_cpu, NULL); break;
            case 2: pthread_create(&t, NULL, estressar_memoria, NULL); break;
            case 3: pthread_create(&t, NULL, estressar_disco, NULL); break;
            default:
                printf("\033[1;31mModo invalido.\033[0m\n");
                continue;
        }

        pthread_join(t, NULL);
        printf("\n\033[1;32mExecucao concluida.\033[0m\n");
        printf("\nDeseja realizar outro teste? (1 = sim, 0 = nao): ");
        scanf("%d", &continuar);
    }

    return 0;
}

