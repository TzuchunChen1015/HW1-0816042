#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/time.h>
#include <unistd.h>
#include <set>
#include <vector>
using namespace std;
const int MXL = 1e3;
string RecvMsg(int);
void SendMsg(int, string, bool);
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
	fd_set rSet, allSet; int mxFd = max(tcpFd, udpFd) + 1;
	FD_SET(tcpFd, &allSet);
	FD_SET(udpFd, &allSet);
	set<int> clientSet; set<int>::iterator it; vector<int> rmSet;
	while(true) {
		rSet = allSet;
		select(mxFd, &rSet, NULL, NULL, NULL);
		if(FD_ISSET(tcpFd, &rSet)) {
			int clientFd = accept(tcpFd, NULL, NULL);
			FD_SET(clientFd, &allSet);
			mxFd = max(mxFd, clientFd + 1);
			clientSet.insert(clientFd);
			SendMsg(clientFd, "*****Welcome to Game 1A2B*****\n", 0);
			SendMsg(clientFd, "% ", 1);
		}
		if(FD_ISSET(udpFd, &rSet)) {
			
		} 
		rmSet.clear();
		for(it = clientSet.begin(); it != clientSet.end(); it++) {
			int clientFd = *it;
			if(FD_ISSET(clientFd, &rSet)) {
				string msg = RecvMsg(clientFd);
				if(msg == "") rmSet.push_back(clientFd);
				else {
					SendMsg(clientFd, "Your Sent " + msg + " to the Server\n", 0);
					SendMsg(clientFd, "% ", 1);
				}
			}
		}
		// Remove Closed Clients
		for(int i = 0; i < (int) rmSet.size(); i++) {
			FD_CLR(rmSet[i], &allSet);
			clientSet.erase(rmSet[i]);
			close(rmSet[i]);
			cout << "Number " << rmSet[i] << " disconnected\n";
		}	
	}
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
void SendMsg(int fd, string s, bool lastMsg) {
	char BUF[MXL + 1];
	strcpy(BUF, s.c_str()); BUF[s.length()] = '\0';
	int leftBytes = s.length() + lastMsg, now = 0, nBytes;
	while(leftBytes) {
		nBytes = send(fd, BUF + now, leftBytes, 0);
		leftBytes -= nBytes;
		now += nBytes;
	}
}
