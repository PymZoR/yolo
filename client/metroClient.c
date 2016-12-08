#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#define TRUE 1
#define FALSE 0

// Thread internals
char threadRun;
pthread_t threadId[2];
int threadErr;
int *threadPtr[2];

// Socket internals
int sockfd;
int portno;
int n;

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

typedef struct Messages {
    int message_id;
    int arg1;
    int arg2;
    int arg3;
} Message;

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

void displayParameters() {
    printf("TimeSig %i/%i, Tempo %i, Run %i.\n", timeSig[0], timeSig[1], tempo, run);
}

void error(const char *msg) {
    perror(msg);
    exit(0);
}

void sendSocket(Message *message) {
    char buffOut[256];

    switch(message->message_id) {
        case 0: {
            sprintf(buffOut, "Quit");
            break;
        }

        case 1: {
            sprintf(buffOut, "Stop");
            break;
        }

        case 2: {
            printf("SEND >> TimeSig %i/%i, Tempo %i, Start", message->arg1, message->arg2, message->arg3);
            sprintf(buffOut, "TimeSig %i/%i, Tempo %i, Start", message->arg1, message->arg2, message->arg3);
            break;
        }

        default: {}
    }

    n = write(sockfd, buffOut, strlen(buffOut));
    if (n < 0) {
        error("ERROR writing to socket");
    }
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

        Message message;

        switch(c) {
            // Quit
            case 'q': {
                threadRun = FALSE;
                message.message_id = 0;
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
                message.message_id = 2;
                message.arg1 = timeSig[0];
                message.arg2 = timeSig[1];
                message.arg3 = tempo;
                displayParameters();
                break;
            }

            // Dec tempo
            case 'c': {
                tempo -= 5;
                if (tempo < 30) {
                    tempo = 30;
                }
                message.message_id = 2;
                message.arg1 = timeSig[0];
                message.arg2 = timeSig[1];
                message.arg3 = tempo;
                displayParameters();
                break;
            }

            // Inc tempo
            case 'b': {
                tempo += 5;
                if (tempo > 300) {
                    tempo = 300;
                }
                message.message_id = 2;
                message.arg1 = timeSig[0];
                message.arg2 = timeSig[1];
                message.arg3 = tempo;
                displayParameters();
                break;
            }

            // Start/stop
            case 'm': {
                run = !run;
                message.message_id = 1;
                displayParameters();
                break;
            }

            default: {}
        }

        fflush(stdout);
        sendSocket(&message);
    }

    pthread_exit(NULL);

    return NULL;
}

void* displayOutputThread(void *arg) {
    pthread_t id = pthread_self();

    if(!pthread_equal(id,threadId[1])) {
        return NULL;
    }

    char buffIn[256];
    while (threadRun) {
        bzero(buffIn,256);

        n = read(sockfd, buffIn, 255);

        if (n <= 0) {
             error("ERROR reading from socket");
        }
        printf("\nRECV << %s\n", buffIn);
    }

    pthread_exit(NULL);

    return NULL;
}

void createThreads() {
    threadRun = TRUE;

    threadErr = pthread_create(&(threadId[0]), NULL, &getInputThread, NULL);
    if (threadErr != 0) {
        printf("\ncan't create thread :[%s]", strerror(threadErr));
    }

    threadErr = pthread_create(&(threadId[1]), NULL, &displayOutputThread, NULL);
    if (threadErr != 0) {
        printf("\ncan't create thread :[%s]", strerror(threadErr));
    }
}

void waitForThreads() {
    pthread_join(threadId[0], (void**)&(threadPtr[0]));
    pthread_join(threadId[1], (void**)&(threadPtr[1]));
}

int main(int argc, char *argv[]) {
    // Open socket connection
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    // Init termios: Disable buffered IO with arg 'echo'
    int echo = 0;
    init_termios(echo); // Disable echo

    // Set default values
    singId     = 1;
    timeSig[0] = SIGNATURES[singId][0];
    timeSig[1] = SIGNATURES[singId][1];
    tempo      = 90;
    run        = TRUE;

    // Print title & menu
    displayMenu();
    displayParameters();

    createThreads();
    waitForThreads();

    // Reset termios
    reset_termios();

    // close socket
    close(sockfd);
    return 0;
}
