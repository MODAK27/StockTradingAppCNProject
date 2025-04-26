
#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <cstring>  
#include <fstream> // Include for std::ifstream
#include "helper.h"
#include "serverP.h"


using namespace std;

/**
 * serverP UDP socket -> create, connect to M, and bind and return socketP
 */
int bootUpServerP(){
    cout<<"[Server P] Booting up using UDP on port "<<P_UDP_PORT<<endl;

    struct sockaddr_in addrP;

    // 1. create UDP socket
    int serverP_udpSock = create_UDP_Socket("Server P");

    // 2. create address structure for server P
    initAddr(addrP, P_UDP_PORT);

    // 3. bind to associate UDPsocketP with addrP
    bindUDPSocket(serverP_udpSock, addrP, "Server P");
    return serverP_udpSock;
 }

/**
 * Read info from file "portfolios.txt" and store in DS
 * @param filename: the name of the file to read from
 */ 
unordered_map<string, vector<Holding>> readInfoFromFile(string filename) {
    ifstream in(filename);
    if (!in) {
        cerr << "ERROR: Could not open file " << filename << endl;
        exit(1);
    }

    string line, currentUser;
    unordered_map<string, vector<Holding>> portfolios;

    auto isStockLine = [&](const string &line){
        istringstream ss(line);
        string name; int qty;
        /**
         Here only kept checking for stock name and quantity
         coz of edge cases in portfolios.txt
        James

        Mary 
        Patricia 
        S1 0 0
        S2 0
         */
        return bool(ss >> name >> qty);
    };
    
    while (getline(in, line)) {
        if (line.empty()) continue;
        line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1).erase(0, line.find_first_not_of(" \t\n\r\f\v")); 
        // to trim trailing white spaces to cover up edge cases if portfolios.txt has trailing spaces
        /** 
        James

        Mary 
        Patricia 
        S1 333 503.59
        S2 200 510.24 
        */
        if (!isStockLine(line)) {
            // it’s a username
            currentUser = line;
            portfolios[currentUser]; 
        } else {
            // it’s a stock entry
            istringstream ss(line);
            Holding h;
            ss >> h.stockName >> h.quantity;
            if (!(ss >> h.avgCostPrice)) {h.avgCostPrice = 0.0; }  // default price when missing for edge case when avgCostPrice is not given
            portfolios[currentUser].push_back(h);
        }
    }
    return portfolios;
}


int main() {
    signal(SIGCHLD, SIG_IGN); // kill all zombie processes
    char input_buffer_from_serverM[MAXBUFLEN];   // message from the main server

    int serverP_udpSock = bootUpServerP();
    unordered_map<string, vector<Holding>> portfolios = readInfoFromFile("portfolios.txt");

    while (true) {
        struct sockaddr_in main_server_addr; 
        string receivedMsg = udpRequestReceiveFrom(serverP_udpSock, main_server_addr, "Server P", "Server M");

        if(processReadinessCheckOfServer(receivedMsg, serverP_udpSock, main_server_addr, "Server P")) {
            continue; // if the message is a PING, continue to the next iteration
        }

        strcpy(input_buffer_from_serverM, receivedMsg.c_str());
        
        // The ServerP received a request from the ServerM
        
        // Process the received message
        istringstream ss(input_buffer_from_serverM);
        string action, user;
        ss >> action >> user;
        if (action == "POSITION") {
            cout << "[Server P] Received a position request from the main server for Member: "+ user +"\n";
            ostringstream response;
            for (auto &portfolio : portfolios[user]) {
                if (portfolio.quantity == 0) {
                    continue; // skip stocks with quantity 0
                }
                response << portfolio.stockName << " " << portfolio.quantity << " " << portfolio.avgCostPrice <<"\n";
            }
            string responseStr = response.str();
            udpRequestSendTo(responseStr, serverP_udpSock, main_server_addr, "Server P");
            cout << "[Server P] Finished sending the gain and portfolio of "<< user << " to the main server."<<endl;

        } else if (action == "SELL") {
            string stockName;
            int givenQuantity;
            string earlyCheck;
            ss >> stockName >> givenQuantity >> earlyCheck;
            cout << "[Server P] Received a sell request from the main server."<<endl;

            // Check if the user exists 
            if (!portfolios.count(user)) {
                cout<< "[Server P] User " << user << " does not exist. Unable to sell " <<endl;
                string failMessage = "FAIL";
                udpRequestSendTo(failMessage, serverP_udpSock, main_server_addr, "Server P");
            } else {
                // user exists 
                auto &vec = portfolios[user];
                auto it = find_if(vec.begin(), vec.end(), [&](auto &h){ return h.stockName == stockName; });
                if (earlyCheck == "YES") {
                    
                    // Check if the stock exists 
                    if (it == vec.end()) {
                        string failMessage = "FAILSTOCK";
                        udpRequestSendTo(failMessage, serverP_udpSock, main_server_addr, "Server P");

                    } // and no. of stocks requested are elgible for selling 
                    else if (it != vec.end() && it->quantity >= givenQuantity) {
                        cout << "[Server P] Stock " << stockName
                                << " has sufficient shares in " << user
                                << "'s portfolio. Requesting user's confirmation for selling stock."<<endl;
    
                        string successMessage = "OK_ELIGIBLE";
                        udpRequestSendTo(successMessage, serverP_udpSock, main_server_addr, "Server P");
                        // Wait for confirmation from the user to be sent in next message
                        
                    } else {
                        cout << "[Server P] Stock " << stockName
                                << " does not have enough shares in " << user
                                << "'s portfolio. Unable to sell " << givenQuantity
                                << " shares of " << stockName << "."<<endl;
                        string failMessage = "FAILQUANTITY";
                        udpRequestSendTo(failMessage, serverP_udpSock, main_server_addr, "Server P");
                    }
                } else {
                    //earlyCheck == "NO"
                    // It means that user has already confirmed the sell request and should proceed with selling
                    it->quantity -= givenQuantity;
                    if (it->quantity == 0) {
                        vec.erase(it); // remove the stock from the portfolio if quantity is 0
                    }
                    cout << "[Server P] Successfully sold " << givenQuantity
                            << " shares of " << stockName
                            << " and updated " << user << "'s portfolio."<<endl;
                    string successMessage = "OK_SOLD";
                    udpRequestSendTo(successMessage, serverP_udpSock, main_server_addr, "Server P");
                }
           
            }
            
        } else if (action == "BUY") {
            string stockName;
            int givenQuantity;
            double newPrice;
            ss >> stockName >> givenQuantity >> newPrice;
            cout << "[Server P] Received a buy request from the client."<<endl;
            if (!portfolios.count(user)) {
                cout<< "[Server P] User " << user << " does not exist. Unable to buy "<<endl;
                string failMessage = "FAIL";
                udpRequestSendTo(failMessage, serverP_udpSock, main_server_addr, "Server P");

            } else {
                // user exists 
                auto &vec = portfolios[user];
                auto it = find_if(vec.begin(), vec.end(), [&](auto &h){ return h.stockName == stockName; });
                if (it != vec.end()) {
                    // stock already exists in the portfolio
                    it->avgCostPrice = ((it->avgCostPrice * it->quantity) + (newPrice * givenQuantity)) / (it->quantity + givenQuantity);
                    it->quantity += givenQuantity;
                } else {
                    // new stock
                    portfolios[user].push_back({stockName, givenQuantity, newPrice});
                }
                cout << "[Server P] Successfully bought " << givenQuantity
                      << " shares of " << stockName
                      << " and updated " << user << "'s portfolio."<<endl;
                string successMessage = "OK";
                udpRequestSendTo(successMessage, serverP_udpSock, main_server_addr, "Server P");
            }
        } else {
            cout << "[Server P] Unknown action: " << action << endl;
            string failMessage = "FAIL";
            udpRequestSendTo(failMessage, serverP_udpSock, main_server_addr, "Server P");
        }
    }
    close(serverP_udpSock);
 }
