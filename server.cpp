#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT            "6160"
#define BACKLOG         0

void sigchld_handler(int s){
    //waitpid() might overwrite errno, so we save and restore it
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void){
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage cli_addr;       //Client information
    char s[INET6_ADDRSTRLEN];
    struct sigaction sa;
    int sock_fd, new_fd;                    //Listen on sock_fd, New connections on new_fd
    socklen_t sin_size;
    int yes = 1;
    int rv;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;            //Use IP of machine

    if((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0){
        fprintf(stderr, "[E]\tgetaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next){
        if((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            perror("[E]\tCould not create socket.\n");
            continue;
        }

        if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
            close(sock_fd);
            perror("setsockopt");
            exit(1);
        }

        if(bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1){
            close(sock_fd);
            perror("[E]\tFailed to bind.\n");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);                 //Finished with this structure

    if(p == NULL){
        fprintf(stderr, "[E]\tFailed to bind.\n");
    }

    if(listen(sock_fd, BACKLOG) == -1){
        perror("[E]\tFailed to listen.\n");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;        //Kill dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa, NULL) == -1){
        perror("[E]\tsigaction\n");
        exit(1);
    }

    printf("[I]\tWaiting for connections...\n");

    while(1){
        sin_size = sizeof(cli_addr);
        new_fd = accept(sock_fd, (struct sockaddr *) &cli_addr, &sin_size);
        if(new_fd == -1){
            perror("accept");
            continue;
        }

        inet_ntop(cli_addr.ss_family, get_in_addr((struct sockaddr *) &cli_addr), s, sizeof(s));
        printf("[I]\tNew connection from %s\n", s);

        if(!fork()){
            close(sock_fd);
            if(send(new_fd, "Hello, world!", 13, 0) == -1){
                perror("send");
            }
            close(new_fd);
            exit(0);
        }

        close(new_fd);
    }

}