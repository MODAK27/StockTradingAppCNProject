#include <string>
#include <cstring>   
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helper.h"
#include <errno.h>

using namespace std;
#define MAXBUFLEN 1024     // max number of bytes at once
#define LOCALHOST "127.0.0.1" // localhost IP address

string destinationName (string source) {
    if (source.find("Server")!= string::npos) return "Client";
    else if (source.find("Client")!= string::npos) return "Server";
    else return "Unknown";
}

// ----------------TCP helpers--------------------

string receiveLineTCP(int sock, string source) {
    string line;
    char ch;
    while (true) {
        int n = read(sock, &ch, 1);
        if (n < 0) {
            perror((source + " read").c_str());
            exit(1);
        }
        if (n == 0) {
            cerr<< "ERROR: " << source << " failed to receive TCP information from " << destinationName(source) << " " << endl;
            exit(1);
        }
        if (ch == '\n') {
            break;             // full line received
        } 
        if (ch == '\r') {
            continue;          // skip CR if sent as CRLF
        }
        line.push_back(ch);
    }
    return line;
}
string receiveMultiLine(int sock, string source) {
    string result = "";
    while (true) {
        string line = receiveLineTCP(sock, source);
        if (line.empty()) break;     // server must send a blank line at the end
        result += line + "\n";
    }
    return result;
}
void sendTCP(string msg, int sock, string source) {
    string out = msg;
    if (out.empty() || out.back() != '\n') {
        out.push_back('\n');
    }
    if(send(sock, out.c_str(), out.size(), 0) == -1){
        cerr << "ERROR: " << source << " failed to send UDP request To " << destinationName(source) << " " << endl;
        exit(1);
    }
}
// create TCP socket
int createUniqueTCPSocket(string source) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        cerr<<"ERROR: "<< source<<" failed to create TCPsocket"<<endl;
        exit(1);
    }
    return sock;
}
void bindTCPSocket(int server_udpSock, sockaddr_in &addr, string source)
{    // need :: (global scope resolution operator) to use bind for sockets from socket.h
    if (::bind(server_udpSock, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1){
        cerr<<"ERROR: "<<source<<" could not bind TCP socket"<<endl;
        exit(1);
    }
}
// --------------------UDP helpers--------------------

void udpRequestSendTo(string &msg, int udpSock, sockaddr_in &dest, string source) {
    if(sendto(udpSock, msg.c_str(), msg.size(), 0, (const sockaddr*)(&dest), sizeof(dest))== -1) {
        cerr << "ERROR: " << source << " failed to send UDP reqeust to " << destinationName(source) << " " << endl;
        exit(1);
    }
}
string udpRequestReceiveFrom(int udpSock, sockaddr_in &dest, string source, string destination) {
    char buf[MAXBUFLEN];
    socklen_t addr_len = sizeof(dest);
    int n = recvfrom(udpSock, buf, sizeof(buf)-1, 0, (sockaddr*)(&dest), &addr_len);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // timeout—caller will treat this as "no reply yet"
            return "";
        }
        // a genuine socket error:
        cerr << "ERROR: " << source << " failed to receive UDP information from " << destination << " " << endl;
        exit(1); 
    }
    buf[n] = '\0';
    return string(buf);
}

// create address structure 
void initAddr(sockaddr_in &addr, int port) {
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_pton(AF_INET, LOCALHOST, &addr.sin_addr);
}

// create UDP socket
int create_UDP_Socket(string source) {
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    if(sock == -1){
       cerr<<"ERROR: Server "<< source<<" failed to create UDPsocket"<<endl;
       exit(1);
    }
    return sock;
}
void bindUDPSocket(int server_udpSock, sockaddr_in &addr, string source)
{    // need :: (global scope resolution operator) to use bind for sockets from socket.h
    if (::bind(server_udpSock, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1){
        cerr<<"ERROR: "<< source<<" could not bind UDP socket"<<endl;
        exit(1);
    }
}

// ------------------- Other helpers -----------------
bool processReadinessCheckOfServer(string &receivedMsg, int server_udpSock, sockaddr_in &main_server_addr, string source)
{
    if (receivedMsg == "PING")
    {
        // cout <<"["<< source<<"] Received PING from Server M. Responding with PONG." << endl; // -------------------
        string pong = "PONG";
        udpRequestSendTo(pong, server_udpSock, main_server_addr, source);
        return true;
    } else {
        return false;
    }
}
