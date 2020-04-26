#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>

#define MAX_N_PROCESS 5000
#define PRIORITY_HIGH 99
#define PRIORITY_LOW 1

struct process {
    char name[40];
    int ready, exect; // exectime
    pid_t pid;
};

int compar (const void *a, const void *b) { return ((struct process*)a)->ready - ((struct process*)b)->ready; }
void run_unit_time () { volatile unsigned long i; for(i=0;i<1000000UL;i++); }

void use_cpu (pid_t pid, int cpu_id);
void set_priority (pid_t pid, int priority);
void FIFO (int n_process, struct process *processes);
void RR   (int n_process, struct process *processes);
void SJF  (int n_process, struct process *processes);
void PSJF (int n_process, struct process *processes);


int main() {
    char sche_policy[10];
    int n_process;
    struct process processes[MAX_N_PROCESS];

    // set scheduling priority of main process
    use_cpu (getpid(), 0);
    set_priority (getpid(), PRIORITY_HIGH);

    // parse input
    scanf ("%s%d", sche_policy, &n_process);
    for (int i = 0; i < n_process; ++i) {
        scanf ("%s%d%d", processes[i].name, &processes[i].ready, &processes[i].exect);
        processes[i].pid = 0;
    }
    qsort (processes, n_process, sizeof (struct process), compar);

    // select scheduling policy
    switch (sche_policy[0]) {
        case 'F':
            FIFO (n_process, processes);
            break;
        case 'R':
            RR (n_process, processes);
            break;
        case 'S':
            SJF (n_process, processes);
            break;
        case 'P':
            PSJF (n_process, processes);
            break;
    }

    return 0;
}


void use_cpu (pid_t pid, int cpu_id) {
    cpu_set_t mask;
    CPU_ZERO (&mask);
    CPU_SET (cpu_id, &mask);
    if (sched_setaffinity ((pid), sizeof (mask), &mask) != 0){
        perror ("sched_setaffinity error\n");
        exit (EXIT_FAILURE);
    }
}


void set_priority (pid_t pid, int priority) {
    struct sched_param param;
	param.sched_priority = priority;
    if (sched_setscheduler (pid, SCHED_FIFO, &param) != 0) {
        perror ("sched_setscheduler error\n");
        exit (EXIT_FAILURE);
    }
}



void FIFO (int n_process, struct process *processes) {

}


void RR (int n_process, struct process *processes) {

}


void SJF (int n_process, struct process *processes) {

}


void PSJF (int n_process, struct process *processes) {

}
