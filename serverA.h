#ifndef SERVERA_H
#define SERVERA_H

#include <string>
#include <unordered_map>
#include <netinet/in.h>
#include "helper.h"

using namespace std;

/**
 * Boots up Server A:
 *  - Creates a UDP socket
 *  - Initializes and binds to A_UDP_PORT
 *  - Returns the UDP socket descriptor
 */
int bootUpServerA();

/**
 * Reads username/hashed password pairs from the specified file.
 * @param filename Path to the members file
 * @return A map from username to hashed password
 */
unordered_map<string, string> readInfoFromFile(string filename);

void processReadinessCheckOfServer(string &receivedMsg, int serverA_udpSock, sockaddr_in &main_server_addr, int &retFlag);

#endif // SERVERA_H
