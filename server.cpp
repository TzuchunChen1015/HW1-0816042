#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/time.h>
#include <sys/types.>
#include <unistd.h>
using namespace std;
string RecvMsg(int);
int main(int argc, char** argv) {
	// Set Server IP and Port	
	const string IP = argv[1];
	sockaddr_in serverAddr;
	bzero(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(atoi(IP.c_str()));
	// Build TCP
	int tcpFd = socket(AF_INET, SOCK_STREAM, 0);	// fd = 3
	if(tcpFd == -1 ) {
		cerr << "Failed to Construct TCP Socket\n";
		exit(0);
	}
	bind(tcpFd, (const struct sockaddr*) &serverAddr, sizeof(serverAddr));
	listen(tcpFd, SOMAXCONN);
	// Build UDP
	int udpFd = socket(AF_INET, SOCK_DGRAM, 0);		// fd = 4
	if(udpFd == -1 ) {
		cerr << "Failed to Construct UDP Socket\n";
		exit(0);
	}	
	bind(udpFd, (const struct sockaddr*) &serverAddr, sizeof(serverAddr));
	// Use Select
	FD_SET rSet, allSet;
	
	return 0;
}
string RecvMsg(int fd) {
	string msg; char BUF[2]; int nBytes;
	BUF[1] = '\0';
	while((nBytes = recv(fd, BUF, 1, 0)) > 0) {
		if(BUF[0] == '\n') break;
		msg += BUF[0];
	}
	return msg;
}
