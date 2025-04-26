
#include <iostream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
#include <iomanip>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include "helper.h"
#include "client.h"

using namespace std;


int bootUpClient(){
    cout<<"[Client] Booting up.\n";

    // create TCP socket
    int clientTCPSock = -1;

    sockaddr_in main_server_addr{};
    // call helper function to create the address struct for TCP port of main server
    initAddr(main_server_addr, M_PORT_TCP);

    //  connect to main server
    int attempts = 0;
    while (attempts++ < MAX_RETRIES) { // MAX_RETRIES here = 120 or 2 mins
        if (clientTCPSock >= 0) close(clientTCPSock);

        clientTCPSock = createUniqueTCPSocket("Client");

        // Most clients don’t need to bind to an ephemeral port; the OS picks one automatically when you call connect()
        if (connect(clientTCPSock, (sockaddr*)&main_server_addr, sizeof(main_server_addr)) == 0) {
            break;  // TCP connection to main server was success!
        }
        // perror("[Client] connect failed, retrying");
        sleep(1);
    }
    if (attempts > MAX_RETRIES) {
        cerr<<"[Client] Could not connect after "<< MAX_RETRIES <<" tries. Exiting.\n";
        exit(1);
    }
    return clientTCPSock;
 }

int main() {
    int sock = bootUpClient();
    cout << "[Client] Logging in.\n";
    string username, password;
    while (true) {
        cout << "Please enter the username: ";
        getline(cin, username);
        cout << "Please enter the password: ";
        getline(cin, password);
        
        // send username and password over TCP
        sendTCP(username + "\n", sock, "Client");
        sendTCP(password + "\n", sock, "Client");
        
        string response = receiveLineTCP(sock, "Client");

        if (response == "Authentication successful") {
            cout << "[Client] You have been granted access.\n";
            break;
        } else {
            cout << "[Client] The credentials are incorrect. Please try again.\n";
        }
    }

     // dynamic TCP port per client (used getsockname referenced from given pdf to get the dynamic port) -------------REFERENCE-------------
     sockaddr_in local;
     socklen_t len = sizeof(local);
     getsockname(sock, (sockaddr*)&local, &len);
     int port = ntohs(local.sin_port);

    // all commands requests
    while (true) {
        
        // Asking for commands
        cout << "[Client] Please enter the command:\n"
        "quote\n"
        "quote <stock name>\n"
        "buy <stock name> <number of shares>\n"
        "sell <stock name> <number of shares>\n"
        "position\n"
        "exit\n";
        cout<<"--------------------------------------\n";
        string cmd, response;
        if (!getline(cin, cmd)) {
            cmd = "exit";
        } else if (cmd.rfind("quote", 0) == 0) {

            sendTCP(cmd, sock, "Client");
            cout << "[Client] Sent a quote request to the main server.\n";
            response = receiveMultiLine(sock, "Client"); 
            cout << "[Client] Received the response from the main server using TCP over port "<< port << ".\n";
            cout << response;

        } else if (cmd == "position") {
            
            sendTCP(cmd, sock, "Client");
            cout << "[Client] " << username << " sent a position request to the main server.\n";
            response = receiveMultiLine(sock, "Client");
            cout << "[Client] Received the response from the main server using TCP over port "
                  << port << ".\n";
            cout << response;

        } else if (cmd.rfind("buy", 0) == 0) {
            bool retFlag = true;
            validateBuyCommand(cmd, retFlag); // validate the input 
            if (retFlag == false)
                continue;

            sendTCP(cmd, sock, "Client");
            cout << "[Client] " << username << " sent a buy request to the main server.\n";
            string response = receiveMultiLine(sock, "Client");
            cout<<response;
            
            if (response.find("Error") != string::npos) {
                continue; // start new request or reitertate
            }
            string chosenPrompt;
            while (true) {
                getline(cin, chosenPrompt);
                if (chosenPrompt == "Y" || chosenPrompt == "y" || chosenPrompt == "N" || chosenPrompt == "n") {
                    break;
                } else {
                    cout << "[Client] Invalid input. Please enter 'Y' or 'N': ";
                }
            }
            
            istringstream ss (cmd);
            string buyOrSell;
            string stockName;
            int shares;
            ss >> buyOrSell>>stockName >> shares;
            sendTCP(chosenPrompt, sock, "Client");
            response = receiveMultiLine(sock, "Client");
            if (response.find("OK") != string::npos) {
                cout << "[Client] Received the response from the main server using TCP over port " + to_string(port)<<endl;
                cout<< username + " successfully bought " + to_string(shares) + " shares of " + stockName + ".\n";
                
            } else if (response.find("DENIED") != string::npos) {
                cout << "[Client] " + username + " denied the buy request.\n";
                
            } else {
                cout << "[Client] Some Error Occured" << endl;
            }

        } else if (cmd.rfind("sell", 0) == 0) {
            bool retFlag = true;
            validateSellCommand(cmd, retFlag); // validate the sell input command
            if (retFlag == false)
                continue;

            sendTCP(cmd, sock, "Client");
            cout << "[Client] " << username << " sent a sell request to the main server.\n";
            string response = receiveMultiLine(sock, "Client");
            cout << response;

            if (response.find("Error") != string::npos) {
                continue;
            }
            string chosenPrompt;
            while (true) {
                getline(cin, chosenPrompt);
                if (chosenPrompt == "Y" || chosenPrompt == "y" || chosenPrompt == "N" || chosenPrompt == "n") {
                    break;
                } else {
                    cout << "[Client] Invalid input. Please enter 'Y' or 'N': ";
                }
            }
            istringstream ss (cmd);
            string buyOrSell;
            string stockName;
            int shares;
            ss >> buyOrSell>> stockName >> shares;
            sendTCP(chosenPrompt, sock, "Client");
            response = receiveMultiLine(sock, "Client");
            if (response.find("OK") != string::npos)
            {
                cout << "[Client] " + username + " successfully sold " + to_string(shares) + " shares of " + stockName + "\n";
            }
            else {
                cout << "[Server M] Sell request for " << shares << " shares of " << stockName << " failed." << endl;
            }
            
        } else if (cmd == "exit") {
            // do no thing here, one exit condition for codebase handled below
        } else {
            cout << "[Client] Invalid command. Please try again.\n";
        }

        if (cmd == "exit") {
            sendTCP(cmd, sock, "Client"); // to close the TCP conn by server M
            cout << "[Client] Exiting the client.\n";
            break;
        }
        cout << "—-------------Start a new request—--------------\n";
       
    }
    close(sock);
    return 0;
}
void validateBuyCommand(string &cmd, bool &retFlag)
{
    retFlag = true;
    // Validate "buy" command format checks buy <stock name> <number of shares> making it valid these 3 space separated words and quantity > 0
    size_t firstSpace = cmd.find(' ');
    size_t secondSpace = cmd.find(' ', firstSpace + 1);
    if (firstSpace == string::npos || secondSpace == string::npos){
        cout << "[Client] Error: stock name/shares are required. Please specify a stockname to buy.\n";
        retFlag = false;
        return;
    }
    string stockName = cmd.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    string quantityStr = cmd.substr(secondSpace + 1);
    int quantity = stoi(quantityStr);
    if (quantity <= 0) {
        cout << "[Client] Error: stock name/shares are required. Please specify a stockname to buy.\n";
        retFlag = false;
    }
}
void validateSellCommand(string &cmd, bool &retFlag)
{
    retFlag = true;
    // Validate "sell" command format checks buy <stock name> <number of shares> making it valid these 3 space separated words and quantity > 0
    size_t firstSpace = cmd.find(' ');
    size_t secondSpace = cmd.find(' ', firstSpace + 1);
    if (firstSpace == string::npos || secondSpace == string::npos){
        cout << "[Client] Error: stock name/shares are required. Please specify a stockname to sell.\n";
        retFlag = false;
        return;
    }
    string stockName = cmd.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    string quantityStr = cmd.substr(secondSpace + 1);
    int quantity = stoi(quantityStr);
    if (quantity <= 0) {
        cout << "[Client] Error: stock name/shares are required. Please specify a stockname to sell.\n";
        retFlag = false;
    }
}
