#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <signal.h>

#define TRUE 1
#define FALSE 0

#define NSEC_PER_SEC 1000000000L
#define timerdiff(a,b) ((float)((a)->tv_sec - (b)->tv_sec) + \
((float)((a)->tv_nsec - (b)->tv_nsec))/NSEC_PER_SEC)


// Thread internals
char threadRun;
pthread_t threadId[2];
int threadErr;
int *threadPtr[2];

// Constants
const unsigned short SIGN_NUMBER      = 4;
const unsigned short SIGNATURES[4][2] = {
    {2, 4}, {3, 4}, {4, 4}, {6, 8}
};

// Parameters
unsigned short singId;
unsigned short timeSig[2];
unsigned short tempo;
char run;

// Internals
float blinkPeriod;
int note = 0;
int blinkState = FALSE;
static struct timespec prev = {.tv_sec=0,.tv_nsec=0};

void displayMenu() {
    printf("+---------------------------------------------------------------+\n");
    printf("|                  _                                            |\n");
    printf("|       /\\/\\   ___| |_ _ __ ___  _ __   ___  _ __ ___   ___     |\n");
    printf("|      /    \\ / _ \\ __| '__/ _ \\| '_ \\ / _ \\| '_ ` _ \\ / _ \\    |\n");
    printf("|     / /\\/\\ \\  __/ |_| | | (_) | | | | (_) | | | | | |  __/    |\n");
    printf("|     \\/    \\/\\___|\\__|_|  \\___/|_| |_|\\___/|_| |_| |_|\\___|    |\n");
    printf("|                                                               |\n");
    printf("|                                 _____                         |\n");
    printf("|                          /\\ /\\  \\_   \\                        |\n");
    printf("|                         / / \\ \\  / /\\/                        |\n");
    printf("|                         \\ \\_/ /\\/ /_                          |\n");
    printf("|                          \\___/\\____/                          |\n");
    printf("+---------------------------------------------------------------+\n\n");
    printf("Use five single key inputs without Enter key.\n");
    printf("    'z'    Time signature    Rotates as 2/4 -> 3/4 -> 4/4 -> 8/8\n");
    printf("    'c'    Dec tempo         Dec tempo by 5 (min tempo 30)\n");
    printf("    'b'    Inc tempo         Inc tempo by 5 (max tempo 200)\n");
    printf("    'm'    Start/Stop        Toggle run state\n");
    printf("    'q'    Quit              Quit this program\n\n");
}

void arameters() {
    printf("TimeSig %i/%i, Tempo %i, Run %i.\n", timeSig[0], timeSig[1], tempo, run);
}

void* getInputThread(void *arg) {
    pthread_t id = pthread_self();

    if(!pthread_equal(id,threadId[0])) {
        return NULL;
    }

    char c;
    while (threadRun) {
        c  = getch();
        printf("\n Key %c: ", c);

        switch(c) {
            // Quit
            case 'q': {
                threadRun = FALSE;
                resetTimer();
                break;
            }

            // Time signature
            case 'z': {
                singId += 1;
                if (singId > SIGN_NUMBER-1) {
                    singId = 0;
                }

                timeSig[0] = SIGNATURES[singId][0];
                timeSig[1] = SIGNATURES[singId][1];
                displayParameters();
                break;
            }

            // Dec tempo
            case 'c': {
                tempo -= 5;
                if (tempo < 30) {
                    tempo = 30;
                }
                displayParameters();
                break;
            }

            // Inc tempo
            case 'b': {
                tempo += 5;
                if (tempo > 300) {
                    tempo = 300;
                }
                displayParameters();
                break;
            }

            // Start/stop
            case 'm': {
                run = !run;
                displayParameters();
                break;
            }

            default: {}
        }

        fflush(stdout);

        // Time for 1/4 note per mesure
        float quarterTime = 60.0/tempo;

        // Number of 1/4 per note
        int divider = timeSig[1]/4;

        // Divided by two for blinking : half the time led is up, other half is down
        blinkPeriod = (quarterTime / divider / 2);
    }

    pthread_exit(NULL);

    return NULL;
}

void createThreads() {
    threadErr = pthread_create(&(threadId[0]), NULL, &getInputThread, NULL);
    if (threadErr != 0) {
        printf("\ncan't create thread :[%s]", strerror(threadErr));
    }
}

void waitForThreads() {
    pthread_join(threadId[0], (void**)&(threadPtr[0]));
}

void timerHandler(int signo) {
    struct timespec now;

    // If not running, return immediatly
    if (!run) {
        return;
    }

    clock_gettime(CLOCK_MONOTONIC, &now);

    // Get elapsed time
    float diff = timerdiff(&now, &prev);

    // Not enough time elapsed, return
    if (diff < blinkPeriod) {
        return;
    }
    prev = now;

    if (!blinkState) {
        // Output next note
        note++;
        if (note > timeSig[0]) {
            note = 1;
        }

        unsigned short output = note + 10*(singId+1);
        printf("%i ", output);
        fflush(stdout);

        #ifdef BB
            setLeds(note);
        #endif
    } else {
        #ifdef BB
            clearLeds();
        #endif
    }

    blinkState = !blinkState;
}

int main(void) {
    // Init mmap
    #ifdef BB
        initMmap();
    #else
        printf("Debug: host is PC\n\n");
    #endif

    // Init termios: Disable buffered IO with arg 'echo'
    int echo = 0;
    init_termios(echo); // Disable echo

    // Print title & menu
    displayMenu();
    displayParameters();

    // Set default values
    singId     = 1;
    timeSig[0] = SIGNATURES[singId][0];
    timeSig[1] = SIGNATURES[singId][1];
    tempo      = 90;
    blinkPeriod = 1/3.0;
    run        = TRUE;

    // Create threads
    createThreads();
    threadRun = TRUE;

    // Init timer
    initTimer(&prev, timerHandler);

    // Wait for threads to finish, i.e user quit
    waitForThreads();

    // Reset termios
    reset_termios();

    // Close mmap
    #ifdef BB
        closeMmap();
    #endif

    printf("Thanks to be an awesome user of Metronome UI. Good bye !\n");
    return 0;
}
