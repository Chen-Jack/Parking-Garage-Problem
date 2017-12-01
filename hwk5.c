#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>

struct thread_params{
  int total_iterations;
  double gauss_mean;
  double exp_mean;
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
double *spot; //An array of all spots
pthread_mutex_t *lock; //An array of mutex locks on each slot
pthread_t *thread; //An array of thread ids

int open_spots = 0;
pthread_mutex_t lock_open;

int missed_cars = 0;
pthread_mutex_t lock_missed;

void increment_open_spots(){
  pthread_mutex_lock(&lock_open);
  printf("incrementing open spots\n");
  open_spots++;
  pthread_mutex_unlock(&lock_open);
}

void increment_missed_cars(){
  pthread_mutex_lock(&lock_missed);
  // printf("missed car\n");
  missed_cars++;
  pthread_mutex_unlock(&lock_missed);
}

double get_next_car(double lambda){ //Generated using Inverse Transform (Exponential)
  double u = (double)(random())/RAND_MAX;
  return -log(u) / lambda;
}

double generate_stay_time(double gauss_mean, double std_dev){ //Generate using Acceptance/Rejection (Normal)
  /*
    Accept u2 if 
    U <= f(u2)/M * g(u2)
  */

  //pdf of normal distr(non standard)
  /*
    f(x) = (1/2*pi*stdev^2)* e^(-1 * (x - mean)^2/ 2*stdev^2)
  */

  //pdf of exponential distr(where exp mean is 1)
  /*
    g(x) = e^-x
  */

  /* M is a const such that M*g(x) >= f(x) or  M >= f(x)/g(x), 
  Dividing our normal dist with exp dist, we ge the following. 
  
  M>= 1/(2pi*std_dev^2) * e^(-(x-mean/2std^2) + x) 
 */
  double c =  1 / sqrt(2*M_PI * pow(std_dev,2)) ;
  double M = 
    c *  exp( 01 * ( (pow(1-gauss_mean, 2) ) /(2*pow(std_dev,2)) ) + 1);

  double u;
  double u2;

  while(1){
    u = (double)(random())/RAND_MAX;
    u2 = (double)(random())/RAND_MAX;

    if( u < ( (c * exp(-1 * pow(u2 - gauss_mean , 2) / 2*pow(std_dev, 2) )) / (M * exp(-1 * u2))) ){
      return u2;
    }
    printf("reject, trying again\n");
  }
}

//Attempts to park in this spot
void check_spot(int spot_index, double next_car_time, int* parked){

  pthread_mutex_lock(&lock[spot_index]);

  if (spot[spot_index] < next_car_time){
    increment_open_spots();
    if( *parked == 0 ){ //parked is false
      printf("Parking\n");
      *parked = 1;
      spot[spot_index] = next_car_time + generate_stay_time();
    }
  }

  pthread_mutex_unlock(&lock[spot_index]);
}


void foo(void* param){
  struct thread_params* p = (struct thread_params*)param; 

  for(int i=0; i< p->total_iterations; i++){
    double next_time = get_next_car(1/p->exp_mean);
    int parked = 0; //Reset this flag every loop

    for (int j = 0; j < 3; j++){  //total spots??
      check_spot(j, next_time, &parked);
    }

    // If you reached this part of the code, then the break statement didnt execute
    if(parked == 0){
      increment_missed_cars();
    }
  }
}

int main(int argc, char **argv)
{
  init_sequence(1000); //Setting lag table

  if(argc != 5){
    printf("Please enter all your variables man!\n");
    return 0;
  }

  int TOTAL_SPOTS = atoi(argv[1]);
  int TOTAL_THREADS = atoi(argv[4]);
  pthread_mutex_init(&lock_open, NULL);
  pthread_mutex_init(&lock_missed, NULL);

  //initiate spots with time 0.
  spot = (double *)malloc(sizeof(double) * TOTAL_SPOTS);
  for (int i = 0; i < TOTAL_SPOTS; i++)
  {
    spot[i] = 0.0;
  }

  //init array of mutex per spot;
    lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t)*TOTAL_SPOTS);
    for(int i=0; i<TOTAL_SPOTS; i++){
      pthread_mutex_init(&lock[i], NULL);
    }

  //Creating all threads, executing parking
  for(int i=0; i<TOTAL_THREADS; i++){
    thread = (pthread_t *)malloc(sizeof(pthread_t) * TOTAL_THREADS);
    //Creation and execution of other threads
    struct thread_params params;
    params.total_iterations = 100000000/TOTAL_THREADS + 1;
    params.exp_mean = atof(argv[2]);
    params.gauss_mean = atof(argv[3]);
    pthread_create(&thread[i], NULL, (void*)foo, (void*)&params);
  }

  //AFTER ALL OTHER THREADS ARE DONE


  while(1){

  }

  //pthreads_exit();
}