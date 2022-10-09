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
#include <sstream>
#include <map>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
using namespace std;
const int MXL = 1e3, MXClient = 1e3;
struct UserInfo { string name, email, password; };
map<string, UserInfo> nameMap, emailMap;
struct ClientStatus {
	bool login = false;
	string username;
} CS[MXClient + 1];
vector<int> rmSet;
string RecvMsg(int);
void SendMsg(int, string, bool);
void DealWithCommand(int, string);
void CommandLogin(int, vector<string>&);
void CommandLogout(int, vector<string>&);
void CommandStartGame(int, vector<string>&);
bool Is4DigitNumber(string&);
string RandomPick4Digit();
string GetResponse(string&, string&);
void CommandExit(int, vector<string>&);
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
	FD_ZERO(&allSet);
	FD_SET(tcpFd, &allSet);
	FD_SET(udpFd, &allSet);
	set<int> clientSet; set<int>::iterator it;
	while(true) {
		rSet = allSet;
		select(mxFd, &rSet, NULL, NULL, NULL);
		// Listen and Accept
		if(FD_ISSET(tcpFd, &rSet)) {
			int clientFd = accept(tcpFd, NULL, NULL);
			FD_SET(clientFd, &allSet);
			mxFd = max(mxFd, clientFd + 1);
			clientSet.insert(clientFd);
			SendMsg(clientFd, "*****Welcome to Game 1A2B*****\n", 0);
			SendMsg(clientFd, "% ", 1);
		}
		// Handle UDP Message
		if(FD_ISSET(udpFd, &rSet)) {
			
		}
		// Check Each Client 
		rmSet.clear();
		for(it = clientSet.begin(); it != clientSet.end(); it++) {
			int clientFd = *it;
			if(FD_ISSET(clientFd, &rSet)) {
				string msg = RecvMsg(clientFd);
				if(msg == "") rmSet.push_back(clientFd);
				else {
					msg.pop_back();
					DealWithCommand(clientFd, msg);
					if(rmSet.empty() || rmSet.back() != clientFd)
						SendMsg(clientFd, "% ", 1);
				}
			}
		}
		// Remove Closed Clients
		for(int i = 0; i < (int) rmSet.size(); i++) {
			CS[rmSet[i]].login = false;
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
		msg += BUF[0];
		if(BUF[0] == '\n') break;
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
void DealWithCommand(int fd, string s) {
	if(!s.length()) return;
	stringstream ss; ss << s;
	string str; vector<string> v;
	while(ss >> str) v.push_back(str);
	if(v.front() == "login") CommandLogin(fd, v);
	else if(v.front() == "logout") CommandLogout(fd, v);
	else if(v.front() == "start-game") CommandStartGame(fd, v);
	else if(v.front() == "exit") CommandExit(fd, v);
	else SendMsg(fd, "ERROR: Command Not Found\n", 0);
}
void CommandLogin(int fd, vector<string>& v) {
	if(v.size() != 3) SendMsg(fd, "Usage: login <username> <password>\n", 0);
	else if(CS[fd].login) SendMsg(fd, "Please logout first.\n", 0);
	else if(nameMap.find(v[1]) == nameMap.end()) SendMsg(fd, "Username not found.\n", 0);
	else if(v[2] != nameMap[v[1]].password) SendMsg(fd, "Password not correct.\n", 0);
	else {
		CS[fd].login = true;
		CS[fd].username = v[1];
		SendMsg(fd, "Welcome, " + v[1] + ".\n", 0);
	}
}
void CommandLogout(int fd, vector<string>& v) {
	if(v.size() != 1) SendMsg(fd, "Usage: logout\n", 0);
	else if(!CS[fd].login) SendMsg(fd, "Please login first.\n", 0);
	else {
		CS[fd].login = false;
		SendMsg(fd, "Bye, " + CS[fd].username + ".\n", 0);
	}
}
void CommandStartGame(int fd, vector<string>& v) {
	if(v.size() > 2) SendMsg(fd, "Usage: start-game <4-digit number>\n", 0);
	else if(!CS[fd].login) SendMsg(fd, "Please login first.\n", 0);
	else if(v.size() == 2 && !Is4DigitNumber(v.back())) SendMsg(fd, "Usage: start-game <4-digit number>\n", 0);
	else {
		string question = v.back();
		if(v.size() == 1) question = RandomPick4Digit();
		SendMsg(fd, "Please typing a 4-digit number:\n", 0);
		int usedTime = 5;
		while(usedTime) {
			string guess; cin >> guess;
			if(!Is4DigitNumber(guess)) SendMsg(fd, "Your guess should be a 4-digit number.\n", 0);
			else {
				usedTime--;
				if(guess == question) {
					SendMsg(fd, "You got the answer!\n", 0);
					return;
				}
				else SendMsg(fd, GetResponse(guess, question), 0);
			}
		}
		SendMsg(fd, "You lose the game!\n", 0);
	}
}
bool Is4DigitNumber(string& s) {
	if(s.length() != 4) return false;
	for(int i = 0; i < 4; i++) if(!isdigit(s[i])) return false;
	return true;
}
string RandomPick4Digit() {
	srand(time(NULL));
	string question;
	for(int i = 0; i < 4; i++) {
		int x = rand() % 10;
		question = (char) (x + '0');
	}
	return question;
}
string GetResponse(string& guess, string& ans) { return "123"; }
void CommandExit(int fd, vector<string>& v) {
	if(v.size() != 1) SendMsg(fd, "Usage: exit\n", 0);
	else rmSet.push_back(fd);
}
