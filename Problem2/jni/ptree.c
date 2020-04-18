#include<stdio.h>
#include<stdlib.h>
#include<sys/syscall.h>
#include<unistd.h>
#include<string.h>

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


void printTree(struct prinfo *buf,int *nr)
{
	//record the generation of each process
	int gene[1000] = {0};
	int tmp_pid;
	int i,j;
	int parent[1000] = {0};
	parent[0] = buf[0].pid;
	int num = 1;
	
	for(i = 1;i < *nr;i++){
		tmp_pid = buf[i].parent_pid;
		j = num;
		while(j > 0 && tmp_pid != parent[j-1]){
			j--;
		}
		if(j==0){
			gene[i] = -1;
		}
		else{
			num = j;
			gene[i] = j;
			parent[num] = buf[i].pid;
			num++;
		}
	}

	//print the tree
	printf("There are %d processes.\n",*nr);
	for(i = 0;i < *nr;i++){
		for(j = 0;j < gene[i];j++)
			printf("\t");
		printf("%s,%d,%ld,%d,%d,%d,%d\n",buf[i].comm,buf[i].pid,buf[i].state,buf[i].parent_pid,
		buf[i].first_child_pid,buf[i].next_sibling_pid,buf[i].uid);
	}
}

int main(){
	//Memory allocation
	struct print *buf = malloc(1000*sizeof(struct prinfo));
	int *nr = malloc(sizeof(int));
	if(buf == NULL || nr == NULL){
		printf("Fail to allocate memory.\n");
		exit(-1);
	}

	//system call
	syscall(__NR_pstreecall,buf,nr);

	//print the process tree with DFS
	printTree(buf,nr);

	//Reclaim allocated memory
	free(buf);
	free(nr);
	
	return 0;
}
