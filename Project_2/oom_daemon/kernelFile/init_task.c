/*
 *  linux/arch/arm/kernel/init_task.c
 */
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/init_task.h>
#include <linux/mqueue.h>
#include <linux/uaccess.h>

#include <asm/pgtable.h>

static struct signal_struct init_signals = INIT_SIGNALS(init_signals);
static struct sighand_struct init_sighand = INIT_SIGHAND(init_sighand);
/*
 * Initial thread structure.
 *
 * We need to make sure that this is 8192-byte aligned due to the
 * way process stacks are handled. This is done by making sure
 * the linker maps this in the .text segment right after head.S,
 * and making head.S ensure the proper alignment.
 *
 * The things we do for performance..
 */
union thread_union init_thread_union __init_task_data =
	{ INIT_THREAD_INFO(init_task) };

/*
 * Initial task structure.
 *
 * All other task structs will be allocated on slabs in fork.c
 */
struct task_struct init_task = INIT_TASK(init_task);

EXPORT_SYMBOL(init_task);

unsigned long muser_cost[2000] = {0};
int muid[2000] = {0};
int mrt = 0;
struct task_struct *largest_rss_point[2000] = {NULL};
unsigned long largest_rss_pcost[2000] = {0};
bool oom_flag = false;
EXPORT_SYMBOL(muser_cost);
EXPORT_SYMBOL(muid);
EXPORT_SYMBOL(mrt);
EXPORT_SYMBOL(largest_rss_point);
EXPORT_SYMBOL(largest_rss_pcost);
EXPORT_SYMBOL(oom_flag);

struct MMLimits my_mm_limits = 
{
	.num_entry = 0,
};

EXPORT_SYMBOL(my_mm_limits);

void MMLimits_add(struct MMLimits *ml, uid_t uid, unsigned long mm_max){
	int i;
	for(i = 0;i < ml->num_entry;i++){
		if(ml->id[i] == uid){
			ml->limits[i] = mm_max;
			break;
		}
	}
	if(i == ml->num_entry){
		if(ml->num_entry < max_entry){
			printk("add new limitation.\n");
			ml->id[i] = uid;
			ml->limits[i] = mm_max;
			ml->num_entry++;
		}
		else{
			printk("out of bound, cannot add the limitation.\n");
		}
	}
}

EXPORT_SYMBOL(MMLimits_add);

void MMLimits_traverse(struct MMLimits *ml){
	int i;
	for(i = 0;i < ml->num_entry;i++){
		printk("uid=%d, mm_max=%lu\n", ml->id[i], ml->limits[i]);
	}
}

EXPORT_SYMBOL(MMLimits_traverse);

unsigned long MMLimits_search(struct MMLimits *ml, uid_t uid){
	int i;
	for(i = 0;i < ml->num_entry;i++){
		if(ml->id[i] == uid)
			return ml->limits[i];
	}
	if(i == ml->num_entry)
		return 0;
}

EXPORT_SYMBOL(MMLimits_search);

/*
  * User out of memory killer
  * DFS traverse
  */
void psDFS(struct task_struct *ts, unsigned long* user_cost, int* uid, int *rt, unsigned long*lrpc, struct task_struct **lp)
{
	struct task_struct *temp;
	struct list_head *p;
	struct mm_struct *ms;
	uid_t id;
	unsigned long cost;
	unsigned long pd = 4096;
	int i;
	if(ts == NULL){
		//printk("Warning: this is an empty pointer.\n");
		return;
	}
	
	p = (&ts->children)->next;
	id = ts->cred->uid;
	ms = get_task_mm(ts);
	if(ms != NULL){
		cost = get_mm_rss(ms) * pd;
		//printk("The rss of process %d of user %d is %lu.\n", ts->pid, id, cost);
		for(i = 0; i < *rt;i++){
			if(uid[i] == id){
				user_cost[i] += cost;
				if(cost>lrpc[i]){
					lrpc[i] = cost;
					lp[i] = ts;
				}
				break;
			}
		}
		if(i == *rt){
			*rt = *rt + 1;
			uid[i] = id;
			user_cost[i] = cost;
			lrpc[i] = cost;
			lp[i] = ts;
		}
	}
	else{
		//printk("This is a kernel process.\n");
	}
	
	while(p != (&ts->children) && p != NULL){
		temp = list_entry(p,struct task_struct,sibling);
		psDFS(temp, user_cost, uid, rt, lrpc, lp);
		p = p->next;
	}
}
EXPORT_SYMBOL(psDFS);
