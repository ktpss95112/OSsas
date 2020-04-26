#include <stdio.h>
#include <stdlib.h>

#define MAX_N_PROCESS 5000

struct process {
    char name[40];
    int ready, exect; // exectime
};

int compar(const void *a, const void *b) { return ((struct process*)a)->ready - ((struct process*)b)->ready; }


int main() {
    char sche_policy[10];
    int n_process;
    struct process processes[MAX_N_PROCESS];

    // parse input
    scanf("%s%d", sche_policy, &n_process);
    for (int i = 0; i < n_process; ++i) scanf("%s%d%d", processes[i].name, &processes[i].ready, &processes[i].exect);
    qsort(processes, n_process, sizeof(struct process), compar);

    return 0;
}
