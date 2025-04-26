#ifndef HELPER_H
#define HELPER_H

#define MAXBUFLEN 1024     // max number of bytes at once
#define LOCALHOST "127.0.0.1" // localhost IP address

#include <string> // Include string library
#include <netinet/in.h> // Include for sockaddr_in
using namespace std;

/**
 * Hardcoded constants
 */
#define LOCALHOST "127.0.0.1"
#define M_PORT_TCP 45569 // server M TCP port
#define M_PORT_UDP 44569 // Server M UDP port
#define A_UDP_PORT 41569 // Server A UDP port
#define P_UDP_PORT 42569 // Server P UDP port
#define Q_UDP_PORT 43569 // Server Q UDP port
#define MAXBUFLEN 1024     // max number of bytes at once

#define MAX_RETRIES 120 // hardcoded to wait for 120 sec -> TAs feel free to change


string receiveLineTCP(int sock, string source);

string receiveMultiLine(int sock, string source);

void sendTCP(string s, int sock, string source);

int createUniqueTCPSocket(string source);

void bindTCPSocket(int server_udpSock, sockaddr_in &addr, string source);

void initAddr(sockaddr_in &addr, int port);

void udpRequestSendTo(string &msg, int udpSock, sockaddr_in &dest, string source);

string udpRequestReceiveFrom(int udpSock, sockaddr_in &dest, string source, string destination);

int create_UDP_Socket(string source);

void bindUDPSocket(int server_udpSock, sockaddr_in &addr, string source);

bool processReadinessCheckOfServer(string &receivedMsg, int server_udpSock, sockaddr_in &main_server_addr, string source);

#endif