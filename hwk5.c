#include <stdio.h>
#include <pthreads>

//Global Variables
double* spot;
pthread_mutex_t* lock;
pthread_t* thread;
// int missed_cars = 0;
// int last_checked_spot;

//Attempts to park in this spot
void try_parking(int spot_index, double time){

    pthread_mutex_lock(&lock[spot_index]);
    
    if(spot[spot_index] < time);
        spots[spot_index] = curr_time + generate_stay_time();
    }

    pthread_mutex_unlock(&lock[spot_index]);

    
    //pop queue or unlock
}

void foo(){

    while(1){
        double next_time = generatetime();
        for(int i=0; i<TOTAL_SPOTS; i++){
            try_parking(i, next_time, &lock[i]);
            if(success == 1){
                break; //You already parked, stop looking.
            }
        }

        //If you reached this part of the code, then the break statement didnt execute
        missed_cars++;
    }
}




int main(int argc, char** argv){
    int TOTALS_SPOTS = 3;
    int TOTAL_THREADS = 4;

    //initiate spots with time 0.
    spot = (double*) malloc(sizeof(double)* TOTALS_SPOTS);
    for(int i=0; i<TOTAL_SPOTS; i++){
        spot[i] = 0.0;
    }

    //init array of mutex per spot;
    spots_mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t)*TOTAL_SPOTS;
    for(int i=0; i<TOTAL_SPOTS; i++){
        pthread_mutex_init(&lock[i], NULL);
    }

    //Creating all threads, executing parking
    for(int i=0; i<TOTAL_THREADS; i++){
        thread = (pthread_t *) malloc( sizeof(pthread_t) * TOTAL_THREADS );
        pthread_create(&thread[i], NULL, foo, NULL);
    }


    


    //pthreads_exit();

}