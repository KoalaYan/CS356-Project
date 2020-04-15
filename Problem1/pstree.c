#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/sched.h>
#include<linux/unistd.h>
#include<linux/list.h>
#include<linux/slab.h>
#include<linux/uaccess.h>
#include<linux/syscalls.h>
#include<linux/gfp.h>

MODULE_LICENSE("Dual BSD/GPL");
#define __NR_pstreecall 356

struct prinfo{
	pid_t parent_pid;	//process id of parent
	pid_t pid;		//process id
	pid_t first_child_pid;	//pid of youngest child
	
	pid_t next_sibling_pid;	//pid of older sibling
	long state;		//current state of process
	long uid;		//user id of process owner
	char comm[64];		//name of program executed
};

//this function is used to translate the task_struct into prinfo
void translate(struct task_struct *ts,struct prinfo *pf){
	//sched.h file line 1373
	pf->parent_pid = ts->parent->pid;
	//sched.h file line 1359
	pf->pid = ts->pid;
		
	//sched.h file line 1377, list.h file line 186 & line 345
	if(list_empty(&(ts->children))){
		pf->first_child_pid = 0;
	}
	else{
		//list_entry helps to find which task_struct the line_head pointer in
		pf->first_child_pid = list_entry((&ts->children)->next,struct task_struct,children)->pid;
	}

	if(list_empty(&(ts->sibling))){
		pf->next_sibling_pid = 0;
	}
	else
	{
		pid_t next_sibling_pid = list_entry(&(ts->sibling),struct task_struct,sibling)->pid;
		if(next_sibling_pid == pf->parent_pid){
			pf->next_sibling_pid = 0;
		}
		else{
			pf->next_sibling_pid = next_sibling_pid;
		}
	}

	//sched.h file line 1266
	pf->state = ts->state;
	//cred.h file line 125
	pf->uid = ts->cred->uid;
	//sched.h file line 2360
	get_task_comm(pf->comm,ts);
}

//DFS for pstree
void pstreeDFS(struct task_struct *ts,struct prinfo *buf,int *nr)
{
	struct task_struct *temp;
	struct list_head *p;// =  (&ts->children)->next;

	translate(ts,&buf[*nr]);//translate(ts,buf+(*nr));
	*nr = *nr + 1;

	list_for_each(p,&ts->children)
	{
		temp = list_entry(p,struct task_struct,sibling);
		pstreeDFS(temp,buf,nr);
	}
/*
	while(p != &(ts->children)){
		temp = list_entry(p,struct task_struct,children);
		pstreeDFS(temp,buf,nr);
		p = p->next;
	}
*/
}

static int (*oldcall)(void);
static int pstree(struct prinfo *buf,int *nr)
{
	/**
	 * Memory distribution and allocation of Linux kernel address space
	 * kcalloc(size_t n, size_t size, gfp_t flags) : allocate array memory and set zero
	 * kzalloc(size_t size, gfp_t flags) : allocate memory and set zero
	**/
	struct prinfo *k_buf = kcalloc(500, sizeof(struct prinfo),GFP_KERNEL);
	int *k_nr = kzalloc(sizeof(int),GFP_KERNEL);
	if(k_buf == NULL || k_nr == NULL){
		printk("Fail to allocate memory.\n");
		return -1;
	}
	*k_nr = 0;
	
	//DFS+translation
	read_lock(&tasklist_lock);
	pstreeDFS(&init_task,k_buf,k_nr);
	read_unlock(&tasklist_lock);

	//copy from kernel to user
	//unsigned long copy_to_user(void __user *to, const void *from, unsigned long n);
	if(copy_to_user(buf,k_buf,500*sizeof(struct prinfo))){
		printk("Fail to copy from kernel to user.\n");
		return -1;
	}
	if(copy_to_user(nr,k_nr,sizeof(int))){
		printk("Fail to copy from kernel to user.\n");
		return -1;
	}

	/**
	 * Reclaim allocated memory
	 * void kfree(const void *objp) : Memory release function
	**/
	kfree(k_nr);
	kfree(k_buf);

	return 0;
}

static int addsyscall_init(void)
{
	long *syscall = (long*)0xc000d8c4;
	oldcall = (int(*)(void))(syscall[__NR_pstreecall]);
	syscall[__NR_pstreecall] = (unsigned long )pstree;
	printk(KERN_INFO "modelu load!\n");
	return 0;
}

static void addsyscall_exit(void)
{
	long *syscall = (long*)0xc000d8c4;
	syscall[__NR_pstreecall] = (unsigned long )oldcall;
	printk(KERN_INFO "module exit\n");
}

module_init(addsyscall_init);
module_exit(addsyscall_exit);
