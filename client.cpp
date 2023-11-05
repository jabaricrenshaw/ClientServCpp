#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <thread>
#include <limits>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#ifndef ADDRESS
#define ADDRESS "127.0.0.1"
#endif
#ifndef PORT
#define PORT "6160"
#endif
#ifndef BUF_SIZE
#define BUF_SIZE 4096
#endif

class cli_info {
private:
	std::string nickname;

public:
	cli_info(){
		this->new_nickname();
	}

	std::string get_nickname(){
		return this->nickname;
	}

	void new_nickname(){
		do{
			std::cout << "Nickname?: ";
			std::getline(std::cin, this->nickname);
		}while(this->nickname.length() < 3 || this->nickname.length() > 16);
	}

	~cli_info(){ }
};

void f_read(int sockfd){
	std::string rec;
	char *buf = new char[BUF_SIZE];
	int nread;

	/*
    *  	recv(...) blocks inside of this thread, waiting for
    *	message from server.
    */

	while(1){
		memset(buf, 0, BUF_SIZE);
		rec = "";

		if((nread = recv(sockfd, buf, BUF_SIZE, 0)) != -1 && nread > 0){
			rec = buf;
			if(rec == "!disconnect")
				break;

			std::cout << "[M]\t'" << rec << "'" << std::endl;
		}else if(nread == -1){
			std::cerr << "Could not receive message." << std::endl;
		}
	}
	
	delete[] buf;
	return;
}

void f_write(int sockfd, cli_info *client_info){
	std::string msg;
	std::string temp;
	//char *buf = new char[BUF_SIZE];

	if(client_info->get_nickname().length() == 0)
		client_info->new_nickname();

	if(send(sockfd, "!setname", 8, 0) == -1)
		std::cerr << "[E]\tCould not set nickname!\n" << std::endl;

	usleep(10000);

	//strcpy(buf, client_info->get_nickname().c_str());
	temp = client_info->get_nickname().c_str();
	if(send(sockfd, temp.c_str(), temp.size(), 0) == -1)
		std::cerr << "[E]\tCould not set nickname!\n" << std::endl;
	
	//std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	while(1){
		temp = "";
		msg = "";
		//memset(buf, 0, BUF_SIZE);	
		
		std::cout << "Enter your message: ";
		std::getline(std::cin, msg);

		/*
        *   If connection is lost with the server, the
        *   client will only realize after the second
        *   message is not delivered...
        */

		//strcpy(buf, msg.substr(0, BUF_SIZE).c_str());
		if(send(sockfd, msg.c_str(), msg.size(), 0) == -1)
			std::cerr << "[E]\tCould not send message!" << std::endl;

		if(msg == "!disconnect"){
			std::cerr << "[I]\tDisconnecting!" << std::endl;
			break;
		}

		if(msg == "!setname"){
			client_info->new_nickname();
			temp = client_info->get_nickname();
			//strcpy(buf, client_info->get_nickname().c_str());
			if(send(sockfd, temp.c_str(), temp.size(), 0) == -1)
				std::cerr << "[E]\tCould not set nickname!" << std::endl;
		}
	}
	
	return;
}

int main(){
	int sockfd, s;
	struct addrinfo hints, *result, *rp;
	cli_info *client_info;

	std::ios::sync_with_stdio(false);
	//cin.tie(NULL);

	/*
	 *	Need for defining IP address of server later (possibly port as well)
	 *	if(argc != 2){
	 *		std::cerr << "Usage is " << argv[0] << " {HOSTNAME}" << std::endl;
	 *		exit(EXIT_FAILURE);
	 *	}
	*/
	
	client_info = new cli_info();

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

	if((s = getaddrinfo(ADDRESS, PORT, &hints, &result)) != 0){
		std::cerr << "getaddrcli_info: " << gai_strerror(s) << std::endl;
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
		std::cerr << "Could not connect to host." << std::endl;
		exit(EXIT_FAILURE);
	}

	std::cout << "Connected to " << ADDRESS << " on port " << PORT << std::endl;

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
