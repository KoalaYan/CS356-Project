/*
 *  linux/arch/arm/kernel/sys_arm.c
 *
 *  Copyright (C) People who wrote linux/arch/i386/kernel/sys_i386.c
 *  Copyright (C) 1995, 1996 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This file contains various random system calls that
 *  have a non-standard calling sequence on the Linux/arm
 *  platform.
 */
#include <linux/export.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/mman.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/ipc.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>

/* Fork a new task - this creates a new program thread.
 * This is called indirectly via a small wrapper
 */
asmlinkage int sys_fork(struct pt_regs *regs)
{
#ifdef CONFIG_MMU
	return do_fork(SIGCHLD, regs->ARM_sp, regs, 0, NULL, NULL);
#else
	/* can not support in nommu mode */
	return(-EINVAL);
#endif
}

/* Clone a task - this clones the calling program thread.
 * This is called indirectly via a small wrapper
 */
asmlinkage int sys_clone(unsigned long clone_flags, unsigned long newsp,
			 int __user *parent_tidptr, int tls_val,
			 int __user *child_tidptr, struct pt_regs *regs)
{
	if (!newsp)
		newsp = regs->ARM_sp;

	return do_fork(clone_flags, newsp, regs, 0, parent_tidptr, child_tidptr);
}

asmlinkage int sys_vfork(struct pt_regs *regs)
{
	return do_fork(CLONE_VFORK | CLONE_VM | SIGCHLD, regs->ARM_sp, regs, 0, NULL, NULL);
}

/* sys_execve() executes a new program.
 * This is called indirectly via a small wrapper
 */
asmlinkage int sys_execve(const char __user *filenamei,
			  const char __user *const __user *argv,
			  const char __user *const __user *envp, struct pt_regs *regs)
{
	int error;
	char * filename;

	filename = getname(filenamei);
	error = PTR_ERR(filename);
	if (IS_ERR(filename))
		goto out;
	error = do_execve(filename, argv, envp, regs);
	putname(filename);
out:
	return error;
}

int kernel_execve(const char *filename,
		  const char *const argv[],
		  const char *const envp[])
{
	struct pt_regs regs;
	int ret;

	memset(&regs, 0, sizeof(struct pt_regs));
	ret = do_execve(filename,
			(const char __user *const __user *)argv,
			(const char __user *const __user *)envp, &regs);
	if (ret < 0)
		goto out;

	/*
	 * Save argc to the register structure for userspace.
	 */
	regs.ARM_r0 = ret;

	/*
	 * We were successful.  We won't be returning to our caller, but
	 * instead to user space by manipulating the kernel stack.
	 */
	asm(	"add	r0, %0, %1\n\t"
		"mov	r1, %2\n\t"
		"mov	r2, %3\n\t"
		"bl	memmove\n\t"	/* copy regs to top of stack */
		"mov	r8, #0\n\t"	/* not a syscall */
		"mov	r9, %0\n\t"	/* thread structure */
		"mov	sp, r0\n\t"	/* reposition stack pointer */
		"b	ret_to_user"
		:
		: "r" (current_thread_info()),
		  "Ir" (THREAD_START_SP - sizeof(regs)),
		  "r" (&regs),
		  "Ir" (sizeof(regs))
		: "r0", "r1", "r2", "r3", "r8", "r9", "ip", "lr", "memory");

 out:
	return ret;
}
EXPORT_SYMBOL(kernel_execve);

/*
 * Since loff_t is a 64 bit type we avoid a lot of ABI hassle
 * with a different argument ordering.
 */
asmlinkage long sys_arm_fadvise64_64(int fd, int advice,
				     loff_t offset, loff_t len)
{
	return sys_fadvise64_64(fd, offset, len, advice);
}

asmlinkage long sys_set_mm_limits(uid_t uid, unsigned long mm_max)
{
 	printk("syscall is invoked. And uid is %d, mm_max is %lu.\n",uid ,mm_max);	

	//uid: the id of the user that we want to set memory limit for
	//mm_max: maximum amount of physical memory the user can use(in bytes)
	
	MMLimits_add(&my_mm_limits, uid, mm_max);
	MMLimits_traverse(&my_mm_limits);
	
	return 0;
}

asmlinkage long sys_oomk_syscall(void)
{
	
 	unsigned long temp;
	int i;
	struct task_struct *p;
	struct task_struct *q;

	//printk("my_oom_Killer is running.\n");

	mrt = 0;

	read_lock(&tasklist_lock);
	psDFS(&init_task, muser_cost, muid, &mrt, largest_rss_pcost, largest_rss_point);
	read_unlock(&tasklist_lock);
	
	//printk("Finish psDFS and there are %d users.\n", mrt);
	read_lock(&tasklist_lock);
	for(i = 0; i < mrt;i++){
		temp = MMLimits_search(&my_mm_limits, muid[i]);
		p = largest_rss_point[i];
		printk("The limitation of user %d is %lu, and its cost is %lu.\n", muid[i], temp, muser_cost[i]);
		if(temp != 0 && temp < muser_cost[i] && p != NULL){
			
			task_lock(p);	/* Protect ->comm from prctl() */
			printk("uid=%d, uRSS=%lu, mm_max=%lu, pid=%d, pRSS=%lu.\n", muid[i], muser_cost[i], temp, task_pid_nr(p), largest_rss_pcost[i]);
			//pr_err("Kill process %d (%s).\n", task_pid_nr(p), p->comm);
			task_unlock(p);

			
			for_each_process(q)
				if (q->mm == p->mm && !same_thread_group(q, p) &&
				    !(q->flags & PF_KTHREAD)) {
					task_lock(q);
					pr_err("Kill process %d (%s) sharing same memory\n",
						task_pid_nr(q), q->comm);
					task_unlock(q);
					do_send_sig_info(SIGKILL, SEND_SIG_FORCED, q, true);
				}
			

			set_tsk_thread_flag(p, TIF_MEMDIE);
			do_send_sig_info(SIGKILL, SEND_SIG_FORCED, p, true);

		}
	}
	read_unlock(&tasklist_lock);
	
	return 0;
}

