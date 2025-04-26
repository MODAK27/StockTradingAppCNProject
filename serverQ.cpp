
#include <iostream>
#include <thread>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <map>
#include <string>
#include <map>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <cstring>
#include <fstream>
#include "helper.h"
#include "serverQ.h"
using namespace std;


/**
 * serverQ UDP socket -> create, connect to M, and bind and return socketA
 */
int bootUpServerQ(){
    cout<<"[Server Q] Booting up using UDP on port "<<Q_UDP_PORT<<endl;

    struct sockaddr_in addrQ;

    // 1. create UDP socket
    int serverQ_udpSock = create_UDP_Socket("Server Q");

    // 2. create address structure for server Q
    initAddr(addrQ, Q_UDP_PORT);

    // 3. bind to associate UDPsocketQ with addrQ
    bindUDPSocket(serverQ_udpSock, addrQ, "Server Q");
    return serverQ_udpSock;
 }

/**
 * Read info from file "quotes.txt" and store in DS
 * @param filename: the name of the file to read from
 */ 
map<string, Sequence> readInfoFromFile(string filename) {

    ifstream in(filename);
    if (!in) {
        cerr << "ERROR: Could not open file " << filename << endl;
        exit(1);
    }

    string line, currentUser;
    map<string,Sequence> quotes;

    while (getline(in, line)) {
        if (line.empty()) continue;

        istringstream ss(line);
        string stockName;
        double price;
        vector<double> prices;
        ss >> stockName;
        while(ss >> price) {
            prices.push_back(price);
        }
        Sequence seq;
        seq.prices = prices;
        seq.idx = 0; // Initialize the index to 0
        quotes[stockName] = seq;
    }
    return quotes;
}


int main() {
    signal(SIGCHLD, SIG_IGN); // kill all zombie processes
    char input_buffer_from_serverM[MAXBUFLEN];   

    int serverQ_udpSock = bootUpServerQ();
    map<string, Sequence> quotes = readInfoFromFile("quotes.txt");

    while (true) {
        struct sockaddr_in main_server_addr; 
        string receivedMsg = udpRequestReceiveFrom(serverQ_udpSock, main_server_addr, "Server Q", "Server M");

        if(processReadinessCheckOfServer(receivedMsg, serverQ_udpSock, main_server_addr, "Server Q")) {
            continue; // if the message is a PING, continue to the next iteration
        }

        strcpy(input_buffer_from_serverM, receivedMsg.c_str());
        
        // "The ServerQ received a request from the ServerM."
        
        // Process the received message
        istringstream ss(input_buffer_from_serverM);
        string action, user;
        ss >> action >> user;
        
        ostringstream response;

        if (action == "QUOTEALL") {
            cout << "[Server Q] Received a quote request from the main server.\n";
            
            for (auto &p : quotes) {
                auto &seq = p.second;
                double price = seq.prices[seq.idx];
                ostringstream priceTemp;
                priceTemp << std::fixed << std::setprecision(2) << price;
                response << p.first << " " << priceTemp.str() << "\n"; // returned stock name and price
            }
            auto out = response.str();
            udpRequestSendTo(out, serverQ_udpSock, main_server_addr,   "Server Q");
            cout << "[Server Q] Returned all stock quotes.\n";
        }
        else if (action == "QUOTE") {
            string stockName;
            ss >> stockName;
            cout << "[Server Q] Received a quote request from the main server for stock "<< stockName << ".\n";
            auto it = quotes.find(stockName);
            if (it == quotes.end()) {
                cerr << "[Server Q] Stock " << stockName << " not found.\n";
                response<< "";
                string responseStr = response.str();
                udpRequestSendTo(responseStr, serverQ_udpSock, main_server_addr,   "Server Q");
                
            } else {
                auto &seq = it->second;
                double price = seq.prices[seq.idx];
                ostringstream priceTemp;
                priceTemp << std::fixed << std::setprecision(2) << price;
                response<< stockName<< " " << priceTemp.str(); 
                string responseStr = response.str();
                udpRequestSendTo(responseStr, serverQ_udpSock, main_server_addr,   "Server Q");
                cout << "[Server Q] Returned the stock quote of " << stockName << ".\n";
            }
        }
        else if (action == "ADVANCE") {
            string stockName;
            ss >> stockName;
            auto it = quotes.find(stockName);
            if (it == quotes.end()) {
                cerr << "[Server Q] Stock " << stockName << " not found to advance.\n";
                string out = "NA";
                udpRequestSendTo(out, serverQ_udpSock, main_server_addr,   "Server Q");
                continue;
            }
            auto &seq = it->second;
            int t = seq.idx; // current time
            double price = seq.prices[seq.idx];
            ostringstream priceTemp;
            priceTemp << std::fixed << std::setprecision(2) << price;
            seq.idx = (seq.idx + 1) % seq.prices.size();
            
            cout << "[Server Q] Received a time forward request for " << stockName
                      << ", the current price of that stock is " << priceTemp.str()
                      << " at time " << t << ".\n";
            auto out = stockName + " " + priceTemp.str();
            udpRequestSendTo(out, serverQ_udpSock, main_server_addr,   "Server Q");
        } else {
            cerr << "[Server Q] Unknown action: " << action << "\n";
        }
    }
}
