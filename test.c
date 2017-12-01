#include <stdio.h>
#include <pthread.h>

pthread_mutex_t lock;

void* foo(void* n){
    // struct sched_param sched_p;
    // pthread_t t = pthread_self();
    // int ret = pthread_setschedparam(t, SCHED_FIFO, &sched_p);
    // if(ret != 0)
    //     printf("error setting sched\n");
    // else
    //     printf("Success\n");

    printf("%d has been locked \n", *(int*)n);
    pthread_mutex_lock(&lock); //This will be blocked

    printf("%d YOU RELEASED THE LOCK!\n", *(int*)n);

    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);

}

int main(){

    pthread_mutex_init(&lock, NULL);
    pthread_mutex_lock(&lock);

    pthread_t t0,t1;
    int n0 = 0;
    int n1 = 1;
    pthread_create(&t0, NULL, foo, (void*)&n0);
    // for(int i = 0; i<100000; i++){}
    pthread_create(&t1, NULL, foo, (void*)&n1);
    //Create thread

    int n = 1;
    while( n != 0){
        printf("hm.\n");
        scanf("%d", &n);
    }

    pthread_mutex_unlock(&lock);



    pthread_exit(NULL);
}