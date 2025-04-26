
#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <iomanip>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include "helper.h"
#include "serverM.h"

using namespace std;

// created string objects rather than string literals to be passed to healthCheck
string serverA = "Server A";
string serverP = "Server P";
string serverQ = "Server Q";
/**
 * serverM UDP and TCP socket 
 */
pair<int, int> bootUpServerM(){
    cout<<"[Server M] Booting up using UDP on port "<<M_PORT_UDP<<endl;
    struct sockaddr_in addrM;
    

    // 1. create UDP socket
    int serverM_udpSock = create_UDP_Socket("Server M");

    // 2. create address structure for server M
    initAddr(addrM, M_PORT_UDP);
 
    // 3. bind to associate UDPsocketM with addrM
    bindUDPSocket(serverM_udpSock, addrM, "Server M");

    // 4. create TCP socket
    int serverM_tcpSock = createUniqueTCPSocket("Server M");

    // 5. create address structure for server M
    sockaddr_in main_server_addr{};
    initAddr(main_server_addr, M_PORT_TCP);
    
    // 6. bind to associate TCPsocketM with addrM
    bindTCPSocket(serverM_tcpSock, main_server_addr, "Server M");
    
    return make_pair(serverM_udpSock, serverM_tcpSock);
 }



// Helper to check if the other servers are up
bool healthCheck(string &name, sockaddr_in &addr, int serverM_udpSock) {
    string ping = "PING";

    // cout << "[Server M] Checking " << name << " on UDP port " << port << "..." <<endl;
    // send ping
    udpRequestSendTo(ping, serverM_udpSock, addr, "Server M");
    // set receive timeout
    timeval tv{2, 0}; // define timeout of 2 second
    // set socket option to honour the timeout so that it doesn't block the thread indefinitely
    setsockopt(serverM_udpSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    string response = udpRequestReceiveFrom(serverM_udpSock, addr, "Server M", name);
    if (response == "PONG") {
        // cout << "[Server M] " << name << " is up." << endl; // -----------------
        return true;
    }
    else {
        // cerr << "[Server M] Unexpected response from " << name << ": " << response << endl;
        return false;
    }
}

string encryption(string &passwordToEncrypt) {
    string password;
    password.reserve(passwordToEncrypt.size());
    for (unsigned char c : passwordToEncrypt) {
        if (c >= 'A' && c <= 'Z') {
            password += char('A' + (c - 'A' + 3) % 26);
        }
        else if (c >= 'a' && c <= 'z') {
            password += char('a' + (c - 'a' + 3) % 26);
        }
        else if (c >= '0' && c <= '9') {
            password += char('0' + (c - '0' + 3) % 10);
        }
        else {
            // special characters unchanged
            password += c;
        }
    }
    return password;
}
double processPositionResponse(string &response, string &user, int serverM_udpSock, sockaddr_in &addr) {
    double gain = 0.0;
    istringstream responseStream(response);
    string line;
    cout<<"[Server M] Sent the quote request to server Q"<<endl; 
    while (getline(responseStream, line)) {
        if (line.empty()) continue;
        istringstream ss(line);
        string stockName;
        int shares;
        double avg_buy_price;

        if (!(ss >> stockName >> shares >> avg_buy_price)) {
            cerr << "[Server M] Warning: malformed position line: \"" 
                      << line << "\"\n";
            continue;
        }
        // send request to server Q to calculate the gain
        string positionReq = "QUOTE " + user + " " + stockName;
        udpRequestSendTo(positionReq, serverM_udpSock, addr, "Server M");
        
        string response = udpRequestReceiveFrom(serverM_udpSock, addr,  "Server M", "Server Q");

        string price = response.substr(response.find(' ') + 1); // remove stock name
        double currentPrice = stod(price);
        // calculate gain
        gain += (currentPrice - avg_buy_price) * shares;

    }
    cout<<"[Server M] Received quote response from server Q."<<endl;
    return gain;
}
void serveClient(int clientTCPSock, int serverM_udpSock, sockaddr_in &addrA, sockaddr_in &addrP, sockaddr_in &addrQ) {
    int attempts = 0;
    bool authenticated = false;
    string user = "";
    // ── Login retry loop ────────────────────────────────────────────────────────
    while (attempts < MAX_RETRIES && !authenticated) {

    
        user = receiveLineTCP(clientTCPSock, "");
        string password = receiveLineTCP(clientTCPSock, "");

        string hashed = encryption(password);
        string asterikPassword(password.size(), '*');

        cout<<"[Server M] Received username "<< user <<" and password "<< asterikPassword <<endl;

        string loginReq = "LOGIN " + user + " " + hashed;
        udpRequestSendTo(loginReq, serverM_udpSock, addrA, "Server M");
        cout<<"[Server M] Sent the authentication request to Server A"<<endl;

        string loginResponse = udpRequestReceiveFrom(serverM_udpSock, addrA,  "Server M", "Server A");
        cout<<"[Server M] Received the response from server A using UDP over port "<< M_PORT_UDP << ".\n";
        if (loginResponse != "OK") {
            sendTCP("Authentication failed\n", clientTCPSock, "Server M");
            attempts++;
        } else {
            sendTCP("Authentication successful\n", clientTCPSock, "Server M");
            authenticated = true;
        }
    }
    cout<<"[Server M] Sent the response from server A to the client using TCP over port "<<M_PORT_TCP <<endl;

    while(true) {
        string option = receiveLineTCP(clientTCPSock,   "Server M");

        // THINK as soon as position sent, again quote sent same time, then what? --------------------------
        if (option.rfind("exit",0)  == 0) {
            cout << "[Server M] Client disconnected." << endl;
            break; // then only this function is forced to return and the client got disconnected forcing sequential

        } else if (option.find("quote", 0) == 0) {
            handleClientQuotesRequest(option, user, serverM_udpSock, addrQ, clientTCPSock);

        } else if (option.find("position", 0) == 0) {
            handleClientPositionsRequest(user, serverM_udpSock, addrP, addrQ, clientTCPSock);

        } else if (option.find("buy", 0) == 0) {
            handleBuyRequest(user, option, clientTCPSock, serverM_udpSock, addrQ, addrP);

        } else if (option.find("sell", 0) == 0) {
            handleClientSellRequest(user, option, serverM_udpSock, addrQ, addrP, clientTCPSock);

        } else {
            cout << "[Server M] Invalid command. Please try again." << endl;
        }


    }
}
void handleBuyRequest(string &user, string &option, int clientTCPSock, int serverM_udpSock, sockaddr_in &addrQ, sockaddr_in &addrP) {
    
    cout << "[Server M] Received a buy request from member " << user + " using TCP over port " << M_PORT_TCP << "." << endl;
    istringstream ss(option);
    string buyOrSell;
    string stockName;
    int shares;
    ss >> buyOrSell >> stockName >> shares;

    // get the current price of the stock from server Q without advancing it now, 
    // advance at last, as stated in piazza, no change in price during buy/sell only at last either buy/sell confirmed/denied
    string quoteReq = "QUOTE " + user + " " + stockName;
    udpRequestSendTo(quoteReq, serverM_udpSock, addrQ,  "Server M");
    cout << "[Server M] Sent the quote request to server Q." << endl;
    string response = udpRequestReceiveFrom(serverM_udpSock, addrQ,  "Server M", "Server Q");
    cout << "[Server M] Received quote response from server Q" << endl;
    if (response == "") {
        string messageToSend = "[Client] Error: stock name does not exist. Please check again";
        sendTCP(messageToSend, clientTCPSock, "Server M");
        sendTCP("", clientTCPSock, "Server M");
        return;
    }
    
    string currentPrice = response.substr(response.find(' ') + 1); // remove stock name

    string messageToSend = "[Client] " + stockName + "'s current price is " + currentPrice + ". Proceed to buy? (Y/N)\n";
    sendTCP(messageToSend, clientTCPSock, "Server M");
    sendTCP("", clientTCPSock, "Server M");

    cout<<"[Server M] Sent the buy confirmation to the client."<<endl;
    string chosenPrompt = receiveLineTCP(clientTCPSock, "Server M");
    if (chosenPrompt == "Y" || chosenPrompt == "y")
    {
        cout<<"[Server M] Buy approved."<<endl;
        string positionReq = "BUY " + user + " " + stockName + " " + to_string(shares) + " " + currentPrice;
        udpRequestSendTo(positionReq, serverM_udpSock, addrP, "Server M");
        cout << "[Server M] Forwarded the buy confirmation response to server P." << endl;

        string responseFromServerP = udpRequestReceiveFrom(serverM_udpSock, addrP, "Server M", "Server P");
        if (responseFromServerP == "OK")
        {   sendTCP(responseFromServerP, clientTCPSock, "Server M");
            sendTCP("", clientTCPSock, "Server M");
            cout<<"[Server M] Forwarded the buy result to the client."<<endl;
        }
        
    } else if (chosenPrompt == "N" || chosenPrompt == "n")
    {
        sendTCP("DENIED", clientTCPSock, "Server M");
        sendTCP("", clientTCPSock, "Server M");
        cout<<"[Server M] Buy denied."<<endl;
    } else {
        cout<<"Not a valid operation."<<endl;
    }
    // send a time forward request to server Q to advance the time
    bool retFlag;
    timeForwardRequestForBuyOrSell(user, stockName, serverM_udpSock, addrQ, clientTCPSock, retFlag);
    if (retFlag)
        return;
}

void timeForwardRequestForBuyOrSell(string &user, string &stockName, int serverM_udpSock, sockaddr_in &addrQ, int clientTCPSock, bool &retFlag)
{
    retFlag = true;
    // send a time forward request to server Q to advance the time
    string positionReq = "ADVANCE " + user + " " + stockName;
    udpRequestSendTo(positionReq, serverM_udpSock, addrQ, "Server M");
    cout << "[Server M] Sent a time forward request for " << stockName << endl;

    string advanceResponse = udpRequestReceiveFrom(serverM_udpSock, addrQ, "Server M", "Server Q");
    if (advanceResponse == "NA")
    {
        string messageToSend = "[Client] Error: stock name does not exist. Please check again";
        sendTCP(messageToSend, clientTCPSock, "Server M");
        sendTCP("", clientTCPSock, "Server M");
        return;
    }
    retFlag = false;
}

void handleClientSellRequest(string &user, string &option, int serverM_udpSock, sockaddr_in &addrQ, sockaddr_in &addrP, int clientTCPSock)
{
    cout << "[Server M] Received a sell request from member " << user << " using TCP over port " << M_PORT_TCP << "." << endl;
    istringstream ss(option);
    string buyOrSell;
    string stockName;
    int shares;
    ss >> buyOrSell >> stockName >> shares;

    // get the current price of the stock from server Q without advancing it now, 
    // advance at last, as stated in piazza, no change in price during buy/sell only at last either buy/sell confirmed/denied
    string quoteReq = "QUOTE " + user + " " + stockName;
    udpRequestSendTo(quoteReq, serverM_udpSock, addrQ,  "Server M");
    cout << "[Server M] Sent the quote request to server Q." << endl;
    string response = udpRequestReceiveFrom(serverM_udpSock, addrQ,  "Server M", "Server Q");
    cout << "[Server M] Received quote response from server Q" << endl;
    if (response == "") {
        string messageToSend = "[Client] Error: stock name does not exist. Please check again";
        sendTCP(messageToSend, clientTCPSock, "Server M");
        sendTCP("", clientTCPSock, "Server M");
        return;
    }
    
    string currentPrice = response.substr(response.find(' ') + 1); // remove stock name

    string positionReq = "SELL " + user + " " + stockName + " " + to_string(shares) + " YES";
    udpRequestSendTo(positionReq, serverM_udpSock, addrP, "Server M");
    cout << "[Server M] Forwarded the sell request to server P." << endl;

    string responseFromServerP = udpRequestReceiveFrom(serverM_udpSock, addrP, "Server M", "Server P");
    string messageToSend = "";
    if (responseFromServerP == "FAILSTOCK")
    {
        messageToSend = "[Client] Error: stock name does not exist. Please check again \n";
        sendTCP(messageToSend, clientTCPSock, "Server M");
        sendTCP("", clientTCPSock, "Server M");
        // send a time forward request to server Q to advance the time
        bool retFlag;
        timeForwardRequestForBuyOrSell(user, stockName, serverM_udpSock, addrQ, clientTCPSock, retFlag);
        if (retFlag)
            return;
        return;
    }
    else if (responseFromServerP == "FAILQUANTITY") {
        messageToSend = "[Client] Error: " + user + " does not have enough shares of " + stockName + " to sell. Please try again. \n";
        sendTCP(messageToSend, clientTCPSock, "Server M");
        sendTCP("", clientTCPSock, "Server M");
        // send a time forward request to server Q to advance the time
        bool retFlag;
        timeForwardRequestForBuyOrSell(user, stockName, serverM_udpSock, addrQ, clientTCPSock, retFlag);
        if (retFlag)
            return;
        return;
    }
    else if (responseFromServerP == "OK_ELIGIBLE") {
        messageToSend = "[Client] " + stockName + " current price is " + currentPrice + ". Proceed to sell? (Y/N)\n";
    } else {
        cout<<"[Server M] Sell request for " << shares << " shares of " << stockName << " failed. UNEXPECTED" << endl;
    }
    sendTCP(messageToSend, clientTCPSock, "Server M");
    sendTCP("", clientTCPSock, "Server M");
    cout << "[Server M] Forwarded the sell confirmation to the client." << endl;

    string chosenPrompt = receiveLineTCP(clientTCPSock, "Server M");
    if (chosenPrompt == "Y" || chosenPrompt == "y")
    {
        string positionReq = "SELL " + user + " " + stockName + " " + to_string(shares) + " NO";
        udpRequestSendTo(positionReq, serverM_udpSock, addrP, "Server M");
        cout << "[Server M] Forwarded the sell confirmation response to Server P." << endl;

        string messageToSend = udpRequestReceiveFrom(serverM_udpSock, addrP, "Server M", "Server P");
        if (messageToSend == "OK_SOLD") {
            sendTCP("OK", clientTCPSock, "Server M");
            sendTCP("", clientTCPSock, "Server M");
            cout << "[Server M] Forwarded the sell result to the client." << endl;
        } else {
            cout<<"[Server M] Sell request for " << shares << " shares of " << stockName << " failed." << endl;
        }
        
    } else if (chosenPrompt == "N" || chosenPrompt == "n")
    {   
        sendTCP("DENIED", clientTCPSock, "Server M");
        sendTCP("", clientTCPSock, "Server M");
        cout << "[Server M] Sell denied." << endl;
    } else {
        cout << "Not a valid operation." << endl;
    }

    // send a time forward request to server Q to advance the time
    bool retFlag;
    timeForwardRequestForBuyOrSell(user, stockName, serverM_udpSock, addrQ, clientTCPSock, retFlag);
    if (retFlag)
        return;
    
}
void handleClientPositionsRequest(string &user, int serverM_udpSock, sockaddr_in &addrP, sockaddr_in &addrQ, int clientTCPSock)
{
    cout << "[Server M] Received a position request from Member to check " << user << "’s gain using TCP over port " << M_PORT_TCP << "." << endl;

    string positionReq = "POSITION " + user;
    udpRequestSendTo(positionReq, serverM_udpSock, addrP, "Server P");
    cout << "[Server M] Forwarded the position request to server P." << endl;

    string responseToBeSent = "stock shares avg_buy_price\n";
    string response = udpRequestReceiveFrom( serverM_udpSock, addrP, "ServerM", "Server P");
    responseToBeSent += response;

    cout << "[Server M] Received user’s portfolio from server P using UDP over " << M_PORT_UDP << "." << endl;
    double gain = processPositionResponse(response, user, serverM_udpSock, addrQ);
    ostringstream gainStr;
    gainStr << fixed << setprecision(2) << gain;
    responseToBeSent += user + "’s current profit is : " + gainStr.str() + "\n";

    sendTCP(responseToBeSent, clientTCPSock,    "Server M");
    sendTCP("", clientTCPSock, "Server M");
    cout << "[Server M] Forwarded the gain to the client." << endl;
}

void handleClientQuotesRequest(string &option, string &user, int serverM_udpSock, sockaddr_in &addrQ, int clientTCPSock)
{
    string quoteReq, response;
    if (option.length() == 5 || option.find_first_not_of(' ', 6) == string::npos)
    {
        // it means requested only QUOTE or "QUOTE  " (with trailing spaces), so send QUOTEALL to Server Q
        cout << "[Server M] Received a quote request from " << user << ", using TCP over port " << M_PORT_TCP << "." << endl;
        quoteReq = "QUOTEALL " + user;
        udpRequestSendTo(quoteReq, serverM_udpSock, addrQ,  "Server M");
        cout << "[Server M] Forwarded the quote request to server Q" << endl;
        response = udpRequestReceiveFrom( serverM_udpSock, addrQ, "Server M", "Server Q");
        cout << "[Server M] Received the quote response from server Q using UDP over " << M_PORT_UDP << "." << endl;
        // cout << "[Server M] Received quote response from server Q." << endl;
    }
    else
    {
        // means someone requested for quote for a specific stock, find post space the stock name
        string stockName = option.substr(option.find(' ') + 1);
        quoteReq = "QUOTE " + user + " " + stockName;
        cout << "[Server M] Received a quote request from " << user << " for stock " << stockName << ", using TCP over port " << M_PORT_TCP << "." << endl;
        udpRequestSendTo(quoteReq, serverM_udpSock, addrQ,  "Server M");
        cout << "[Server M] Forwarded the quote request to server Q" << endl;
        response = udpRequestReceiveFrom(serverM_udpSock, addrQ,  "Server M", "Server Q");
        if (response == "") response += stockName + " does not exist. Please try again.";
        cout << "[Server M] Received the quote response from server Q for stock " << stockName << " using UDP over " << M_PORT_UDP << "." << endl;
    }
    
    sendTCP(response, clientTCPSock, "Server M");
    sendTCP("", clientTCPSock, "Server M");
    cout << "[Server M] Forwarded the quote response to the client." << endl;
}
int main()
{
    signal(SIGCHLD, SIG_IGN);
    
    pair<int, int> serverM_sockets = bootUpServerM();
    int serverM_udpSock = serverM_sockets.first;
    int serverM_tcpSock = serverM_sockets.second;

    struct sockaddr_in addrA, addrP, addrQ;

    // Now set up the UDP destinations for A, P, and Q
    initAddr(addrA, A_UDP_PORT);
    initAddr(addrP, P_UDP_PORT);
    initAddr(addrQ, Q_UDP_PORT);

    // Check if the other servers are up
    int tries = 0;
    while (++tries <= MAX_RETRIES) {
        if (healthCheck(serverA, addrA, serverM_udpSock) &&
            healthCheck(serverP, addrP, serverM_udpSock) &&
            healthCheck(serverQ, addrQ, serverM_udpSock) ) {
                // cout<<"[Server M] All dependent servers are up. Proceeding to listen for client requests."<<endl;
                break;  
        }
        // cerr << "[Server M] Waiting for backends (attempt "<< tries << "/" << MAX_RETRIES << ")...\n";
        sleep(1);
    }
    if (tries > MAX_RETRIES) {
        cerr << "[Server M] One or more dependent servers are down. Exiting." << endl;
            exit(1);
    }

    // listen for incoming connections and holds multiple pending SYNs.
    if(listen(serverM_tcpSock, 10) == -1){
        perror("ERROR: Server M failed to listen on TCP socket");
        exit(1);
    }

    while (true) {
        // pulls the next one off the queue post listen command, returning a new socket descriptor
        int clientTCPSock = accept(serverM_tcpSock, nullptr, nullptr); 

        if (clientTCPSock<0) continue;
        pid_t pid = fork(); // create a child process
        // used fork to handle parallel clients only in server M layer, not post this as when 2 parallel call comes from 2 client at same tine 
        // and one UDP socket is used to communicate to other servers A,P,Q, packets might mix
        if (pid==0) {
            close(serverM_tcpSock); // close the listening socket in the child process
            serveClient(clientTCPSock, serverM_udpSock, addrA, addrP, addrQ); // in 163 line forced ssequential
            //  this way it ensures that one client has finished off it's execution then it handles the next one
            close(clientTCPSock);
            _exit(0);// stating that the child process is done
        }
        close(clientTCPSock);
    }

    close(serverM_tcpSock); close(serverM_udpSock);
    
    return 0;
}