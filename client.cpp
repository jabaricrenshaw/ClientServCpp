#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "6160"
#define BUF_SIZE 500

void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char **argv){
    int sockfd, s, nread;
    char buf[BUF_SIZE], mesg[BUF_SIZE] = "I am the king.";;
    size_t len;
    struct addrinfo hints, *result, *rp;

    if(argc != 2){
        fprintf(stderr, "Usage is %s {HOSTNAME}.\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&buf, 0, sizeof(char) * BUF_SIZE);
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    if((s = getaddrinfo(argv[1], PORT, &hints, &result)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    for(rp = result; rp != NULL; rp = rp->ai_next){
        if((sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1){
            continue;
        }

        if(connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1){
            break;
        }

        close(sockfd);
    }

    freeaddrinfo(result);

    if(rp == NULL){
        fprintf(stderr, "Could not connect.\n");
        exit(EXIT_FAILURE);
    }



    /************************
    * Read and write stage. *
    ************************/

    if((nread = recv(sockfd, buf, BUF_SIZE - 1, 0)) == -1){
        fprintf(stderr, "Could not receive message (Too large).\n");
    }

    if(send(sockfd, mesg, sizeof(mesg), 0) == -1){
        fprintf(stderr, "Could not send message.\n");
    }

    buf[nread] = '\0';
    printf("[M]\t'%s'\n", buf);

    close(sockfd);

    return 0;
}