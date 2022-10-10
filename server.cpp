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
struct UserInfo {
	string name, email, password;
	UserInfo() {}
	UserInfo(string n, string e, string p) {
		this->name = n;
		this->email = e;
		this->password = p;
	}
};
map<string, UserInfo> nameMap, emailMap;
struct ClientStatus {
	bool login = false;
	string username;
	bool inGame = false;
	int timeLeft = 5;
	string question;
} CS[MXClient + 1];
vector<int> rmSet;
void Init();
string RecvMsg(int);
void SendMsg(int, string, bool);
void SendMsgUDP(int, string, sockaddr_in&);
void CommandRegister(int, vector<string>&, sockaddr_in&);
void CommandGameRule(int, vector<string>&, sockaddr_in&);
void DealWithCommand(int, string);
void CommandLogin(int, vector<string>&);
void CommandLogout(int, vector<string>&);
void CommandStartGame(int, vector<string>&);
void PlayGame(int, string);
bool Is4DigitNumber(string&);
string RandomPick4Digit();
string GetResponse(string&, string&);
void CommandExit(int, vector<string>&);
int main(int argc, char** argv) {
	Init();
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
			sockaddr_in clientAddr;
			bzero(&clientAddr, sizeof(clientAddr));
			char BUF[MXL + 1]; int nBytes; socklen_t len;
			nBytes = recvfrom(udpFd, BUF, sizeof(BUF), 0, (struct sockaddr*) &clientAddr, &len);
			sendto(udpFd, BUF, strlen(BUF), 0, (struct sockaddr*) &clientAddr, sizeof(clientAddr));
			BUF[nBytes] = '\0';
			string msg = BUF;
		cout << msg << '\n';
			stringstream ss; ss << msg; 
			string str; vector<string> v;
			while(ss >> str) v.push_back(str);
			if(v.front() == "register") CommandRegister(udpFd, v, clientAddr);
			else if(v.front() == "game-rule") CommandGameRule(udpFd, v, clientAddr);
			else SendMsgUDP(udpFd, "ERROR: Command Not Found\n", clientAddr);
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
					if(!CS[clientFd].inGame && (rmSet.empty() || rmSet.back() != clientFd))
						SendMsg(clientFd, "% ", 1);
				}
			}
		}
		// Remove Closed Clients
		for(int i = 0; i < (int) rmSet.size(); i++) {
			CS[rmSet[i]].login = CS[rmSet[i]].inGame = false, CS[rmSet[i]].timeLeft = 5;
			FD_CLR(rmSet[i], &allSet);
			clientSet.erase(rmSet[i]);
			close(rmSet[i]);
			cout << "Number " << rmSet[i] << " disconnected\n";
		}	
	}
	return 0;
}
void Init() {
	nameMap.clear(), emailMap.clear();
	nameMap["Peter"] = emailMap["peter@google.com"] = UserInfo("Peter", "peter@google.com", "0000");
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
void SendMsgUDP(int fd, string s, sockaddr_in& clientAddr) {
	char BUF[MXL + 1];
cout << ntohs(clientAddr.sin_port) << '\n';
cout << s << '\n';
	strcpy(BUF, s.c_str()); BUF[s.length()] = '\0';
	sendto(fd, BUF, strlen(BUF) + 1, 0, (struct sockaddr*) &clientAddr, sizeof(clientAddr));
}
void CommandRegister(int fd, vector<string>& v, sockaddr_in& clientAddr) {
	if(v.size() != 4) SendMsgUDP(fd, "register <username> <email> <password>\n", clientAddr);
	else if(nameMap.find(v[1]) != nameMap.end()) SendMsgUDP(fd, "Username is already used.\n", clientAddr);
	else if(emailMap.find(v[2]) != emailMap.end()) SendMsgUDP(fd, "Email is already used.\n", clientAddr);
	else {
		nameMap[v[1]] = emailMap[v[2]] = UserInfo(v[1], v[2], v[3]);
		SendMsgUDP(fd, "Register successfully.\n", clientAddr);
	}
}
void CommandGameRule(int fd, vector<string>& v, sockaddr_in& clientAddr) {
	if(v.size() != 1) SendMsgUDP(fd, "Usage: game-rule\n", clientAddr);
	else {
		string msg = "1. Each question is a 4-digit secret number.\n"
					"2. After each guess, you will get a hint with the following information:\n"
					"2.1 The number of \"A\", which are digits in the guess that are in the correct position.\n"
					"2.2 The number of \"B\", which are digits in the guess that are in the answer but are in the wrong position.\n"
					"The hint will be formatted as \"xAyB\".\n"
					"3. 5 chances for each question.\n";
		SendMsgUDP(fd, msg, clientAddr);
	}
}
void DealWithCommand(int fd, string s) {
	if(!s.length()) return;
	else if(CS[fd].inGame) PlayGame(fd, s);
	else {
		stringstream ss; ss << s;
		string str; vector<string> v;
		while(ss >> str) v.push_back(str);
		if(v.front() == "login") CommandLogin(fd, v);
		else if(v.front() == "logout") CommandLogout(fd, v);
		else if(v.front() == "start-game") CommandStartGame(fd, v);
		else if(v.front() == "exit") CommandExit(fd, v);
		else SendMsg(fd, "ERROR: Command Not Found\n", 0);
	}
}
void CommandLogin(int fd, vector<string>& v) {
	if(v.size() != 3) SendMsg(fd, "Usage: login <username> <password>\n", 0);
	else if(CS[fd].login) SendMsg(fd, "Please logout first.\n", 0);
	else if(nameMap.find(v[1]) == nameMap.end()) SendMsg(fd, "Username not found.\n", 0);
	else if(v[2] != nameMap[v[1]].password) SendMsg(fd, "Password not correct.\n", 0);
	else {
		CS[fd].login = true, CS[fd].inGame = false, CS[fd].timeLeft = 5, CS[fd].username = v[1];
		SendMsg(fd, "Welcome, " + v[1] + ".\n", 0);
	}
}
void CommandLogout(int fd, vector<string>& v) {
	if(v.size() != 1) SendMsg(fd, "Usage: logout\n", 0);
	else if(!CS[fd].login) SendMsg(fd, "Please login first.\n", 0);
	else {
		CS[fd].login = CS[fd].inGame = false, CS[fd].timeLeft = 5;
		SendMsg(fd, "Bye, " + CS[fd].username + ".\n", 0);
	}
}
void CommandStartGame(int fd, vector<string>& v) {
	if(v.size() > 2) SendMsg(fd, "Usage: start-game <4-digit number>\n", 0);
	else if(!CS[fd].login) SendMsg(fd, "Please login first.\n", 0);
	else if(v.size() == 2 && !Is4DigitNumber(v.back())) SendMsg(fd, "Usage: start-game <4-digit number>\n", 0);
	else {
		CS[fd].question = v.back(), CS[fd].inGame = true, CS[fd].timeLeft = 5;
		if(v.size() == 1) CS[fd].question = RandomPick4Digit();
		SendMsg(fd, "Please typing a 4-digit number:\n", 1);
	}
}
void PlayGame(int fd, string guess) {
	if(!Is4DigitNumber(guess)) SendMsg(fd, "Your guess should be a 4-digit number.\n", 1);
	else {
		CS[fd].timeLeft--;
		if(guess == CS[fd].question) {
			SendMsg(fd, "You got the answer!\n", 0);
			CS[fd].inGame = false, CS[fd].timeLeft = 5;
		}
		else {
			SendMsg(fd, GetResponse(guess, CS[fd].question) + "\n", (CS[fd].timeLeft > 0));
			if(!CS[fd].timeLeft) {
				SendMsg(fd, "You lose the game!\n", 0);
				CS[fd].inGame = false, CS[fd].timeLeft = 5;
			}
		}
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
		question += (char) (x + '0');
	}
	return question;
}
// WARNING: To Be Added...	//
string GetResponse(string& guess, string& ans) { return "123"; }
// WARNING					//
void CommandExit(int fd, vector<string>& v) {
	if(v.size() != 1) SendMsg(fd, "Usage: exit\n", 0);
	else rmSet.push_back(fd);
}
