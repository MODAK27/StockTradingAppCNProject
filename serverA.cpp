#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream> // Include for std::ifstream
#include "helper.h"
#include "serverA.h"
#include <signal.h>
#include <cstring>  

using namespace std;


/**
 * serverA UDP socket -> create, connect to M, and bind and return socketA
 */
int bootUpServerA(){
    cout<<"[Server A] Booting up using UDP on port "<<A_UDP_PORT<<endl;

    struct sockaddr_in addrA;
    
    // 1. create UDP socket
    int serverA_udpSock = create_UDP_Socket("Server A");
    // 2. create address structure for server A
    initAddr(addrA, A_UDP_PORT);
 
    // 3. bind to associate UDPsocketA with addrA
    bindUDPSocket(serverA_udpSock, addrA, "Server A");
    return serverA_udpSock;
 }

/**
 * Read info from file "members.txt" and store in DS
 * @param filename: the name of the file to read from
 */ 
unordered_map<string, string> readInfoFromFile(string filename) {
    ifstream in(filename);
    if (!in) {
        cerr << "ERROR: Could not open file " << filename << endl;
        exit(1);
    }
    string user, pw;
    unordered_map<string, string> users;
    while (in >> user >> pw) {
        users[user] = pw;
    }
    return users;
}

 int main() {
    signal(SIGCHLD, SIG_IGN); // kill all zombie processes
    char input_buffer_from_serverM[MAXBUFLEN];   // message from the main server

    int serverA_udpSock = bootUpServerA();
    unordered_map<string, string> users = readInfoFromFile("members.txt");

    while (true) {
        struct sockaddr_in main_server_addr; // connector's (main server) address info // -------QUESTION------- when does it gets it's value
        string receivedMsg = udpRequestReceiveFrom(serverA_udpSock, main_server_addr, "Server A", "Server M");

        if(processReadinessCheckOfServer(receivedMsg, serverA_udpSock, main_server_addr, "Server A")) {
            continue; // if the message is a PING, continue to the next iteration
        }

        strcpy(input_buffer_from_serverM, receivedMsg.c_str());
        
        // Process the received message
        istringstream ss(input_buffer_from_serverM);
        string command, user, hashed_pw;
        ss >> command >> user >> hashed_pw;
        cout << "[Server A] Received username " << user << " and password ******.\n";

        bool isUserAllowedToAuthenticate = (users.count(user) && users[user] == hashed_pw);
        string response;
        if (isUserAllowedToAuthenticate) {
            response = "OK";
            cout << "[Server A] Member " << user << " has been authenticated.\n";
        } else {
            response = "FAIL";
            cout << "[Server A] The username " << user << " or password ****** is incorrect.\n";
        }
        udpRequestSendTo(response, serverA_udpSock, main_server_addr,   "Server A");
    }
    close(serverA_udpSock);
 }

