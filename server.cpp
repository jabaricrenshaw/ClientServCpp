#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 500
#define PORT "6160"
#define BACKLOG 0

void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

void handle(int newfd){
    int nread;
    char rec[BUF_SIZE], mesg[BUF_SIZE] = "Hello, World!";

    while(strcmp(rec, "Q") != 0){
        memset(&rec, 0, sizeof(char) * BUF_SIZE);
        if( (nread = recv(newfd, rec, sizeof(rec) - 1, 0)) == -1 ){
            fprintf(stderr, "Could not receive from client.\n");
        }else if(strlen(rec) != 0){
            rec[nread] = '\0';
            printf("[M]\t'%s'\n", rec);
        }

        if(send(newfd, mesg, sizeof(mesg), 0) == -1){
            fprintf(stderr, "Could not send to client.\n");
        }else{
            printf("%s", mesg);
        }
    }

    /*
    if( (nread = recv(newfd, rec, sizeof(rec) - 1, 0)) == -1 ){
        fprintf(stderr, "Could not receive from client (Message too large).\n");
    }

    rec[nread] = '\0';
    printf("[M]\t'%s'\n", rec);
    */

   //memset(mesg, 0, sizeof(char) * BUF_SIZE);
}


int main(int argc, char **argv){
    char buf[BUF_SIZE], dbg[INET6_ADDRSTRLEN];
    struct addrinfo hints, *result, *rp;
    struct sockaddr_storage peer_addr;
    int sockfd, newfd, s;
    socklen_t peer_addrlen;
    
    memset(&buf, 0, sizeof(char) * BUF_SIZE);
    memset(&dbg, 0, sizeof(char) * INET6_ADDRSTRLEN);
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC; /* Allow IPV4 and IPV6 connections */
    hints.ai_socktype = SOCK_STREAM; /* TCP Socket */
    hints.ai_flags = AI_PASSIVE; /* Any wildcard IP address */
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    if((s = getaddrinfo(NULL, PORT, &hints, &result)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    /*
        getaddrinfo() returns a list of address structures.
        Try each address until we successfully bind(2).
        If socket(2) or bind(2) fails, we close the socket
        and try the next address. 
    */

    for(rp = result; rp != NULL; rp = rp->ai_next){
        if((sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1){
            fprintf(stderr, "Could not create socket.\n");
            continue;
        }

        if(bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0){
            break; /* Success */
        }

        close(sockfd);
    }

    freeaddrinfo(result); /* No longer needed */

    if(rp == NULL){
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }

    if(listen(sockfd, BACKLOG) == -1){
        fprintf(stderr, "Failed to listen\n");
        exit(EXIT_FAILURE);
    }





    /************************
    * Read and write stage. *
    ************************/

    while(1){
        peer_addrlen = sizeof(peer_addr);
        printf("[I]\tWaiting for connections...\n");
        newfd = accept(sockfd, (struct sockaddr *) &peer_addr, &peer_addrlen);
        if(newfd == -1){
            fprintf(stderr, "accept\n");
            //continue;
        }

        inet_ntop(peer_addr.ss_family, get_in_addr((struct sockaddr *) &peer_addr), dbg, sizeof(dbg));
        printf("[I]\tNew connection from %s\n", dbg);

        
        pid_t pid = fork();
        //Child process starts
        if(pid < 0){
            fprintf(stderr, "Could not accept new client.\n");
        }else if(pid == 0){
            close(sockfd);
            handle(newfd);
            exit(0);
        }else{
            close(newfd);
        }
    }

    close(sockfd);

    return 0;

}