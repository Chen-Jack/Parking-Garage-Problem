#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>

struct thread_params{
  int total_iterations;
  double gauss_mean;
  double exp_mean;
  int TOTAL_SPOTS;
  int TOTAL_THREADS;
};
char * init_sequence ( int state_size ){
  char * state ;
  state = ( char *) malloc ( state_size * sizeof ( char ) ) ;
  if ( NULL != state )
    initstate ( time ( NULL ) , state , state_size ) ;
    return state ;
}
void finalize ( char * state ){
  free ( state ) ;
}

//Global Variables
//1 lock per spot
int MAX_ITERATIONS = 1000000;
double *spot; //An array of all spots
pthread_mutex_t *lock; //An array of mutexes for each spot
pthread_t* thread; //An array of thread ids
int open_spots = 0;
pthread_mutex_t lock_open; //Mutex to change open_spots
int missed_cars = 0;
pthread_mutex_t lock_missed;  //Mutex to change missed_cars
int next_car = 0;
pthread_mutex_t lock_next_car;

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

double get_next_car(double lambda){ //Generated using Inverse Transform (Exponential)
  pthread_mutex_lock(&lock_next_car);
  
  double time = next_car;        //Retrieve the time of the next car
    double u = (double)(random())/RAND_MAX;
    double interval = -log(u) / lambda;
    next_car += interval;
  
  pthread_mutex_unlock(&lock_next_car);
  return time;
}

double generate_stay_time(double gauss_mean, double std_dev){ //Generate using Acceptance/Rejection (Normal)
  /*
    Accept u2 if u <= f(u2)/M * g(u2)
  //pdf of normal distr(non standard)
    f(x) = (1/2*pi*stdev^2)* e^(-1 * (x - mean)^2/ 2*stdev^2)

  //pdf of exponential distr(where exp mean is 1)
    g(x) = e^-x

  M is a const such that M*g(x) >= f(x) or  M >= f(x)/g(x), 
  Dividing our normal dist with exp dist, we ge the following. 
  
  M>= 1/(2pi*std_dev^2) * e^(-(x-mean/2std^2) + x) 
 */
  double c =  1 / sqrt(2*M_PI * pow(std_dev,2)) ;
  double M = c *  exp( -1 * ( (pow(1-gauss_mean, 2) ) /(2*pow(std_dev,2)) ) + 1);

  double u;
  double u2;

  while(1){
    u = (double)(random())/RAND_MAX;
    u2 = (double)(random())/RAND_MAX;

    if( u < ( (c * exp(-1 * pow(u2 - gauss_mean , 2) / 2*pow(std_dev, 2) )) / (M * exp(-1 * u2))) ){
      return u2;
    }
    // printf("Reject, trying again\n");
  }
}

void foo(void* param){
  int local_missed_cars = 0;
  int local_open_spots = 0;
  pthread_t id = pthread_self();
  struct thread_params* p = (struct thread_params*)param; 

  for(int i=0; i< p->total_iterations; i++){
    // printf("%d\n",i);

    double next_car_time= get_next_car(1/p->exp_mean); //1/exp mean is lambda
    
    int s = rand() % (p->TOTAL_SPOTS+1);

    int parked = 0; //Reset this flag for every new car
    for (int j = 0; j < p->TOTAL_SPOTS; j++){  
      s = (s + j) % p->TOTAL_SPOTS;

      if (spot[j] < next_car_time){//If the spot is available
        local_open_spots++;
        if( parked == 0 ){ //parked is false
          parked = 1;
          spot[j] = next_car_time+ generate_stay_time(p->gauss_mean, p->gauss_mean/4);
        }
      }
    }

    // If you still haven't parked aka parked == 0 after checking each spot
    if(parked == 0){
      local_missed_cars++;
    }
  }

  //Update the global variables with these local thread variables
  increment_open_spots(local_open_spots);
  increment_missed_cars(local_missed_cars);
  pthread_exit(NULL);
}

int main(int argc, char **argv)
{
  time_t start = time(NULL);
  init_sequence(1000); //Setting lag table

  if(argc != 4 && argc !=5){
    printf("Please enter all your variables man!\n");
    return 0;
  }

  //Initializing params that all thread will execute upon
  struct thread_params params;
    params.TOTAL_SPOTS = atoi(argv[1]);
    if(argc == 4)
      params.TOTAL_THREADS = 4;
    else
      params.TOTAL_THREADS = atoi(argv[4]);
    params.total_iterations = MAX_ITERATIONS/params.TOTAL_THREADS + 1;
    params.exp_mean = atof(argv[2]);
    params.gauss_mean = atof(argv[3]);


  //Initializing mutex locks needed to update global vars
  pthread_mutex_init(&lock_open, NULL);
  pthread_mutex_init(&lock_missed, NULL);
  pthread_mutex_init(&lock_next_car , NULL);

  //initiate spots with time 0.
  spot = (double *)malloc(sizeof(double) * params.TOTAL_SPOTS);
  for (int i = 0; i < params.TOTAL_SPOTS; i++){
    spot[i] = 0.0;
  }

  //init array of mutex per spot;
  lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t)*params.TOTAL_SPOTS);
  for(int i=0; i<params.TOTAL_SPOTS; i++){
    pthread_mutex_init(&lock[i], NULL);
  }


  //Creating all threads
  thread = (pthread_t*)malloc(sizeof(pthread_t) * params.TOTAL_THREADS);
  for(int i=0; i<params.TOTAL_THREADS; i++){
    int ret = pthread_create(&thread[i], NULL, (void*)foo, (void*)&params);
    if( ret != 0){
      printf("Error when creating thread. Exiting\n");
      pthread_exit(NULL);
    }
  }

  if(1){
    int n;
    scanf("%d", &n);
  }

  //WAIT UNTIL AFTER ALL OTHER THREADS ARE DONE
  for (int i = 0; i < params.TOTAL_THREADS; i++){
    printf("Waiting to join %d\n",i);
    int ret = pthread_join(thread[i], NULL);
    if(ret != 0)
      printf("%d : Error %d\n",i ,ret);
    else
     printf("%d : Success\n" ,ret);
  } 


  double missed_car_prob = (double)missed_cars / MAX_ITERATIONS;
  double average_open_spots = (double)open_spots / MAX_ITERATIONS; 

  printf("A %f chance of a car not parking\n", missed_car_prob*100);
  printf("An average of %f open spot\n" , average_open_spots);

  time_t stop = time(NULL);
  time_t total_time = stop - start;
  printf("Total Time %ld second(s) \n", total_time);

  pthread_exit(NULL);
}