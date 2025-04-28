#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <unordered_map>
#include <netinet/in.h>
#include "helper.h"

using namespace std;

/**
 * Boots up Client:
 *  - Creates a TCP socket
 *  - Initializes and binds to dynamic port
 *  - Returns the TCP socket descriptor
 */
int bootUpClient();

void validateCommand(string &cmd, bool &retFlag, string command);

#endif // CLIENT_H
