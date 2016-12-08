#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

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

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	char buffer[256];
   char exitStr[] = "exit";
	struct sockaddr_in serv_addr, cli_addr;
	int n;
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
	do {
		bzero(buffer,256);
		n = read(newsockfd,buffer,255);
		if (n < 0) error("ERROR reading from socket");
      buffer[strlen(buffer) - 1] = '\0';

      printf("\n---------------------\n> %s\n", buffer);

      if(startsWith(buffer, "TimeSig")) {
         char numeratorStr[255], denominatorStr[255], tempoStr[255];
         int numerator, denominator, tempo;
         if(sscanf("TimeSig %s/%s, Tempo %s, Start", numeratorStr, denominatorStr, tempoStr) == 1) {
            numerator = atoi(numeratorStr);
            denominator = atoi(denominatorStr);
            tempo = atoi(tempoStr);
            printf("Received command TimeSig with fraction [%d/%d] with tempo [%d]\n", numerator, denominator, tempo);
            //TODO: do something with numerator, denominator and tempo


            //Now, notify client that everything works fine
            n = write(newsockfd,"Received command TimeSig",24);
      		if (n < 0) error("ERROR writing to socket");
         }
         else {
            n = write(newsockfd,"Received malformed command TimeSig",34);
      		if (n < 0) error("ERROR writing to socket");
            error("Incorrect command from client");
         }
      }
      else if (startsWith(buffer, "Stop")) {
         printf("Received command Stop\n");
         //TODO: stop

         n = write(newsockfd,"Received command Stop",21);
         if (n < 0) error("ERROR writing to socket");
      }
      else if (startsWith(buffer, "Quit")) {
         printf("Received command Quit\n");
         //TODO: quit

         n = write(newsockfd,"Received command Quit",21);
         if (n < 0) error("ERROR writing to socket");
      }
      else {
         n = write(newsockfd,"Received unknow command",23);
         printf("Sending unknow command. Status: %d\n", n);

         if (n < 0) error("ERROR writing to socket");
         //error("Received unknow command");
      }
	} while(strcmp(buffer, exitStr));
	close(newsockfd);
	close(sockfd);
	return 0;
}
