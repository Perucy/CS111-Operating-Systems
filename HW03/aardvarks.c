#include "/comp/111/assignments/aardvarks/anthills.h"
#include <semaphore.h>
#include <time.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0

int initialized = FALSE;

int rem_ants[ANTHILLS];
sem_t anthill_sem[ANTHILLS];
pthread_mutex_t anthill_lock[ANTHILLS];
struct timespec timer;
int finish = FALSE;
/************************************************************** 
 * Notes: In this revised code,  instead of accessing the hills
 * sequentially, we access the hills randomly.
 * Using a thread for the slurp routine
 * Using a timer to time 1 sec slurping
 * Focused on not sulking
 * Used static for a faster run of the program
 * 
********************************************************************/

/******************initialize_resources*******************************
 * 
 * Parameters: None
 * Return: None
 * Notes: the function initializes resources (mutex locks, semaphores and
 * array to track the number of ants left on anthills)
 */
static void initialize_resources() {
    timer.tv_sec = 1;
    timer.tv_nsec = 600000000;
    for (int i = 0; i < ANTHILLS; i++) {
        sem_init(&anthill_sem[i], 0, AARDVARKS_PER_HILL);
        pthread_mutex_init(&anthill_lock[i], NULL);
        rem_ants[i] = ANTS_PER_HILL;
    }
}

/**********************************************************************
 * 
 * argumets: aardvark name and hill number
 * return: none
 * Notes: implements a slurp routine
 ***********************************************************************/
static void my_slurp(char aname, int hill) {
    pthread_mutex_lock(&anthill_lock[hill]);
    if (rem_ants[hill] > 0) {
        rem_ants[hill]--;
        pthread_mutex_unlock(&anthill_lock[hill]);
        
        if (slurp(aname, hill) != 1) {
            pthread_mutex_lock(&anthill_lock[hill]);
            rem_ants[hill]++;
            pthread_mutex_unlock(&anthill_lock[hill]);
        }
    } else {
        pthread_mutex_unlock(&anthill_lock[hill]);
    }
}
/****************************************************************
 * 
 * arguments: a semaphore to the hill
 * returns: NULL
 * Notes: sets a timer for 1 second to slurp
 ****************************************************************/
static void *slurp_timer(void *timer_sem) {
    nanosleep(&timer, NULL);
    sem_post((sem_t *)timer_sem);
    return NULL;
}

void *aardvark(void *input) {
    char aname = *(char *)input;
    int hill;
    unsigned int rand = time(NULL) ^ pthread_self();

    //initializing mutexes, semaphores etc
    pthread_mutex_lock(&init_lock);
    if (!initialized++) {
        initialize_resources();
    }
    pthread_mutex_unlock(&init_lock);

    //now be an aardvark
    while (chow_time()) {
        hill = rand_r(&rand) % ANTHILLS;
        
        if (sem_trywait(&anthill_sem[hill]) == 0) {
            pthread_t my_timer;
            pthread_create(&my_timer, NULL, slurp_timer, &anthill_sem[hill]);
            pthread_detach(my_timer);
            
            my_slurp(aname, hill);
        }
    }

    //cleaning after sync
    pthread_mutex_lock(&init_lock);
    if (++finish == AARDVARKS) {
        for (int i = 0; i < ANTHILLS; i++) {
            sem_destroy(&anthill_sem[i]);
            pthread_mutex_destroy(&anthill_lock[i]);
        }
    }
    pthread_mutex_unlock(&init_lock);
    
    return NULL;
}