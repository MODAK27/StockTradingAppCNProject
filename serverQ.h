#ifndef SERVERQ_H
#define SERVERQ_H

#include <string>
#include <unordered_map>
#include <vector>
#include <netinet/in.h>
#include "helper.h"

using namespace std;

struct Sequence { 
    vector<double> prices; 
    int idx = 0; //  increments on every buy or sell of that stock.
};
/**
 * Boots up Server P:
 *  - Creates a UDP socket
 *  - Initializes and binds to P_UDP_PORT
 *  - Returns the UDP socket descriptor
 */
int bootUpServerQ();

/**
 * Read info from file "portfolios.txt" and store in DS
 * @param filename: the name of the file to read from
 */ 
map<string, Sequence> readInfoFromFile(string filename);


#endif // SERVERQ_H
