#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <thread>
#include <unistd.h>

#define ADDRESS "127.0.0.1"
#define PORT "6160"
#define BUF_SIZE 500

/*  This is a flag to tell the read thread (working in f_read) to stop
*   when the client is finished writing and would like to disconnect.
*/
int THREADQ = 1; 

void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void f_read(int sockfd){
    char *buf = (char *)calloc(BUF_SIZE, sizeof(char));
    int nread;
    while(1 && THREADQ){
        if((nread = recv(sockfd, buf, BUF_SIZE - 1, 0)) == -1){
            fprintf(stderr, "Could not receive message.\n");
        }else if(strlen(buf) != 0){
            buf[nread] = '\0';
            printf("[M]\t'%s'\n", buf);
            memset(buf, 0, sizeof(buf));
        }
    }
}

void f_write(int sockfd){
    char mesg[BUF_SIZE];
    while(strcmp(mesg, "Q") != 0){
        memset(mesg, 0, sizeof(mesg));
        printf("Enter your message: ");
        scanf(" %[^\n]", mesg);

        if(send(sockfd, mesg, sizeof(mesg), 0) == -1){
            fprintf(stderr, "Could not send message.\n");
        }
    }

    THREADQ = 0;
}

int main(int argc, char **argv){
    int sockfd, s;
    size_t len;
    struct addrinfo hints, *result, *rp;

    /*
    if(argc != 2){
        fprintf(stderr, "Usage is %s {HOSTNAME}.\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    */

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    if((s = getaddrinfo(ADDRESS, PORT, &hints, &result)) != 0){
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



    /****************************
    *   Read and write stage.   *
    *****************************/
    /*  I figured creating threads would allow
    *   the client to read/write at the same time.
    *
    *   It works, 
    *   I am not confident this is thread safe and such...     
    */

    std::thread t_read(f_read, sockfd);
    std::thread t_write(f_write, sockfd);
    t_read.join();
    t_write.join();

    printf("Disconnecting!\n");
    close(sockfd);
    
    return 0;
}