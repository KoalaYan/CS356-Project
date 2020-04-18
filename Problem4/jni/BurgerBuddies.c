#include<stdio.h>
#include<stdlib.h>
#include<sys/syscall.h>
#include<unistd.h>
#include<string.h>
#include<semaphore.h>
#include<pthread.h>
#include<time.h>

int strToInt(char *str) {
    int i = 0;
    int sum = 0;
    while(str[i]<='9' && str[i] >= '0'){
        sum = sum*10 + (str[i] - '0');
        i++;
    }
    return sum;
}

sem_t sem_cashier, sem_order, sem_rack_empty, sem_rack_full, sem_cashier_ready,mutex,sem_kill;
int num_customer;
int n_cook, n_cashier, n_customer, n_rack;
pthread_t *thread_cook;
pthread_t *thread_cashier;
pthread_t *thread_customer;
pthread_t *thread_killer;

int *id_cook;
int *id_cashier;
int *id_customer;


void *cook(void *argv){
    int id = *(int*)argv;
    //printf("Cook[%d].\n", id);
    while(1){
        sem_wait(&sem_rack_full);
        //make burgers and place them on the rack
        sleep(1);
        printf("Cook[%d] makes a burger.\n", id);
        sem_post(&sem_rack_empty);
    }
    return;
}

void *cashier(void *argv){
    int id = *(int*)argv;
    //printf("Cashier[%d].\n",id);
    while(1){
         sem_wait(&sem_order);
         //accept an order
         printf("Cashier[%d] accepts an order.\n",id);
         sem_wait(&sem_rack_empty);
        //get a burger from the rack
        printf("Cashier[%d] takes a burger to a customer.\n",id);
        sleep(1);
        sem_post(&sem_rack_full);
        sem_post(&sem_cashier_ready);
    }
    return;
}

void *customer(void *argv){
    int id = *(int*)argv;
    sleep(rand()%id);
    printf("Customer[%d] comes.\n",id);
    sem_wait(&sem_cashier);    
    //approaching  a cashier and start the order process
    sem_post(&sem_order);
    sem_wait(&sem_cashier_ready);
    //order and take it away
    sem_wait(&mutex);
	num_customer--;	
    if(num_customer==0){
        sem_post(&sem_kill);
    }
    sem_post(&mutex);
    sem_post(&sem_cashier);
    return;
}

void *killer(){
    sem_wait(&sem_kill);
    int i;
    for(i = 0;i < n_cook;i++){
        pthread_kill(thread_cook[i],0);
    }
    for(i = 0;i < n_cashier;i++){
        pthread_kill(thread_cashier[i],0);
    }
}

int main(int argc, char **argv){
    if (argc != 5)
    {
        printf("Input Error!");
        exit(EXIT_FAILURE);
    }


    n_cook = strToInt(argv[1]);
    n_cashier = strToInt(argv[2]);
    num_customer = n_customer = strToInt(argv[3]);
    n_rack = strToInt(argv[4]);

    //printf("The four argv are %d %d %d %d.\n",n_cook,n_cashier,n_customer,n_rack);

    sem_init(&mutex,0,1);
    sem_init(&sem_kill,0,0);
    sem_init(&sem_cashier_ready,0,0);
    sem_init(&sem_cashier,0,n_cashier);
    sem_init(&sem_rack_empty,0,0);
    sem_init(&sem_rack_full,0,n_rack);
    sem_init(&sem_order,0,0);
    
    thread_cook = malloc(n_cook * sizeof(pthread_t));
    thread_cashier = malloc(n_cashier * sizeof(pthread_t));
    thread_customer = malloc(n_customer * sizeof(pthread_t));
    thread_killer = malloc(sizeof(pthread_t));
    id_cook = malloc(n_cook * sizeof(int));
    id_cashier = malloc(n_cashier * sizeof(int));
    id_customer = malloc(n_customer * sizeof(int));

    printf("Cooks[%d], Cashiers[%d], Customers[%d]\n",n_cook,n_cashier,n_customer);
    printf("Begin run.\n");

    pthread_create(thread_killer,NULL,killer,NULL);
    
    int i;
    for(i = 0;i < n_cook;i++){
        id_cook[i] = i + 1;
        pthread_create(thread_cook + i,NULL,cook,id_cook + i);
        //pthread_join(thread_cook[i-1],NULL);
    }
    for(i = 0;i < n_cashier;i++){
        id_cashier[i] = i + 1;
        pthread_create(thread_cashier + i,NULL,cashier,id_cashier + i);
        //pthread_join(thread_cashier[i-1],NULL);
    }
    for(i = 0;i < n_customer;i++){
        id_customer[i] = i + 1;
        pthread_create(thread_customer + i,NULL,customer,id_customer + i);
        //pthread_join(thread_customer[i-1],NULL);
    }

    pthread_join(*thread_killer,NULL);
    for(i = 0;i < n_cook;i++){
        //pthread_create(thread_cook + i,NULL,cook,id_cook + i);
        pthread_join(thread_cook[i],NULL);
    }
    for(i = 0;i < n_cashier;i++){
        //pthread_create(thread_cashier + i,NULL,cashier,id_cashier + i);
        pthread_join(thread_cashier[i],NULL);
    }
    for(i = 0;i < n_customer;i++){
        //pthread_create(thread_customer + i,NULL,customer,id_customer + i);
        pthread_join(thread_customer[i],NULL);
    }

    

    free(thread_cook);
    free(thread_cashier);
    free(thread_customer);
    free(thread_killer);
    free(id_cook);
    free(id_cashier);
    free(id_customer);

    sem_destroy(&sem_cashier);
    sem_destroy(&sem_cashier_ready);
    sem_destroy(&sem_rack_empty);
    sem_destroy(&sem_rack_full);
    sem_destroy(&sem_order);
    sem_destroy(&mutex);
    sem_destroy(&sem_kill);
    
    printf("Finishing.\n");

    return 0;
}