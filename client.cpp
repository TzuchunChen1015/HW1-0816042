#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
using namespace std;
const int MXL = 1e3;
string RecvMsg(int);
void SendMsg(int, string);
int main(int argc, char** argv) {
	
	const string IP = argv[1], PORT = argv[2];
	sockaddr_in serverAddr;
	bzero(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(IP.c_str());
	serverAddr.sin_port = htons(atoi(PORT.c_str()));

	int tcpFd = socket(AF_INET, SOCK_STREAM, 0);
	if(tcpFd == -1) {
		cerr << "Failed to Construct TCP Socket\n";
		exit(0);
	}
	connect(tcpFd, (const struct sockaddr*) &serverAddr, sizeof(serverAddr));
	cout << RecvMsg(tcpFd);

	int udpFd = socket(AF_INET, SOCK_DGRAM, 0);
	if(udpFd == -1) {
		cerr << "Failed to Construct UDP Socket\n";
		exit(0);
	}

	string msg;
	while(getline(cin, msg)) {
		msg += '\n';
		// Use UDP to Send Message
		if(msg[0] == 'r' || msg[0] == 'g') {
			char BUF[MXL + 1];
			strcpy(BUF, msg.c_str()); BUF[msg.length()] = '\0';
			sendto(udpFd, BUF, strlen(BUF), 0, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
			bzero(BUF, sizeof(BUF));
			socklen_t len;
			recvfrom(udpFd, BUF, sizeof(BUF), 0, (struct sockaddr*) &serverAddr, &len);
			string msg = BUF;
			cout << msg;
		}
		// Use TCP to Send Message
		else {
			SendMsg(tcpFd, msg);
			if(msg == "exit\n") break;
			cout << RecvMsg(tcpFd);
		}
	}
	close(tcpFd), close(udpFd);
	return 0;
}
string RecvMsg(int fd) {
	string msg; char BUF[2]; int nBytes;
	BUF[1] = '\0';
	while((nBytes = recv(fd, BUF, 1, 0)) > 0) {
		if(BUF[0] == '\0') break;
		msg += BUF[0];
	}
	return msg;
}
void SendMsg(int fd, string s) {
	char BUF[MXL + 1];
	strcpy(BUF, s.c_str()); BUF[s.length()] = '\0';
	int leftBytes = s.length(), now = 0, nBytes;
	while(leftBytes) {
		nBytes = send(fd, BUF + now, leftBytes, 0);
		leftBytes -= nBytes;
		now += nBytes;
	}
}
