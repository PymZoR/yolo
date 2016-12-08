#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <math.h>

#define TRUE 1
#define FALSE 0

#define NSEC_PER_SEC 1000000000L
#define timerdiff(a,b) ((float)((a)->tv_sec - (b)->tv_sec) + \
((float)((a)->tv_nsec - (b)->tv_nsec))/NSEC_PER_SEC)

// Constants
char pattern2[2] = {'#', '!'};
char pattern3[3] = {'#', '!', '!'};
char pattern4[4] = {'#', '!', '+', '!'};
char pattern6[6] = {'#', '!', '!', '+', '!', '!'};

char *curPatern;

// Parameters
unsigned short timeSig[2] = {2, 4};
unsigned short tempo = 90;
int run = FALSE;

// Internals
float blinkPeriod;
int note = 0;
int blinkState = FALSE;
static struct timespec prev = {.tv_sec=0,.tv_nsec=0};

// socket
int sockfd, newsockfd, portno;
char buffer[256];
socklen_t clilen;
char exitStr[] = "exit";
struct sockaddr_in serv_addr, cli_addr;
int n;

// thread
pthread_t threadId;
int *threadPtr;

void timerHandler(int signo) {
	// If not running, return immediatly
    if (!run) {
        return;
    }

	#ifdef BB
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
    // Get elapsed time
    float diff = timerdiff(&now, &prev);

    // Not enough time elapsed, return
    if (diff < blinkPeriod) {
        return;
    }
    prev = now;
	#endif

    if (!blinkState) {
        // Output next note
        note++;
        if (note > timeSig[0]) {
            note = 1;
        }

		// Display
        printf("%i", curPatern[note-1]);

		// Send to client
		char buffOut[256];
		sprintf(buffOut, "%c", curPatern[note-1]);
		write(newsockfd, buffOut, strlen(buffOut));

		// Set leds
        #ifdef BB
            setLeds(note);
        #endif
    } else {
        #ifdef BB
            clearLeds();
        #endif
    }

    blinkState = !blinkState;
	fflush(stdout);
}

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

int startsWith(const char *a, const char *b)
{
	if(strncmp(a, b, strlen(b)) == 0) return 1;
	return 0;
}

void* getInputThread(void *arg) {
	pthread_t id = pthread_self();

	if(!pthread_equal(id,threadId)) {
		return NULL;
	}

	// Init timer
	#ifdef BB
		initTimer(&prev, timerHandler);
	#endif

	do {
		bzero(buffer,256);
		n = read(newsockfd, buffer, 255);
		if (n < 0) error("ERROR reading from socket");

		if(strlen(buffer) > 0) {
			printf("\n---------------------\n> %s\n", buffer);

			if(startsWith(buffer, "TimeSig")) {
				int numerator, denominator, tempo_;
				if(sscanf(buffer, "TimeSig %d/%d, Tempo %d, Start", &numerator, &denominator, &tempo_) > 0) {
					printf("Received command TimeSig with fraction [%d/%d] with tempo [%d]\n", numerator, denominator, tempo_);

					if (numerator == 2)
						curPatern = pattern2;
					else if (numerator == 3)
						curPatern = pattern3;
					else if (numerator == 4)
						curPatern = pattern4;
					else if (numerator == 6)
						curPatern = pattern6;

					timeSig[0] = numerator;
					timeSig[1] = denominator;
					tempo = tempo_;

					// Time for 1/4 note per mesure
					float quarterTime = 60.0/tempo;

					// Number of 1/4 per note
					int divider = timeSig[1]/4;

					// Divided by two for blinking : half the time led is up, other half is down
					blinkPeriod = (quarterTime / divider / 2);

					run = TRUE;
					// DEBUG : Now, notify client that everything works fine
					//n = write(newsockfd,"Received command TimeSig",24);
					if (n < 0) error("ERROR writing to socket");
				}
				else {
					//n = write(newsockfd,"Received malformed command TimeSig",34);
					if (n < 0) error("ERROR writing to socket");
					error("Incorrect command from client");
				}
			}
			else if (startsWith(buffer, "Stop")) {
				printf("Received command Stop\n");

				run = !run;

				//n = write(newsockfd,"Received command Stop",21);
				if (n < 0) error("ERROR writing to socket");
			}
			else if (startsWith(buffer, "Quit")) {
				printf("Received command Quit\n");
				//TODO: quit

				//n = write(newsockfd,"Received command Quit",21);
				if (n < 0) error("ERROR writing to socket");
			}
			else {
				//n = write(newsockfd,"Received unknow command",23);
				printf("Sending unknow command. Status: %d\n", n);

				if (n < 0) error("ERROR writing to socket");
				//error("Received unknow command");
			}
		}
	} while(strlen(buffer) > 0);

	pthread_exit(NULL);
	return NULL;
}

int main(int argc, char *argv[])
{
	// Init mmap
	#ifdef BB
		initMmap();
	#else
		printf("DEBUG >> host is PC\n\n");
	#endif

	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
	         sizeof(serv_addr)) < 0)
		error("ERROR on binding");
	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	printf("Server started on port [%d], waiting for client...\n", portno);
	newsockfd = accept(sockfd,
	                   (struct sockaddr *) &cli_addr,
	                   &clilen);
	if (newsockfd < 0)
		error("ERROR on accept");
	printf("New client connected, waiting for commands...\n");

	int threadErr = pthread_create(&threadId, NULL, &getInputThread, NULL);
    if (threadErr != 0) {
        printf("\ncan't create thread :[%s]", strerror(threadErr));
    }

	pthread_join(threadId, (void**)&(threadPtr));

	// Close socket
	close(newsockfd);
	close(sockfd);

	// Close mmap
	#ifdef BB
		closeMmap();
	#endif
	return 0;
}
