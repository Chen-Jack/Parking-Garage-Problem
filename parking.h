#ifndef PARKING_H
#define PARKING_H

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
  double M = c *  exp( -1 * ( (pow(1-gauss_mean, 2) ) /(2*pow(std_dev,2)) ) + 1);

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
void check_spot(struct thread_params p, int spot_index, double next_car_time, int* parked){

  pthread_mutex_lock(&lock[spot_index]);

  if (spot[spot_index] < next_car_time){
    increment_open_spots();
    if( *parked == 0 ){ //parked is false
      printf("Parking\n");
      *parked = 1;
      spot[spot_index] = next_car_time + generate_stay_time(p.gauss_mean, p.gauss_mean/4);
    }
  }

  pthread_mutex_unlock(&lock[spot_index]);
}

#endif