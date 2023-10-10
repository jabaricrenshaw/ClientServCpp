#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <thread>
#include <map>

#define BUF_SIZE 500
#define PORT "6160"
#define BACKLOG 0

void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET) return &(((struct sockaddr_in *) sa)->sin_addr);
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

void handle(int newfd, char *cliname, std::map<char *, int> clifd){
    int nread;
    char *rec = new char[BUF_SIZE + 1], *mesg = new char[BUF_SIZE + 1];
    strcpy(mesg, "Hello, World!");

    while(1){
        memset(rec, 0, BUF_SIZE);
        if( (nread = recv(newfd, rec, BUF_SIZE, 0)) == -1 ){
            fprintf(stderr, "Could not receive from client.\n");
        }else if(strlen(rec) > 0){
            if(strcmp(rec, "!Disconnect") == 0){
                clifd.erase(clifd.find(cliname));
                printf("[D]\tClient disconnected!\n");
                break;
            }
            printf("[M]\t'%s'\n", rec);
        }

        if(send(newfd, mesg, strlen(mesg), 0) == -1){
            fprintf(stderr, "Could not send to client.\n");
        }
    }

    delete[] rec;
    delete[] mesg;
}

/*
void f_read(int newfd){

}
*/

void f_write(std::map<char *, int> clifd, int newfd){
    char *mesg = new char[BUF_SIZE + 1];
    
    while(1){
        memset(mesg, 0, BUF_SIZE);
        printf("Enter your message: ");
        scanf(" %[^\n]", mesg);

        printf("Loop starting... Client size = %ld\n", clifd.size());
        for(auto it = clifd.begin(); it != clifd.end(); ++it){
            printf("CLIFD DEBUG: %s\t%i\n", it->first, it->second);
            if(send(it->second, mesg, strlen(mesg), 0) == -1){
                fprintf(stderr, "[E]\tCould not send message.\n");
            }
        }
    }

    delete[] mesg;
}


int main(int argc, char **argv){
    struct addrinfo hints, *result, *rp;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addrlen;
    int sockfd, newfd, s;
    char *dbg = new char[BUF_SIZE + 1];
    
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

        /* Success */
        if(bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break; 

        close(sockfd);
    }

    freeaddrinfo(result); /* No longer needed */

    if(listen(sockfd, BACKLOG) == -1){
        fprintf(stderr, "Failed to listen\n");
        exit(EXIT_FAILURE);
    }

    /************************
    * Read and write stage. *
    ************************/

    std::map<char*, int> clifd;
    std::thread t_write;
    while(1){
        peer_addrlen = sizeof(peer_addr);

        printf("[I]\tWaiting for connections...\n");
        newfd = accept(sockfd, (struct sockaddr *) &peer_addr, &peer_addrlen);
        if(newfd == -1){
            fprintf(stderr, "[E]\tCould not accept client.\n");
            break;
        }

        inet_ntop(peer_addr.ss_family, get_in_addr((struct sockaddr *) &peer_addr), dbg, INET6_ADDRSTRLEN);
        printf("[I]\tNew connection from %s\n", dbg);
        
        pid_t pid = fork();
        //Child process starts
        if(pid < 0){
            fprintf(stderr, "Could not accept new client.\n");
        }else if(pid == 0){
            close(sockfd);
            handle(newfd, dbg, clifd);
            exit(0);
        }else{
            close(newfd);
        }

        /*
        This does not work after forking because we use close(sockfd).
        Figure out how to close sockfd properly so that we may us the thread
        to write to clients.

        Also, does when does newfd need to be closed? 


        clifd.emplace(dbg, newfd);
        if(!t_write.joinable()){
            t_write = std::thread{ f_write, clifd, newfd };
        }
        */
        
    }

    /*
    If we create threads to read and write at the same time
    like we did for the client. Then the parent will continue
    execution in the loop to accept new clients.

    Will the threads be overwritten here? Should we keep track
    of which thread pair are assigned to each client that connnects?

    I would only like for the server to give information that the client
    requests, or perform "sudo-like" operations to disconnect all,
    show who is connected, and globally announce... More on this in the future.

    Fa later...

    std::thread t_read(f_read, newfd);
    std::thread t_write(f_write, newfd);
    t_read.join();
    t_write.join();
    */

    t_write.join();

    close(sockfd);

    delete[] dbg;
    free(result);
    free(rp);

    return 0;

}