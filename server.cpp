#include <iostream>
#include <netdb.h>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <thread>
#include <unordered_map>
#include <random>
#include <climits>

#ifndef BUF_SIZE
#define BUF_SIZE 4096
#endif
#ifndef PORT
#define PORT "6160"
#endif
#ifndef BACKLOG
#define BACKLOG 0
#endif

enum msg_mode { ALL, WHISPER };

struct client {
    std::thread t;
    uintptr_t fd;
    uintptr_t id;
    bool present;
    std::string nickname;
    msg_mode m_mode;
};

void * get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET) return &(((struct sockaddr_in *) sa)->sin_addr);
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

void f_read(int newfd, std::unordered_map<int, struct client> &conn, int cli_id){
    int nread;
    std::string rec;
    char *buf = new char[BUF_SIZE]; 

    while(1){
        rec = "";
        memset(buf, 0, BUF_SIZE);

        if((nread = recv(newfd, buf, BUF_SIZE, 0)) != -1 && strlen(buf) > 0){
            rec = buf;

            if(rec == "!disconnect"){
                if(send(newfd, "!disconnect", 11, 0) == -1)
                    std::cerr << "[E]\tClient failed on disconnect. " << cli_id << " (" << conn[cli_id].nickname << ")." << std::endl;

                conn[cli_id].present = false;

                std::cout << "[D]\t Client " << cli_id << " (" << conn[cli_id].nickname << ") disconnected." << std::endl;
                break;
            }

            if(rec == "!setname"){
                memset(buf, 0, BUF_SIZE);
                if(recv(newfd, buf, BUF_SIZE, 0) != -1){
                    rec = buf;
                    conn[cli_id].nickname = rec;
                    continue;
                }
            }

            std::cout << "[M " << (conn[cli_id].nickname == "" ? std::to_string(cli_id) : conn[cli_id].nickname) << "]\t";
            std::cout << "'" << rec << "'" << std::endl;


        }else if(nread == -1){
            std::cerr << "[E]\tCould not read message from client. " << cli_id << " (" << conn[cli_id].nickname << ")." << std::endl;
        }
    }

    delete[] buf;
    return;
}

void f_write(std::unordered_map<int, struct client> &conn){
    std::string msg;

    while(1){
        msg = "";

        std::cout << "Enter your message: ";
        std::getline(std::cin, msg);

        for(auto itr = conn.begin(); itr != conn.end(); itr++){
            if(send(itr->second.fd, msg.c_str(), msg.size(), 0) == -1)
                std::cerr << "[E] " << itr->second.nickname << " could not receive message" << std::endl;
        }
    }

    return;
}

int main(int argc, char **argv){
    std::string cli_addr;
    char *buf = new char[BUF_SIZE];
    struct addrinfo hints, *result, *rp;
    struct sockaddr_storage peer_addr;
    socklen_t peer_addrlen;
    int sockfd, newfd, s;

    std::unordered_map<int, struct client> conn;
    uint16_t cli_id;
    std::thread t_write;

    srand(time(NULL));

    hints.ai_family = AF_UNSPEC; /* Allow IPV4 and IPV6 connections */
    hints.ai_socktype = SOCK_STREAM; /* TCP Socket */
    hints.ai_flags = AI_PASSIVE; /* Any wildcard IP address */
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    if((s = getaddrinfo(NULL, PORT, &hints, &result)) != 0){
        std::cerr << "[E]\tgetaddrinfo: " << gai_strerror(s) << std::endl;
        exit(EXIT_FAILURE);
    }

    for(rp = result; rp != NULL; rp = rp->ai_next){
        if((sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1){
            std::cerr << "[E]\tCould not create socket." << std::endl;
            continue;
        }

        if(bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;

        close(sockfd);
    }

    freeaddrinfo(result);

    if(listen(sockfd, BACKLOG) == -1){
        std::cerr << "[E]\tFailed to listen." << std::endl;
        exit(EXIT_FAILURE);
    }

    while(1){
        std::cout << "[I]\tWaiting for connections..." << std::endl;

        peer_addrlen = sizeof(peer_addr);
        newfd = accept(sockfd, (struct sockaddr *) &peer_addr, &peer_addrlen);
        if(newfd == -1){
            std::cerr << "[E]\tCould not accept client" << std::endl;
            continue;
        }

        inet_ntop(peer_addr.ss_family,  get_in_addr((struct sockaddr *) &peer_addr), buf, INET6_ADDRSTRLEN);
        cli_addr = buf;
        memset(buf, 0, BUF_SIZE);
        std::cout << "[I]\tNew connection from " << cli_addr << std::endl;

        std::cout << "[I]\tNumber of connections (threads): " << conn.size() + 1 << std::endl;

        if(!(t_write.joinable()))
            t_write = std::thread(f_write, std::ref(conn));

        for(auto itr = conn.begin(); itr != conn.end(); ){
            if(itr->second.present == false && itr->second.t.joinable()){
                itr->second.t.join();
                itr = conn.erase(itr);
                continue;
            }
            itr++;
        }

        cli_id = (rand() % UINT16_MAX + 100);
        conn[cli_id] = client {
            std::thread(f_read, newfd, std::ref(conn), cli_id),
            (uintptr_t)newfd,
            cli_id,
            1,
            cli_addr,
            ALL
        };
    }

    for(auto itr = conn.begin(); itr != conn.end(); ++itr){
        if(itr->second.t.joinable()) itr->second.t.join();
        if(send(itr->second.fd, "!disconnect", 11, 0) == -1)
            std::cerr << "[E]\tClient " << itr->second.nickname << " did not disconnect clean!\n" << std::endl;
        close(itr->second.fd);
    }

    if(t_write.joinable())
        t_write.join();

    close(sockfd);
    free(buf);
}