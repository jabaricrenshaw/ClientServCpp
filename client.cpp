#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <thread>
#include <unistd.h>

//Set ADDRESS to the server's public IP address
#define ADDRESS "127.0.0.1"
#define PORT "6160"
#define BUF_SIZE 512

class info{
private:
    char *nickname = new char[16];

public:
    char *get_nickname(){
        return this->nickname;
    }

    void new_nickname(){
        bool namecheck;

        do{
            printf("Nickname?: ");
            scanf(" %[^\n]", this->nickname);
            if((namecheck = strlen(this->nickname) < 3 || strlen(this->nickname) > 16))
                printf("\nNickname must be 3-16 characters.\n");
        }while(namecheck);
    }

    ~info(){
        delete[] this->nickname;
    }
};

void f_read(int sockfd){
    char *rec = new char[BUF_SIZE];
    int nread;

    /*
    Note: recv(...) blocks inside of this thread, waiting for
    message from server.
    */
    while(1){
        memset(rec, 0, BUF_SIZE);

        if((nread = recv(sockfd, rec, BUF_SIZE, 0)) != -1 && strlen(rec) > 0){
            if(strcmp(rec, "!disconnect") == 0)
                break;
            printf("[M]\t'%s'\n", rec);
        }else if(nread == -1){
            fprintf(stderr, "Could not receive message.\n");
        }

    }

    delete[] rec;
    return;
}

void f_write(int sockfd, info *client_info){
    char *msg = new char[BUF_SIZE];

    if(strlen(client_info->get_nickname()) != 0){
        if(send(sockfd, "!setname", 8, 0) == -1)
            fprintf(stderr, "[E]\tCould not set nickname!\n");

        usleep(10000);

        if(send(sockfd, client_info->get_nickname(), strlen(client_info->get_nickname()), 0) == -1)
            fprintf(stderr, "[E]\tCould not set nickname!\n");
    }

    while(1){
        memset(msg, 0, BUF_SIZE);
        printf("Enter your message: ");
        scanf(" %[^\n]", msg);
        
        /*
        *   If connection is lost with the server, the
        *   client will only realize after the second
        *   message is not delivered...
        */
        if(send(sockfd, msg, strlen(msg), 0) == -1)
            fprintf(stderr, "[E]\tCould not send message.\n");
        
        if(strcmp(msg, "!disconnect") == 0){
            printf("[D]\tDisconnecting!\n");
            break;
        }

        if(strcmp(msg, "!setname") == 0){
            client_info->new_nickname();
            if(send(sockfd, client_info->get_nickname(), strlen(client_info->get_nickname()), 0) == -1)
                fprintf(stderr, "[E]\tCould not set nickname!\n");
        }
    }
    
    delete[] msg;
    return;
}

int main(int argc, char **argv){
    int sockfd, s;
    struct addrinfo hints, *result, *rp;
    info *client_info = new info();

    /*
    Need for defining IP address of server later
    if(argc != 2){
        fprintf(stderr, "Usage is %s {HOSTNAME}.\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    */

    client_info->new_nickname();

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
        if((sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
            continue;

        if(connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;

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

    std::thread t_read(f_read, sockfd);
    std::thread t_write(f_write, sockfd, std::ref(client_info));
    t_read.join();
    t_write.join();

    close(sockfd);
    
    return 0;
}