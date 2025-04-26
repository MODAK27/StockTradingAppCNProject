#ifndef SERVERP_H
#define SERVERP_H

#include <string>
#include <unordered_map>
#include <vector>
#include <netinet/in.h>
#include "helper.h"

using namespace std;

struct Holding { 
    string stockName; 
    int quantity; 
    double avgCostPrice; 
};

/**
 * Boots up Server P:
 *  - Creates a UDP socket
 *  - Initializes and binds to P_UDP_PORT
 *  - Returns the UDP socket descriptor
 */
int bootUpServerP();

/**
 * Read info from file "portfolios.txt" and store in DS
 * @param filename: the name of the file to read from
 */ 
unordered_map<string, vector<Holding>> readInfoFromFile(string filename);


#endif // SERVERP_H
