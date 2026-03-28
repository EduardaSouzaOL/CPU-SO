#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "scheduler.h"

// --- FUNÇÕES DA FILA ---
void init_queue(ReadyQueue *q) { q->size = 0; }

bool enqueue(ReadyQueue *q, Task *t) {
    if (q->size >= MAX_READY_QUEUE) return false;
    q->tasks[q->size] = t;
    q->size++;
    return true;
}

Task *dequeue(ReadyQueue *q) {
    if (q->size == 0) return NULL;
    Task *t = q->tasks[0];
    for (int i = 0; i < q->size - 1; i++) {
        q->tasks[i] = q->tasks[i + 1];
    }
    q->size--;
    return t;
}

// --- FUNÇÕES DE ORDENAÇÃO ---

int compare_rm(const void *a, const void *b) {
    Task *taskA = *(Task **)a;
    Task *taskB = *(Task **)b;
    if (taskA->period != taskB->period) return taskA->period - taskB->period;
    return taskA->id - taskB->id;
}

void sort_queue_rm(ReadyQueue *q) {
    if (q->size > 1) qsort(q->tasks, q->size, sizeof(Task *), compare_rm);
}

int compare_edf(const void *a, const void *b) {
    Task *taskA = *(Task **)a;
    Task *taskB = *(Task **)b;
    if (taskA->absolute_deadline != taskB->absolute_deadline) return taskA->absolute_deadline - taskB->absolute_deadline;
    return taskA->id - taskB->id;
}

void sort_queue_edf(ReadyQueue *q) {
    if (q->size > 1) qsort(q->tasks, q->size, sizeof(Task *), compare_edf);
}

// --- MOTOR DE SIMULAÇÃO PARAMETRIZADO ---

void run_simulation(Task original_tasks[], int task_count, int simulation_time, 
                    void (*sort_func)(ReadyQueue*), const char* algorithm, const char* login) {
    
    // 1. Cria uma cópia limpa das tarefas para esta rodada de simulação
    Task tasks[MAX_TASKS];
    for(int i = 0; i < task_count; i++) {
        tasks[i] = original_tasks[i];
        tasks[i].remaining_time = 0;
        tasks[i].next_activation = 0;
        tasks[i].absolute_deadline = 0;
        tasks[i].complete_executions = 0;
        tasks[i].lost_deadlines = 0;
        tasks[i].killed = 0;
    }

    // 2. Monta o nome do arquivo dinamicamente (ex: rate_meso.out)
    char filename[100];
    sprintf(filename, "%s_%s.out", algorithm, login);
    FILE *out_file = fopen(filename, "w");
    
    // Converte o nome do algoritmo para maiúsculo no cabeçalho
    char header[20];
    strcpy(header, algorithm);
    for(int i = 0; header[i]; i++) {
        if(header[i] >= 'a' && header[i] <= 'z') header[i] -= 32;
    }
    fprintf(out_file, "EXECUTION BY %s\n\n", header);

    ReadyQueue ready_queue;
    init_queue(&ready_queue);

    int current_running_id = -1;
    int block_duration = 0;

    // 3. O Grande Loop do Tempo
    for (int t = 0; t < simulation_time; t++) {
        
        for (int i = 0; i < task_count; i++) {
            if (t % tasks[i].period == 0) {
                tasks[i].remaining_time = tasks[i].exec_time;
                tasks[i].absolute_deadline = t + tasks[i].deadline;
                enqueue(&ready_queue, &tasks[i]);
            }
        }

        // Usa a função de ordenação passada por parâmetro (RM ou EDF)
        sort_func(&ready_queue);

        for (int i = 0; i < ready_queue.size; ) {
            Task *tsk = ready_queue.tasks[i];
            if (t > 0 && t == tsk->absolute_deadline && tsk->remaining_time > 0) {
                tsk->lost_deadlines++;
                for (int j = i; j < ready_queue.size - 1; j++) {
                    ready_queue.tasks[j] = ready_queue.tasks[j + 1];
                }
                ready_queue.size--;
            } else {
                i++;
            }
        }

        int next_id = (ready_queue.size > 0) ? ready_queue.tasks[0]->id : -1;

        if (current_running_id != next_id && block_duration > 0) {
            if (current_running_id == -1) {
                fprintf(out_file, "idle for %d units\n", block_duration);
            } else {
                int task_index = -1;
                for (int i = 0; i < task_count; i++) {
                    if (tasks[i].id == current_running_id) { task_index = i; break; }
                }

                char status_char;
                if (tasks[task_index].remaining_time == 0) status_char = 'F';
                else if (t == tasks[task_index].absolute_deadline) status_char = 'L';
                else status_char = 'H';

                fprintf(out_file, "[T%d] for %d units - %c\n", current_running_id, block_duration, status_char);
            }
            block_duration = 0;
        }

        current_running_id = next_id;
        block_duration++;

        if (ready_queue.size > 0) {
            Task *current_task = ready_queue.tasks[0];
            current_task->remaining_time--;

            if (current_task->remaining_time == 0) {
                current_task->complete_executions++;
                dequeue(&ready_queue);
            }
        }
    }

    if (block_duration > 0) {
        if (current_running_id == -1) fprintf(out_file, "idle for %d units\n", block_duration);
        else fprintf(out_file, "[T%d] for %d units - K\n", current_running_id, block_duration);
    }

    for (int i = 0; i < ready_queue.size; i++) {
        ready_queue.tasks[i]->killed++;
    }

    // 4. Imprimir Relatórios Finais
    fprintf(out_file, "\nLOST DEADLINES\n");
    for(int i = 0; i < task_count; i++) fprintf(out_file, "[T%d] %d\n", tasks[i].id, tasks[i].lost_deadlines);

    fprintf(out_file, "\nCOMPLETE EXECUTION\n");
    for(int i = 0; i < task_count; i++) fprintf(out_file, "[T%d] %d\n", tasks[i].id, tasks[i].complete_executions);

    fprintf(out_file, "\nKILLED\n");
    for(int i = 0; i < task_count; i++) fprintf(out_file, "[T%d] %d\n", tasks[i].id, tasks[i].killed);

    fclose(out_file);
    printf("Simulacao %s concluida! Arquivo gerado: %s\n", algorithm, filename);
}

// --- FUNÇÃO PRINCIPAL ---

int main(int argc, char *argv[]) {
    // 1. Agora exigimos 3 argumentos: ./programa <algoritmo> <arquivo>
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <arquivo_de_entrada.txt>\n", argv[0]);
        return 1;
    }

    const char* input_filename = argv[1];   // O arquivo de texto

    FILE *file = fopen(input_filename, "r");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    int simulation_time;
    if (fscanf(file, "%d", &simulation_time) != 1) {
        fprintf(stderr, "Erro: Formato invalido no tempo de simulacao.\n");
        fclose(file);
        return 1;
    }

    Task tasks_base[MAX_TASKS];
    int task_count = 0;

    while (fscanf(file, " T%d %d %d",
                  &tasks_base[task_count].id,
                  &tasks_base[task_count].period,
                  &tasks_base[task_count].exec_time) == 3) {
        tasks_base[task_count].deadline = tasks_base[task_count].period;
        task_count++;
        if (task_count >= MAX_TASKS) break;
    }
    fclose(file);

    // O login definido no requisito
    const char* login = LOGIN;



    #if defined(RATE_MONOTONIC)
        run_simulation(tasks_base, task_count, simulation_time, sort_queue_rm, "rate", login);
    #elif defined(EARLIEST_DEADLINE_FIRST)
        run_simulation(tasks_base, task_count, simulation_time, sort_queue_edf, "edf", login);
    #endif

    return 0;
}