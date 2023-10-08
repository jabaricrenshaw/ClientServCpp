#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <thread>
#include <atomic>
#include <unistd.h>

#define ADDRESS "127.0.0.1"
#define PORT "6160"
#define BUF_SIZE 500

/*  This is a flag to tell the read thread (working in f_read) to stop
*   when the client is finished writing and would like to disconnect.
*/
int THREADQ = 1;

void f_read(int sockfd, std::atomic<bool> complete){
    char *buf = new char[BUF_SIZE + 1];
    int nread;

    while(1){
        if(complete.load()){
            break;
        }

        if((nread = recv(sockfd, buf, BUF_SIZE, 0)) == -1){
            fprintf(stderr, "Could not receive message.\n");
        }else if(strlen(buf) != 0){
            printf("[M]\t'%s'\n", buf);
            memset(buf, 0, BUF_SIZE);
        }
    }

    delete[] buf;
    return;
}

void f_write(int sockfd, std::atomic<bool> complete){
    char *mesg = new char[BUF_SIZE + 1];

    while(1){
        memset(mesg, 0, BUF_SIZE);
        printf("Enter your message: ");
        scanf(" %[^\n]", mesg);
        
        /*
        *   If connection is lost with the server, the
        *   client will only realize after the second
        *   message is not delivered...
        */
        if(send(sockfd, mesg, strlen(mesg), 0) == -1){
            fprintf(stderr, "[E]\tCould not send message.\n");
        }

        if(strcmp(mesg, "!Disconnect") == 0){
            printf("[D]\tDisconnecting!.\n");
            complete.store(true);
            break;
        }
    }
    
    delete[] mesg;
    THREADQ = 0;
    return;
}

int main(int argc, char **argv){
    int sockfd, s;
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

        if(connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) break;

        close(sockfd);
    }

    freeaddrinfo(result);

    if(rp == NULL){
        fprintf(stderr, "Could not connect.\n");
        exit(EXIT_FAILURE);
    }

    printf("Connected to %s on port %s.\n", ADDRESS, PORT);
    /****************************
    *   Read and write stage.   *
    *****************************/
    /*  I figured creating threads would allow
    *   the client to read/write at the same time.
    *
    *   It works, 
    *   I am not confident this is thread safe and such...     
    */

    std::atomic<bool> complete = false;
    std::thread t_read(f_read, sockfd, &complete);
    std::thread t_write(f_write, sockfd, &complete);
    t_read.join();
    t_write.join();

    close(sockfd);
    
    return 0;
}