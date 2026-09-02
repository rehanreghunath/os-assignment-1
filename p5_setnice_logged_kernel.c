#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/sched/prio.h>
#include <linux/cred.h>
#include <linux/errno.h>

SYSCALL_DEFINE1(setnice_logged, int, nice_val)
{
    int old_nice;

    if (nice_val < MIN_NICE || nice_val > MAX_NICE)
        return -EINVAL;

    old_nice = task_nice(current);
    set_user_nice(current, nice_val);

    printk(KERN_INFO
           "setnice_logged: pid=%d comm=%s old_nice=%d new_nice=%d\n",
           current->pid, current->comm, old_nice, nice_val);

    return 0;
}
