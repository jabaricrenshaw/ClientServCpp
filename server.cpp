#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <unistd.h>
#include <string.h>
#include <netdb.h> 

void error(const char *msg){
    perror(msg);
    exit(1);
}

int main(int argc, char **argv){
    int sockfd, newsockfd, portno, n;
    socklen_t clilen;
    char buf[256];
    struct sockaddr_in serv_addr, cli_addr;

    if(argc < 2){
        fprintf(stderr, "[i]\tERROR: No port provided.\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        error("[i]\tERROR: Cannot open socket.\n");
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));

    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    
    //
    printf("Port: %u\nAddress: %s\n", portno, inet_ntoa(serv_addr.sin_addr));
    //

    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        error("[i]\tERROR: Cannot bind socket.\n");
    }

    while(true){
        listen(sockfd, 5);

        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if(newsockfd < 0){
            error("[i]\tERROR: On accept.\n");
        }

        bzero(buf, 256);
        n = read(newsockfd, buf, 255);
        if(n < 0){
            error("[i]\tERROR: Cannot read from socket\n");
        }

        printf("Here is the message\n------------\n %s \n------------\n", buf);
        n = write(newsockfd, "I got your message!", 19);
        if(n < 0){
            error("[i]\tERROR: Cannot write to socket");
        }
    }

    return 0;
}