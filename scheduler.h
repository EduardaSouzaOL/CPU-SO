#ifndef TASK_H
#define TASK_H


/* GLOBAL*/
#define MAX_NAME_LENGTH 20
#define MAX_TASKS 10
#define LOGIN "meso"
#define MAX_READY_QUEUE 100

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    int id;
    int period;
    int exec_time;
    int deadline;
    
    int remaining_time;  
    int next_activation; 
    
    
    int absolute_deadline;
    int complete_executions;
    int lost_deadlines;
    int killed;
} Task;

// Estrutura para controle de instâncias de tarefas (Jobs)
// Útil caso o seu simulador precise lidar com múltiplas instâncias da mesma 
// tarefa ativas ao mesmo tempo, ou para checar deadlines absolutos no tempo.
typedef struct {
    Task *task_ref;        // Ponteiro para a tarefa original (evita duplicar dados estáticos)
    int release_time;      // Tempo de simulação em que esta instância foi liberada
    int absolute_deadline; // release_time + deadline relativo da tarefa
} TaskInstance;

typedef struct {
    // Usamos um array de ponteiros para não copiar as structs inteiras o tempo todo
    Task *tasks[MAX_READY_QUEUE]; 
    int size; // Quantidade atual de tarefas na fila
} ReadyQueue;

#endif 