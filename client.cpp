#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg){
    perror(msg);
    exit(0);
}

int main(int argc, char **argv){
    int sockfd, portno, n;
    char buf[256];
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    if(argc < 3){
        fprintf(stderr, "Usage: %s {HOSTNAME} {PORT}.\n", argv[0]);
        exit(0);
    }

    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        error("[i]\tERROR: Cannot open socket.\n");
    }

    server = gethostbyname(argv[1]);
    //
    printf("%s\n", server->h_name);
    //
    if(server == NULL){
        fprintf(stderr, "[i]\tERROR: Cannot find host.\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    printf("Please enter the meassge: ");
    bzero(buf, 256);
    fgets(buf, 255, stdin);
    n = write(sockfd, buf, strlen(buf));
    if(n < 0){
        error("[i]\tERROR: Cannot read from socket.\n");
    }
    printf("%s", buf);

    return 0;

}