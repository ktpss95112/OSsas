#include <linux/kernel.h>
#include <linux/linkage.h>

asmlinkage void sys_my_print(char *str) {
	printk("%s", str);
}
