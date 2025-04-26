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

void validateBuyCommand(string &cmd, bool &retFlag);

void validateSellCommand(string & cmd, bool &retFlag);

#endif // CLIENT_H
