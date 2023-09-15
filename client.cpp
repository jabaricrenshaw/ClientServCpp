#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT            "6160"
#define MAXDATASIZE     256

void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char **argv){
    struct addrinfo hints, *servinfo, *p;
    char s[INET6_ADDRSTRLEN];
    int sock_fd, numbytes;
    char buf[MAXDATASIZE];
    int rv;
    

    if(argc != 2){
        fprintf(stderr, "Usage: %s {HOSTNAME}\n", argv[0]);
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0){
        fprintf(stderr, "[E]\tgetaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next){
        if((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            perror("[E]\tFailed to create socket.");
            continue;
        }

        if(connect(sock_fd, p->ai_addr, p->ai_addrlen) == -1){
            close(sock_fd);
            perror("[E]\tFailed to connect.");
            continue;
        }

        break;
    }

    if(p == NULL){
        fprintf(stderr, "[E]\tFailed to connect.\n");
        return 2;
    }
    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr), s, sizeof(s));
    printf("[I]\tConnecting to %s\n", s);
    
    freeaddrinfo(servinfo);

    if((numbytes = recv(sock_fd, buf, MAXDATASIZE - 1, 0)) == -1){
        perror("[E]\tCould not receive message (Too large).\n");
        exit(1);
    }
    
    buf[numbytes] = '\0';

    printf("[M]\t'%s'\n", buf);

    close(sock_fd);

    return 0;
}