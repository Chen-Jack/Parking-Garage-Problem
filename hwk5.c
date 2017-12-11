/***
Title : hwk5.c
Author : Jack Chen
Submitted on : Dec 11, 2017

Description : 
  Simulates the Parking Garage Problem (using pthreads). The program is given
  an exponential and normal distribution mean, and will compute the chance 
  that a car cannot park, and the average amount of open spots.

Usage : ./hwk5 [total_spots] [exp mean] [gauss mean] [thread_count]
  *** thread_count is optional ***

Build with : gcc hwk5.c -o hwk5 -lpthread -lm

***/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

struct thread_params{
  int total_iterations;
  double gauss_mean;
  double exp_mean;
  int TOTAL_SPOTS;
  int TOTAL_THREADS;
  pthread_t main_thread;
};

char * init_sequence ( int state_size ){
  char * state ;
  state = ( char *) malloc ( state_size * sizeof ( char ) ) ;
  if ( NULL != state )
    initstate ( time ( NULL ) , state , state_size ) ;
    return state ;
}

//Global Variables
int MAX_ITERATIONS = 1000000;
double error_bound = 0.001;

int iterations_executed = 0; //Actual number of iterations performed
pthread_mutex_t iterations_lock; //Mutex to change iterations_executed

pthread_t* thread; //An array of thread ids
double *spot; //An array of all spots
pthread_mutex_t *lock; //An array of mutexes for each spot

int open_spots = 0; //Available spots during the entire runtime of program
pthread_mutex_t lock_open; //Mutex to change open_spots

int missed_cars = 0;  //Total times a car couldn't park during entire runtime
pthread_mutex_t lock_missed;  //Mutex to change missed_cars

int next_car = 0; //The time of the next arriving car
pthread_mutex_t lock_next_car;

pthread_barrier_t barrier_a;
pthread_barrier_t barrier_b;

void increment_iterations(int i){
  pthread_mutex_lock(&iteration_lock);
  iterations_executed += i;
  pthread_mutex_unlock(&iteration_lock);
}

void increment_open_spots(int i){
  pthread_mutex_lock(&lock_open);
  open_spots += i;
  pthread_mutex_unlock(&lock_open);
}

void increment_missed_cars(int i){
  pthread_mutex_lock(&lock_missed);
  missed_cars += i;
  pthread_mutex_unlock(&lock_missed);
}

//Generated using Inverse Transform (Exponential Dist.)
double get_next_car(double lambda){ 
  
  //Retrieve the time of the next car
  double time = next_car;        
    double u = (double)(random())/RAND_MAX;
    double interval = -log(u) / lambda;
    next_car += interval;
  
  return time;
}

//Generate using Acceptance/Rejection (Normal Dist.)
double generate_stay_time(double gauss_mean, double std_dev){ 
  double c =  1 / sqrt(2*M_PI * pow(std_dev,2)) ;
  double M = c *  exp( -(pow(1-gauss_mean, 2) ) / (2*pow(std_dev,2)) + 1);

  double u;
  double u2;
  while(1){
    u = (double)(random())/RAND_MAX;
    double a = (double)(random())/RAND_MAX;
    u2 = -log(a); //Let lambda = 1

    if( u <=  (c * exp(-1 * pow(u2 - gauss_mean , 2) / (2*pow(std_dev, 2)) )) 
      / (M * exp(-u2)) ){
      return u2;
    }
  }
}

void thread_main(void* param){
  int local_missed_cars = 0;
  int local_open_spots = 0;
  int local_iterations = 0;
  struct thread_params* p = (struct thread_params*)param; 

  for(int i=1; i <= p->total_iterations; i++){ 
    double next_car_time = 0; //The time of next arriving car
    int parked = 0; //Let 0 mean the car has not yet parked

    //Insert car into first slot to prevent race condition
    pthread_mutex_lock(&lock[0]);
    next_car_time = get_next_car(1/p->exp_mean);
    if (spot[0] < next_car_time){ //If the spot is available
      local_open_spots++;
      if( parked == 0 ){ 
        parked = 1;
        //Updating the next available time the spot is free
        spot[0] = next_car_time + 
          generate_stay_time(p->gauss_mean, p->gauss_mean/4);
      }
    }

    //Lock is only released when thread is guaranteed to have the next spot
    for (int j = 1; j < p->TOTAL_SPOTS; j++){  
      pthread_mutex_lock(&lock[j]);
      pthread_mutex_unlock(&lock[j-1]);
      if (spot[j] < next_car_time){ //If the spot is available
        local_open_spots++;
        if( parked == 0 ){ 
          parked = 1;
          spot[j] = next_car_time + 
            generate_stay_time(p->gauss_mean, p->gauss_mean/4);
        }
      }
    }
    pthread_mutex_unlock(&lock[p->TOTAL_SPOTS -1]);

    // If you still haven't parked after checking each spot
    if(parked == 0)
      local_missed_cars++;
    
    //Check convergence every 1 million iterations
    if(i % 1000000 == 0){
      double prev_open_spots_average = 0;
      double prev_missed_cars_average = 0;
      if(iterations_executed != 0){
        prev_open_spots_average = open_spots/iterations_executed;
        prev_missed_cars_average = missed_cars/iterations_executed;
      }

      pthread_barrier_wait(&barrier_a);

      increment_iterations(local_iterations);
      increment_open_spots(local_open_spots);
      increment_missed_cars(local_missed_cars);
      local_iterations = 0;
      local_open_spots = 0;
      local_missed_cars = 0;

      pthread_barrier_wait(&barrier_b);

      //Check Convergences
      double curr_open_spots_average = open_spots/iterations_executed;
      double curr_missed_cars_average = missed_cars/iterations_executed;

      if(curr_open_spots_average - prev_open_spots_average < error_bound &&
        curr_missed_cars_average - prev_missed_cars_average < error_bound){
          //Proceed to exit thread
        break;
      }
     
    } //End of Convergence Conditional

    local_iterations++;

  } //End of Loop

  //Increment one last time before exiting thread
  //Incase total_iterations %1000000 != 0
  increment_iterations(local_iterations);
  increment_open_spots(local_open_spots);
  increment_missed_cars(local_missed_cars);

  //Close all threads, but not the main thread
  if(pthread_equal(pthread_self(), p->main_thread))
    return;	
  else
    pthread_exit(NULL); 
}

int main(int argc, char **argv){
  struct timespec begin;
  clock_gettime(CLOCK_MONOTONIC, &begin);
  init_sequence(100); //Setting lag table

  if(argc != 4 && argc !=5){
    printf("Please enter all your variables!\n");
    pthread_exit(NULL);
  }

  //Initializing parameters that all thread will execute upon
    struct thread_params params;
    params.TOTAL_SPOTS = atoi(argv[1]);
    if(params.TOTAL_SPOTS <= 0){
      printf("Please enter a valid parking spot amount.\n");
      pthread_exit(NULL);
    }
    if(argc == 4)
      params.TOTAL_THREADS = 4;
    else
      params.TOTAL_THREADS = atoi(argv[4]);//Check for not 0 threads
    if(atof(argv[2]) > 0 && atof(argv[3]) > 0){
      params.exp_mean = atof(argv[2]);
      params.gauss_mean = atof(argv[3]);
    }
    else{
      printf("Please enter valid mean values.\n");
      pthread_exit(NULL); 
    }
    params.total_iterations = MAX_ITERATIONS/params.TOTAL_THREADS;
    params.main_thread = pthread_self();


  //Initializing mutex locks needed to update global vars
    pthread_mutex_init(&lock_open, NULL);
    pthread_mutex_init(&lock_missed, NULL);
    pthread_mutex_init(&lock_next_car , NULL);
    pthread_mutex_init(&iteration_lock, NULL);

  //Initiating barriers
    pthread_barrier_init(&barrier_a, NULL, params.TOTAL_THREADS);
    pthread_barrier_init(&barrier_b, NULL, params.TOTAL_THREADS);

  //initiate spots with time 0.
    spot = (double *)malloc(sizeof(double) * params.TOTAL_SPOTS);
    for (int i = 0; i < params.TOTAL_SPOTS; i++){
      spot[i] = 0.0;
    }

  //Init array of mutex per spot;
    lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t)*params.TOTAL_SPOTS);
    for(int i=0; i<params.TOTAL_SPOTS; i++){
      pthread_mutex_init(&lock[i], NULL);
    }

  //Creating all threads
    thread = (pthread_t*)malloc(sizeof(pthread_t) * params.TOTAL_THREADS);
    for(int i=0; i<params.TOTAL_THREADS - 1; i++){
      int ret = pthread_create(&thread[i], NULL, (void*)thread_main, (void*)&params);
      if( ret != 0){
        printf("Error when creating thread. Exiting\n");
        pthread_exit(NULL);
      }
    }

  //The main thread will also execute thread_main
  thread_main((void*)&params);

  //Wait for all threads
    for (int i = 0; i < params.TOTAL_THREADS-1; i++){
      int ret = pthread_join(thread[i], NULL);
      if(ret != 0){
        printf("%d : Error on Join. Exiting Program. %d\n",i ,ret);
        pthread_exit(NULL);
      }
    } 

  //Printing Results
  double missed_car_prob = (double)missed_cars / iterations_executed;
  double average_open_spots = (double)open_spots / iterations_executed; 

  printf("A %f %% chance of a car not parking\n", missed_car_prob*100);
  printf("An average of %f open spots\n" , average_open_spots);

  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &end);
  double total_time = end.tv_sec - begin.tv_sec;
  total_time += (end.tv_nsec - begin.tv_nsec) / 1000000000.0; //Divide by 10^9

  printf("Total Time %f second(s) \n", total_time);

  pthread_exit(NULL);
}
