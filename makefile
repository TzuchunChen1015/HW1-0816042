all: server client
server: server.cpp
	g++ server.cpp -o server
client: client.cpp
	g++ client.cpp -o client
clean:
	rm server client
