#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>


void initTimer(struct timespec *prev, void (*timerHandler)(int)){
    int i = 0;
    timer_t t_id;

    struct itimerspec tim_spec = {.it_interval= {.tv_sec=0,.tv_nsec=10000000},
    .it_value = {.tv_sec=0,.tv_nsec=10000000}};

    struct sigaction act;
    sigset_t set;

    sigemptyset( &set );
    sigaddset( &set, SIGALRM );

    act.sa_flags = 0;
    act.sa_mask = set;
    act.sa_handler = timerHandler;

    sigaction( SIGALRM, &act, NULL );

    if (timer_create(CLOCK_MONOTONIC, NULL, &t_id))
    perror("timer_create");

    if (timer_settime(t_id, 0, &tim_spec, NULL))
    perror("timer_settime");

    clock_gettime(CLOCK_MONOTONIC, prev);

}

void resetTimer() {
    struct sigaction act;
    sigset_t set;

    sigemptyset( &set );
    sigaddset( &set, SIGALRM );

    act.sa_flags = 0;
    act.sa_mask = set;
    act.sa_handler = SIG_DFL;
    sigaction( SIGALRM, &act, NULL );
}
