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
#include <random>
#include <climits>

#define BUF_SIZE 500
#define PORT "6160"
#define BACKLOG 0

struct client{
    std::thread t;
    char *name;
    uint16_t id;
};

void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET) return &(((struct sockaddr_in *) sa)->sin_addr);
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

void f_read(int newfd, std::vector<struct client> &conn, int req){
    int nread;
    char *rec = new char[BUF_SIZE + 1];

    while(1){
        memset(rec, 0, BUF_SIZE);

        if( (nread = recv(newfd, rec, BUF_SIZE, 0)) != -1 && strlen(rec) > 0){
            if(strcmp(rec, "!Disconnect") == 0){
                if(send(newfd, "!Disconnect", 11, 0) == -1)
                    fprintf(stderr, "[E]\tClient failed on disconnect.\n");
                
                for(size_t i = 0; i < conn.size(); ++i)
                    if(conn[i].id == req) conn[i].id = 0;

                printf("[D]\tClient disconnected!\n");
                break;
            }
            printf("[M %d]\t'%s'\n", req, rec);
        }else if(nread == -1){
            printf("[E]\tCould not read from client.\n");
        }
    }

    delete[] rec;
    return;
}

void f_write(int newfd){
    char *mesg = new char[BUF_SIZE + 1];

    printf("Made it to write thread");

    delete[] mesg;
    return;
}

int main(int argc, char **argv){
    struct addrinfo hints, *result, *rp;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addrlen;
    int sockfd, newfd, s;
    char *dbg = new char[BUF_SIZE + 1];
    srand(time(NULL));
    
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

    /*  getaddrinfo() returns a list of address structures.
     *  Try each address until we successfully bind(2).
     *  If socket(2) or bind(2) fails, we close the socket
     *  and try the next address. 
    */

    for(rp = result; rp != NULL; rp = rp->ai_next){
        if((sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1){
            fprintf(stderr, "Could not create socket.\n");
            continue;
        }

        /* Success */
        if(bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break; 

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
    
    //Req is the future id of clients in conn
    uint16_t req;
    //vector holds info on connected clients for help in threads
    std::vector<struct client> conn;
    // Want to create a "global" write thread that writes to all connected clients.
    std::thread t_write;

    while(1){
        printf("[I]\tWaiting for connections...\n");

        peer_addrlen = sizeof(peer_addr);
        newfd = accept(sockfd, (struct sockaddr *) &peer_addr, &peer_addrlen);
        if(newfd == -1){
            fprintf(stderr, "[E]\tCould not accept client.\n");
            continue;
        }

        inet_ntop(peer_addr.ss_family, get_in_addr((struct sockaddr *) &peer_addr), dbg, INET6_ADDRSTRLEN);
        printf("[I]\tNew connection from %s\n", dbg);

        /*
        if(!t_write.joinable()){
            t_write = std::thread(f_write, newfd);
        }
        */

        for(std::vector<struct client>::iterator it = conn.begin(); it != conn.end(); ){
            if(it->id == 0 && it->t.joinable()){
                it->t.join();
                it = conn.erase(it);
            }else{
                it++;
            }
        }

        req = (rand() % UINT16_MAX + 100);
        conn.emplace_back(client{ std::thread(f_read, newfd, std::ref(conn), req), dbg, req });
        /*
        Remove clients who are no longer connected.
            DONE: Remove certain client from vector on explicit !Disconnect
            Maybe polling using SYN/ACK idea will work.
        */
        printf("[I]\tNumber of connections (threads): %lu\n", conn.size());        
    }

    for(auto &x : conn)
        if(x.t.joinable()) x.t.join();

    if(t_write.joinable())
        t_write.join();

    delete[] dbg;
    close(sockfd);
    free(result);
    free(rp);

    return 0;
}