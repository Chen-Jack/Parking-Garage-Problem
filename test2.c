#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>


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

int get_n(){
    return rand();
}
int main(){
    init_sequence(100000);

    for(int i=0; i<10; i++){
        double i = (double)get_n() / RAND_MAX;
        printf(" %f \n", i);
    }


}