#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>

#define MAX_N_PROCESS 5000
#define CPU_PARENT 0
#define CPU_CHILD 1
#define PRIORITY_HIGH 99
#define PRIORITY_LOW 0

struct process {
    char name[40];
    int ready, exect; // exectime
    pid_t pid;
};

int compar (const void *a, const void *b) { return ((struct process*)a)->ready - ((struct process*)b)->ready; }
void run_unit_time () { volatile unsigned long i; for(i=0;i<1000000UL;i++); }

void use_cpu (pid_t pid, int cpu_id);
void set_priority (pid_t pid, int priority);
void run_child (struct process *child_process);
void FIFO (int n_process, struct process *processes);
void RR   (int n_process, struct process *processes);
void SJF  (int n_process, struct process *processes);
void PSJF (int n_process, struct process *processes);


int main() {
    char sche_policy[10];
    int n_process;
    struct process processes[MAX_N_PROCESS];

    // set scheduling priority of main process
    use_cpu (getpid(), CPU_PARENT);
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
    int policy = (priority == PRIORITY_LOW) ? SCHED_IDLE : SCHED_FIFO;
    if (sched_setscheduler (pid, policy, &param) != 0) {
        perror ("sched_setscheduler error\n");
        exit (EXIT_FAILURE);
    }
}


void run_child (struct process *child_process) {
    printf ("%s %d\n", child_process->name, child_process->pid);

    // run
    long start_sec, start_nsec;
    syscall (333, &start_sec, &start_nsec);
    for (int i = 0; i < child_process->exect; ++i) run_unit_time ();
    long end_sec, end_nsec;
    syscall (333, &end_sec, &end_nsec);

    // printk
    char buf[100];
    sprintf (buf, "[Project1] %d %ld.%09ld %ld.%09ld\n", child_process->pid, start_sec, start_nsec, end_sec, end_nsec);
    syscall (334, buf);
    // printf ("%s", buf);

    exit (EXIT_SUCCESS);
}


void FIFO (int n_process, struct process *processes) {
    int que_front = 0, que_end = 0;
    int current_running_count = 0; // 0 indicates not running

    for (int t = 0, finish_child_count = 0; finish_child_count < n_process; ++t) {
        while (que_end != n_process && processes[que_end].ready <= t) {
            pid_t pid = (processes[que_end].pid = fork());
            if (pid < 0) {
                perror ("fork error\n");
                exit (EXIT_FAILURE);
            }
            else if (pid == 0) {
                // child
                pid = (processes[que_end].pid = getpid());
                use_cpu (pid, CPU_CHILD);
                set_priority (pid, PRIORITY_LOW);
                run_child (&(processes[que_end]));
            }
            // parent
            ++que_end;
        }

        // if no job is running
        if (current_running_count == 0) {
            // if no job can run
            if (que_front == que_end) {
                run_unit_time ();
                continue;
            }
            // run next job
            set_priority (processes[que_front].pid, PRIORITY_HIGH);
        }

        run_unit_time ();
        ++current_running_count;

        // finish running
        if (current_running_count == processes[que_front].exect) {
            static int status;
            waitpid (processes[que_front].pid, &status, 0);
            printf ("child %d finished.\n", processes[que_front].pid);
            current_running_count = 0;
            ++que_front;
            ++finish_child_count;
        }
    }
}


void RR (int n_process, struct process *processes) {

}


void SJF (int n_process, struct process *processes) {

}


void PSJF (int n_process, struct process *processes) {

}
