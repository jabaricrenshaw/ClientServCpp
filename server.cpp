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
#include <random>
#include <climits>

#define BUF_SIZE 512
#define PORT "6160"
#define BACKLOG 0

struct client{
    std::thread t;
    uintptr_t fd;
    uint16_t id;
    char *nickname;
    char *msg_mode;
};

void *get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET) return &(((struct sockaddr_in *) sa)->sin_addr);
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

void f_read(int newfd, std::map<int, struct client> &conn, int cli_id){
    int nread;
    char *rec = new char[BUF_SIZE];

    while(1){
        memset(rec, 0, BUF_SIZE);

        if( (nread = recv(newfd, rec, BUF_SIZE, 0)) != -1 && strlen(rec) > 0){
            //Need lookup table or switch case this somehow            
            //Disconnect
            if(strcmp(rec, "!disconnect") == 0){
                if(send(newfd, "!disconnect", 11, 0) == -1)
                    fprintf(stderr, "[E]\tClient failed on disconnect.\n");
                
                conn[cli_id].id = 0;
                
                printf("[D]\tClient disconnected!\n");
                break;
            }

            //Set name
            if(strcmp(rec, "!setname") == 0){
                memset(rec, 0, BUF_SIZE);
                recv(newfd, rec, BUF_SIZE, 0);
                strcpy(conn[cli_id].nickname, rec);
                continue;
            }

            //Show message
            if(strlen(conn[cli_id].nickname) == 0){
                printf("[M %d]\t'%s'\n", cli_id, rec);
            }else{
                printf("[M %s]\t'%s'\n", conn[cli_id].nickname, rec);
            }
        }else if(nread == -1){
            printf("[E]\tCould not read from client.\n");
        }
    }

    delete[] rec;
    return;
}

void f_write(std::map<int, struct client> &conn){
    char *msg = new char[BUF_SIZE];

    while(1){
        memset(msg, 0, BUF_SIZE);

        printf("Enter your message: ");
        scanf(" %[^\n]", msg);

        //Sends message to all connected clients
        for(std::map<int, struct client>::iterator it = conn.begin(); it != conn.end(); ++it){
            if(send(it->second.fd, msg, strlen(msg), 0) == -1)
                fprintf(stderr, "[E]\tCould not send message(s).\n");
        }
    }

    delete[] msg;
    return;
}

int main(int argc, char **argv){
    char *cli_addr = new char[BUF_SIZE];
    struct addrinfo hints, *result, *rp;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addrlen;
    int sockfd, newfd, s;

    //conn holds info on connected clients for help in threads
    std::map<int, struct client> conn;
    //cli_id is the future id of clients in conn
    uint16_t cli_id;
    //"global" write thread that writes to all connected clients.
    //Read threads are created individually for each client that connects
    std::thread t_write;
    
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
    while(1){
        printf("[I]\tWaiting for connections...\n");

        peer_addrlen = sizeof(peer_addr);
        newfd = accept(sockfd, (struct sockaddr *) &peer_addr, &peer_addrlen);
        if(newfd == -1){
            fprintf(stderr, "[E]\tCould not accept client.\n");
            continue;
        }

        inet_ntop(peer_addr.ss_family, get_in_addr((struct sockaddr *) &peer_addr), cli_addr, INET6_ADDRSTRLEN);
        printf("[I]\tNew connection from %s\n", cli_addr);

        
        if(!t_write.joinable()){
            t_write = std::thread(f_write, std::ref(conn));
        }
        
        for(std::map<int, struct client>::iterator it = conn.begin(); it != conn.end(); ){
            if(it->second.id == 0 && it->second.t.joinable()){
                it->second.t.join();
                it = conn.erase(it);
                continue;
            }

            it++;
        }

        cli_id = (rand() % UINT16_MAX + 100);
        conn[cli_id] = client{ std::thread(f_read, newfd, std::ref(conn),cli_id),
                                (uintptr_t)newfd,
                                cli_id,
                                cli_addr,
                                /* "global" */ };
        
        /*
        Remove clients who are no longer connected.
            DONE: Remove certain client from vector on explicit !disconnect
            Maybe polling using SYN/ACK idea will work.
        */
        printf("[I]\tNumber of connections (threads): %lu\n", conn.size());        
    }

    //Closing cleanup
    for(std::map<int, struct client>::iterator it = conn.begin(); it != conn.end(); ++it){
        if(it->second.t.joinable()) it->second.t.join();
        if(send(it->second.fd, "!disconnect", 11, 0) == -1)
            fprintf(stderr, "[E]\tClient(s) did not disconnect clean.\n");
        close(it->second.fd);
    }

    if(t_write.joinable())
        t_write.join();

    delete[] cli_addr;
    close(sockfd);
    free(result);
    free(rp);

    return 0;
}