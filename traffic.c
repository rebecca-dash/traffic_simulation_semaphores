/*
Rebecca Brown
Project 5b
Traffic lights simulation project using semaphores and threads
*/

//all libraries needed for this project
#include <stdio.h>  //input/output operations
#include <stdlib.h> //memory alloc, process controll, conversions, etc.
#include <unistd.h> //access to POSIX OS API
#include <signal.h> //defines values for signals and declares signal() and raise() functions
#include <pthread.h> //for thread functions/mapping
#include <semaphore.h> //defines the sem_t type, etc.
#include <stdbool.h> //supports booleans
#include <errno.h> //macros for reporting/retrieving error conditions
#include <string.h> //supports string functions/uses

/*
compile with:
gcc -Wall -pedantic -pthread -o traffic traffic.c
*/

//function declarations here
void checkInput(int x, char* arr[]);
void *trafficLightController(void *arg);
void *vehicleController(void *arg);
//label all const here
const unsigned int      VEHICLE_CNT_MIN = 10;
const unsigned int      VEHICLE_CNT_MAX = 500;

const unsigned int      LIGHT_WAIT = 4;
const unsigned int      YELLOW_WAIT = 1;
const unsigned int      TRANSIT_TIME_BASE = 1;
const unsigned int      ARRIVAL_INTERVAL = 2;

const unsigned int      NORTH = 0;
const unsigned int      EAST = 1;
const unsigned int      SOUTH = 2;
const unsigned int      WEST = 3;

//semaphores and parameters will be declared globally
sem_t *semaphores;
int semSize = 4;

//function declarations
int main(int argc, char* argv[])
{
    /*Read and validate command line arguments (in a function)*/
    checkInput(argc, argv);
    int numVehicles = atoi(argv[2]);
    /*display headers*/
    printf("CS 370 Project #5B -> Traffic Light Simulation Project\n\tVehicles: %d\n\n", numVehicles);

    //initialize semaphores
    //dynamically create semaphore array (4)
    semaphores = (sem_t *)malloc(semSize * sizeof(sem_t));
    //check for errors
    if(semaphores == NULL){
        perror("Error allocating memory to semaphore array.\n");
        return 1;
    }
    //if no error initialize each semaphore in the array
    for(int i=0; i<semSize; i++){
        //semaphores initialized with sem_init
        if (sem_init(&semaphores[i], 0, 0) != 0){//pshared = 0, not shared with processes, red = 0
            perror("Failed to initialize semaphore in array.\n");
            free(semaphores);//clear the semaphores in order to rerun
            return 1; //exit
        }
    }

    //create array for threads (one per vehicle) with malloc()
    pthread_t *vehiclethreads = (pthread_t *)malloc(numVehicles * sizeof(pthread_t));
    pthread_t *args = (pthread_t *)malloc(numVehicles * sizeof(pthread_t));//arguments to vehicle threads

    //create traffic light controller thread
    pthread_t trafficLeader; //traffic light controller thread
    if(pthread_create(&trafficLeader, NULL, trafficLightController, NULL) != 0){
        perror("Error creating traffic controller thread.\n");
        return 1;
    }

    //create vehicleCount threads and pass a random direction (north, east, south, west)
    //use rand() % 4
    for(int i=0; i<numVehicles; i++){
        //set thread arguments
        args[i]  = rand() % 4;//generate random number 0-3
        if(pthread_create(&vehiclethreads[i], NULL, vehicleController, &args[i]) != 0){
            //handle error
            perror("Failed to create thread for vehicle.\n");
            return 1;
        }
    }

    // wait for the vehicle threads to complete
    for(int i=0; i<numVehicles; i++){
        //error check
        if(pthread_join(vehiclethreads[i], NULL) != 0){
            perror("Failed to join vehicle thread.\n");//error message
            return 1;//exit process
        }
    }

    //cancel the traffic light controller thread pthread_cancel()
    if(pthread_cancel(trafficLeader) != 0 ){
        perror("Failed to cancel traffic light controller thread.\n");
        return 1;//exit process
    }

    //join light controller thread pthread_join() ?
    if(pthread_join(trafficLeader, NULL) != 0){
        perror("Failed to join thread.\n");
        return 1;
    }

    //destroy the semaphores and free the dynamically allocated arrays
    for(int i = 0; i < semSize; i++){
        sem_destroy(&semaphores[i]);//destroy each semaphore
    }
    //free the dynamically allocated arrays
    free(semaphores);
    free(vehiclethreads);
    //display final completion message
    printf("All vehicles successfully passed through the intersection.\n");

    return 0;
}

//function definitions
void checkInput(int x, char* arr[])
{
    /*check if (-vc <int>) > VEHICLE_CNT_MIN && < VEHICLE_CNT_MAX*/
    if( x < 3){//auto exit if arguments < 3
        printf("Usage, ./traffic -vc <vehicleCount>\n");
        exit(0);
    }
    else
    {
        int length = strlen(arr[1]);
        if(length != 3){ //auto exit if not -vc
            printf("Error, invalid item count specifier.\n");
            exit(0);
        }
        char *specs = arr[1];
        for(int k=0; k<length; k++){
            char currChar = specs[k];
            if( k == 0 && currChar != '-'){
                printf("Error, invalid item count specifier.\n");
                exit(0);
            }
            if(k == 1 && currChar != 'v'){
                printf("Error, invalid item count specifier.\n");
                exit(0);
            }
            if(k == 2 && currChar != 'c'){
                printf("Error, invalid item count specifier.\n");
                exit(0);
            }
        }
        //auto exit if vehicle count is < 10 or > 500
        int num = atoi(arr[2]);
        if(num < VEHICLE_CNT_MIN || num > VEHICLE_CNT_MAX){
            printf("Error, vehicle count value out of range.\n");
            exit(0);
        }  
    }
}


void *trafficLightController(void *arg)
{
    while(true)//loop forever
    {
        //print green light msg for N-S
        printf("\033[0;92mGreen light for North-South\033[0m\n");
        //turn lights green-yellow
        sem_post(&semaphores[NORTH]);
        sem_post(&semaphores[SOUTH]);
        sleep(LIGHT_WAIT);//keep light green for light_wait seconds
        //print yellow light message for N->S warning
        printf("\033[0;93mYellow light for North-South\033[0m\n");
        //sleep for YELLOW_WAIT seconds
        sleep(YELLOW_WAIT);
        //turn light red for north-south direction
        sem_wait(&semaphores[NORTH]);
        sem_wait(&semaphores[SOUTH]);
        printf("\033[0;31mRed light for North-South\033[0m\n");
        //print East West green msg
        printf("\033[0;92mGreen light for East-West\033[0m\n");
        //turn N->S lights E->W via semaphore
        sem_post(&semaphores[EAST]);
        sem_post(&semaphores[WEST]);
        //keep light green for LIGHT_WAIT seconds
        sleep(LIGHT_WAIT);
        //print yellow light msg for E->W warning
        printf("\033[0;93mYellow light for East-West\033[0m\n");
        //wait for yellow seconds
        sleep(YELLOW_WAIT);
        //turn light red for E-W direction
        sem_wait(&semaphores[EAST]);
        sem_wait(&semaphores[WEST]);
        printf("\033[0;31mRed light for East-West\033[0m\n");

    }

   //return a void *
   return NULL;
}

void *vehicleController(void *arg){

    int vehicle_id = *((int *)arg);
    char *direction[] = {"North", "East", "South", "West"};
    //print approaching msg
    sleep(ARRIVAL_INTERVAL);//slowed doen vehicle arrival
    printf("\u2195 Vehicle approaching from %s\n", direction[vehicle_id]);
    //sleep(ARRIVAL_INTERVAL);//extra slowed down vehicle arrival
    sem_wait(&semaphores[vehicle_id]); //vehicle waits for traffic light to be green
    printf("\u2194 Vehicle passing through from %s\n", direction[vehicle_id]);
    //wait for vehicle to pass through intersection
    sleep(TRANSIT_TIME_BASE + usleep(rand() % 5000));
    //release semaphore post --> light rurns red
    sem_post(&semaphores[vehicle_id]);
    //end thread
    return NULL;
}

/*
LIGHT MESSAGES:
printf("\033[0;4mCS 370 Project #5B -> Traffic Light ", "Simulation Project\033[0m\n]")
    printf("\033[0;92mGreen light for North-South\033[0m\n");
    printf("\033[0;93mYellow light for North-South\033[0m\n");
    printf("\033[0;31mRed light for North-South\033[0m\n");
    printf("\u2195 Vehicle approaching from East\n");
    printf("\u2194 Vehicle passing through from East\n");

    NORTH = 0;
    EAST = 1;
    SOUTH = 2;
    WEST = 3;
*/