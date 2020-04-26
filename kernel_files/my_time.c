#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/timer.h>

asmlinkage void sys_my_time(long *sec, long *nsec) {
        struct timespec ts;
        getnstimeofday(&ts);
        *sec = ts.tv_sec;
        *nsec = ts.tv_nsec;
}
