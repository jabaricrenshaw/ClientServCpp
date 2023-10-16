#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <thread>
#include <vector>

#define BUF_SIZE 500
#define PORT "6160"
#define BACKLOG 0

void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET) return &(((struct sockaddr_in *) sa)->sin_addr);
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

void handle(int newfd, char *cliname){
    int nread;
    char *rec = new char[BUF_SIZE + 1], *mesg = new char[BUF_SIZE + 1];
    strcpy(mesg, "Hello, World!");

    while(1){
        memset(rec, 0, BUF_SIZE);
        if( (nread = recv(newfd, rec, BUF_SIZE, 0)) == -1 ){
            fprintf(stderr, "Could not receive from client.\n");
        }else if(strlen(rec) > 0){
            if(strcmp(rec, "!Disconnect") == 0){
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

void f_read(int newfd){
    int nread;
    char *rec = new char[BUF_SIZE + 1];

    while(1){
        memset(rec, 0, BUF_SIZE);
        if( (nread = recv(newfd, rec, BUF_SIZE, 0)) == -1 ){
            fprintf(stderr, "[E]\tCould not receive from client.\n");
        }else if(strlen(rec) > 0){
            if(strcmp(rec, "!Disconnect") == 0){
                printf("[D]\tClient disconnected!\n");
                break;
            }
            printf("[M]\t'%s'\n", rec);
        }
    }
}

void f_write(int newfd){
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
        if(send(newfd, mesg, strlen(mesg), 0) == -1){
            fprintf(stderr, "[E]\tCould not send message.\n");
        }
    }
}

struct client{
    std::thread t;
    char *name;
};

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
    
    std::vector<struct client> conn;

    /*
    Want to create a "global" write thread that writes to all connected clients.
    */
    std::thread t_write;

    while(1){
        printf("[I]\tWaiting for connections...\n");

        peer_addrlen = sizeof(peer_addr);
        newfd = accept(sockfd, (struct sockaddr *) &peer_addr, &peer_addrlen);
        if(newfd == -1){
            fprintf(stderr, "[E]\tCould not accept client.\n");
            break;
        }

        inet_ntop(peer_addr.ss_family, get_in_addr((struct sockaddr *) &peer_addr), dbg, INET6_ADDRSTRLEN);
        printf("[I]\tNew connection from %s\n", dbg);

        /*
        Only erases clients who have left using !Disconnect
        */
        for(auto i = 0; i < conn.size(); ++i){
            if(conn[i].t.joinable()){
                conn[i].t.join();
                conn.erase(conn.begin() + i);
            }
        }
        conn.emplace_back(client{ std::thread(f_read, newfd), dbg });
        /*
        Remove clients who are no longer connected.
        Maybe polling using SYN/ACK will work.
        DONE: Remove certain client from vector on explicit !Disconnect
        Data is not now set up to get this done
        Silly client struct...
        */
        printf("[I]\tNumber of connections (threads): %lu\n", conn.size());        
    }

    for(auto &x : conn){
        x.t.join();
    }
    t_write.join();

    close(sockfd);

    delete[] dbg;
    free(result);
    free(rp);

    return 0;

}