#ifndef SERVERM_H
#define SERVERM_H

#include <string>
#include <unordered_map>
#include <netinet/in.h>
#include "helper.h"

using namespace std;

/**
 * Boots up Server M:
 *  - Creates a UDP socket and TCP socket
 *  - Initializes and binds to M_UDP_PORT and M_TCP_PORT
 *  - Returns the UDP socket descriptor and TCP socket descriptor
 */
pair<int, int> bootUpServerM();

// Helper to check if the other servers are up
bool healthCheck(string &name, sockaddr_in &addr, int serverM_udpSock);

string encryption(string &passwordToEncrypt);

double processPositionResponse(string &response, string &user, int serverM_udpSock, sockaddr_in &addr);

void serveClient(int clientTCPSock, int serverM_udpSock, sockaddr_in &addrA, sockaddr_in &addrP, sockaddr_in &addrQ);

void handleBuyRequest(string &user, string &option, int clientTCPSock, int serverM_udpSock, sockaddr_in &addrQ, sockaddr_in &addrP);

void timeForwardRequestForBuyOrSell(string &user, string &stockName, int serverM_udpSock, sockaddr_in &addrQ, int clientTCPSock);

void handleClientSellRequest(string &user, string &option, int serverM_udpSock, sockaddr_in &addrQ, sockaddr_in &addrP, int clientTCPSock);

void handleClientPositionsRequest(string &user, int serverM_udpSock, sockaddr_in &addrP, sockaddr_in &addrQ, int clientTCPSock);

void handleClientQuotesRequest(string &option, string &user, int serverM_udpSock, sockaddr_in &addrQ, int clientTCPSock);

#endif // SERVERM_H