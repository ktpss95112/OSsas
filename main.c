#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>

#define MAX_N_PROCESS 5000
#define MAX_EXECTIME 10000000
#define CPU_PARENT 0
#define CPU_CHILD 1
#define PRIORITY_HIGH 99
#define PRIORITY_LOW 0
#define RR_QUANTUM 500

#define ERR_EXIT(x) do { perror (x); exit (EXIT_FAILURE); } while(0)

struct process {
    char name[40];
    int ready, exect, remain; // exectime
    pid_t pid;
};

void run_unit_time () { volatile unsigned long i; for(i=0;i<1000000UL;i++); }
void empty_handler (int sig) {}
void dummy_child () { nice (-10); while (1); }

void use_cpu (pid_t pid, int cpu_id);
void set_priority (pid_t pid, int priority);
void run_child (struct process *child_process);
void FIFO (int n_process, struct process *processes);
void RR   (int n_process, struct process *processes);
void SJF  (int n_process, struct process *processes);
void PSJF (int n_process, struct process *processes);

sigset_t block_mask, wait_mask, old_mask;


int main() {
    char sche_policy[10];
    int n_process;
    struct process processes[MAX_N_PROCESS];

    // assign a dummy process to child cpu
    pid_t dummy;
    if ((dummy = fork()) < 0) ERR_EXIT ("fork error\n");
    else if (dummy == 0) dummy_child ();
    use_cpu (dummy, CPU_CHILD);

    // set signal handler and block SIGUSR1
    sigfillset (&wait_mask);
    sigdelset (&wait_mask, SIGUSR1);
    sigemptyset (&block_mask);
    sigaddset (&block_mask, SIGUSR1);
    if (sigprocmask (SIG_BLOCK, &block_mask, &old_mask) < 0) ERR_EXIT ("sigprocmask error\n");
    if (signal (SIGUSR1, empty_handler) == SIG_ERR) ERR_EXIT ("signal error\n");

    // set scheduling priority of main process
    use_cpu (getpid(), CPU_PARENT);
    set_priority (getpid(), PRIORITY_HIGH);

    // parse input
    scanf ("%s%d", sche_policy, &n_process);
    for (int i = 0; i < n_process; ++i) {
        scanf ("%s%d%d", processes[i].name, &processes[i].ready, &processes[i].exect);
        processes[i].pid = 0;
        processes[i].remain = processes[i].exect;
    }

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
    if (sched_setaffinity ((pid), sizeof (mask), &mask) != 0) ERR_EXIT ("sched_setaffinity error\n");
}


void set_priority (pid_t pid, int priority) {
    struct sched_param param;
	param.sched_priority = priority;
    int policy = (priority == PRIORITY_LOW) ? SCHED_IDLE : SCHED_FIFO;
    if (sched_setscheduler (pid, policy, &param) != 0) ERR_EXIT ("sched_setscheduler error\n");
}


void run_child (struct process *child_process) {
    sigsuspend (&wait_mask);
    printf ("%s %d\n", child_process->name, child_process->pid);

    // run
    long start_sec, start_nsec;
    syscall (333, &start_sec, &start_nsec);
    for (int i = 0; i < child_process->exect; ++i){
        run_unit_time ();
        // if (i % 1000 == 0) puts (child_process->name);
    }
    long end_sec, end_nsec;
    syscall (333, &end_sec, &end_nsec);

    // printk
    char buf[100];
    sprintf (buf, "[Project1] %d %ld.%09ld %ld.%09ld\n", child_process->pid, start_sec, start_nsec, end_sec, end_nsec);
    syscall (334, buf);
    // printf ("%s %s", child_process->name, buf);

    exit (EXIT_SUCCESS);
}


void FIFO (int n_process, struct process *processes) {
    int que_front = 0, que_end = 0;
    int current_running_count = 0; // 0 indicates not running

    for (int t = 0, finish_child_count = 0; finish_child_count < n_process; ++t) {
        while (que_end != n_process && processes[que_end].ready <= t) {
            pid_t pid = (processes[que_end].pid = fork());
            if (pid < 0) ERR_EXIT ("fork error\n");
            else if (pid == 0) {
                // child
                pid = (processes[que_end].pid = getpid());
                run_child (&(processes[que_end]));
            }
            // parent
            use_cpu (pid, CPU_CHILD);
            set_priority (pid, PRIORITY_LOW);
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
            // printf ("run %s\n", processes[que_front].name);
            set_priority (processes[que_front].pid, PRIORITY_HIGH);
            kill (processes[que_front].pid, SIGUSR1);
        }

        run_unit_time ();
        ++current_running_count;

        // finish running
        if (current_running_count == processes[que_front].exect) {
            static int status;
            waitpid (processes[que_front].pid, &status, 0);
            // printf ("child %d finished.\n", processes[que_front].pid);
            current_running_count = 0;
            ++que_front;
            ++finish_child_count;
        }
    }
}


void RR (int n_process, struct process *processes) {
    int list_entry = -1;
    int next[n_process], prev[n_process];
    int next_entering_job = 0;
    int current_running_count = 0;

    for (int t = 0, finish_child_count = 0; finish_child_count < n_process; ++t) {
        while (next_entering_job != n_process && processes[next_entering_job].ready <= t) {
            // insert next_entering_job into list
            if (list_entry == -1) list_entry = next[next_entering_job] = prev[next_entering_job] = next_entering_job;
            else {
                int _n = list_entry, _p = prev[list_entry];
                prev[_n] = next[_p] = next_entering_job;
                next[next_entering_job] = _n;
                prev[next_entering_job] = _p;
            }

            pid_t pid = (processes[next_entering_job].pid = fork());
            if (pid < 0) ERR_EXIT ("fork error\n");
            else if (pid == 0) {
                // child
                pid = (processes[next_entering_job].pid = getpid());
                run_child (&(processes[next_entering_job]));
            }
            // parent
            // printf ("insert %d\n", next_entering_job);
            use_cpu (pid, CPU_CHILD);
            set_priority (pid, PRIORITY_LOW);
            ++next_entering_job;
        }

        // if no job is running
        if (current_running_count == 0) {
            // if no job can run
            if (list_entry == -1) {
                run_unit_time ();
                continue;
            }
            // run next job
            // printf ("run %s\n", processes[list_entry].name);
            set_priority (processes[list_entry].pid, PRIORITY_HIGH);
            if (processes[list_entry].remain == processes[list_entry].exect) kill (processes[list_entry].pid, SIGUSR1);
        }

        run_unit_time ();
        ++current_running_count;

        // finish running
        if (current_running_count == processes[list_entry].remain) {
            static int status;
            waitpid (processes[list_entry].pid, &status, 0);
            // printf ("child %d finished.\n", processes[list_entry].pid);
            current_running_count = 0;
            ++finish_child_count;

            // remove list_entry from list
            if (next[list_entry] == list_entry) list_entry = -1;
            else {
                int _n = next[list_entry], _p = prev[list_entry];
                next[_p] = _n;
                prev[_n] = _p;
                list_entry = _n;
            }
        }
        // achieve time quantum
        else if (current_running_count == RR_QUANTUM) {
            processes[list_entry].remain -= RR_QUANTUM;
            set_priority (processes[list_entry].pid, PRIORITY_LOW);
            list_entry = next[list_entry];
            current_running_count = 0;
            // printf ("switch to %d\n", list_entry);
        }
    }
}


void SJF (int n_process, struct process *processes) {
    /** Current implementation is O(N) finding the smallest element in a linked list.
     *  Can be improved by using heap (priority_queue).
     */

    int pool_entry = -1;
    int next[n_process]; // next[last_thing] = -1
    int next_entering_job = 0;
    int current_job = -1;

    for (int t = 0, finish_child_count = 0; finish_child_count < n_process; ++t) {
        while (next_entering_job != n_process && processes[next_entering_job].ready <= t) {
            // insert next_entering_job into pool
            if (pool_entry == -1) next[(pool_entry = next_entering_job)] = -1;
            else {
                next[next_entering_job] = pool_entry;
                pool_entry = next_entering_job;
            }

            pid_t pid = (processes[next_entering_job].pid = fork());
            if (pid < 0) ERR_EXIT ("fork error\n");
            else if (pid == 0) {
                // child
                pid = (processes[next_entering_job].pid = getpid());
                run_child (&(processes[next_entering_job]));
            }
            // parent
            // printf ("insert %d\n", next_entering_job);
            use_cpu (pid, CPU_CHILD);
            set_priority (pid, PRIORITY_LOW);
            ++next_entering_job;
        }

        // if no job is running
        if (current_job == -1) {
            // if no job can run
            if (pool_entry == -1) {
                run_unit_time ();
                continue;
            }
            // find next job
            for (int current = pool_entry, min_exect = MAX_EXECTIME; current != -1; current = next[current])
                if (processes[current].exect <= min_exect) min_exect = processes[(current_job = current)].exect;
            // remove next job from pool
            if (current_job == pool_entry) pool_entry = next[current_job];
            else {
                for (int current = pool_entry; current != next[current_job]; current = next[current])
                    if (next[current] == current_job) next[current] = next[current_job];
            }
            // run next job
            // printf ("run %d\n", processes[current_job].pid);
            set_priority (processes[current_job].pid, PRIORITY_HIGH);
            kill (processes[current_job].pid, SIGUSR1);
        }

        run_unit_time ();
        --processes[current_job].remain;

        // finish running
        if (processes[current_job].remain == 0) {
            static int status;
            waitpid (processes[current_job].pid, &status, 0);
            // printf ("child %d finished.\n", processes[current_job].pid);
            current_job = -1;
            ++finish_child_count;
        }
    }
}


void PSJF (int n_process, struct process *processes) {
    /** Current implementation is O(N) finding the smallest element in a linked list.
     *  Can be improved by using heap (priority_queue).
     */

    int pool_entry = -1;
    int next[n_process]; // next[last_thing] = -1
    int next_entering_job = 0;
    int preempt = 0;
    int current_job = -1, current_running_count = 0;

    for (int t = 0, finish_child_count = 0; finish_child_count < n_process; ++t) {
        while (next_entering_job != n_process && processes[next_entering_job].ready <= t) {
            preempt = 1;
            // insert next_entering_job into pool
            if (pool_entry == -1) next[(pool_entry = next_entering_job)] = -1;
            else {
                next[next_entering_job] = pool_entry;
                pool_entry = next_entering_job;
            }

            pid_t pid = (processes[next_entering_job].pid = fork());
            if (pid < 0) ERR_EXIT ("fork error\n");
            else if (pid == 0) {
                // child
                pid = (processes[next_entering_job].pid = getpid());
                run_child (&(processes[next_entering_job]));
            }
            // parent
            // printf ("insert %d\n", next_entering_job);
            use_cpu (pid, CPU_CHILD);
            set_priority (pid, PRIORITY_LOW);
            ++next_entering_job;
        }

        // if no job is running and no job can run
        if (current_job == -1 && pool_entry == -1) {
            run_unit_time ();
            continue;
        }
        
        if (preempt) {
            processes[current_job].remain -= current_running_count;
            current_running_count = 0;
        }
        // if no job is running or preempt
        if (current_job == -1 || preempt) {
            // printf ("switch job, preempt = %d\n", preempt);
            // find next job
            int next_job = -1;
            for (int current = pool_entry, min_remain = MAX_EXECTIME; current != -1; current = next[current])
                if (processes[current].remain <= min_remain) min_remain = processes[(next_job = current)].remain;
            
            // stop previous job
            if (current_job != -1) set_priority (processes[current_job].pid, PRIORITY_LOW);

            // run next job
            // printf ("run %s\n", processes[next_job].name);
            set_priority (processes[next_job].pid, PRIORITY_HIGH);
            current_job = next_job;
            if (processes[current_job].remain == processes[current_job].exect) kill (processes[current_job].pid, SIGUSR1);
        }

        run_unit_time ();
        --processes[current_job].remain;

        // finish running
        if (processes[current_job].remain == 0) {
            static int status;
            waitpid (processes[current_job].pid, &status, 0);
            // printf ("child %d finished.\n", processes[current_job].pid);

            // remove next job from pool
            if (current_job == pool_entry) pool_entry = next[current_job];
            else {
                for (int current = pool_entry; current != next[current_job]; current = next[current])
                    if (next[current] == current_job) next[current] = next[current_job];
            }

            current_job = -1;
            ++finish_child_count;
        }
        preempt = 0;
    }
}
