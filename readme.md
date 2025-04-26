1. Name : Debasish Modak

2. USC ID : 5955298569

3. PROJECT DESCRIPTION :

    The project deals with creating a simple backend oriented STOCK TRADING SYSTEM using C++ and Socket programming(TCP(both dynamic and static) and UDP protocols) mimicing a real time trade. Here we can have multiple clients talking to the main server which internally talks to 3 diff servers. This simple architecture demonstrates the fundamental concepts of distributed systems, networking, and secure client-server interactions. It follows a pre defined protocol of dynamic TCP ports for clients and static TCP port for Main serverM and static UDP portss for all servers. Here in Main server only one static UDP port is used to communicate with multiple static ports owned by diff servers in process. Client to Server is connection oriented thus following TCP protocol and server to server is connectionless oriented thus following UDP protocol. 
    In my implementation, first the servers are spunned up and serverM does a healtCheck via UDP req like "PING PONG" to see whether other servers are already spunned up post reading their own DB and got booted up or not and retries for 2 mins alike "POLLING". As soon as server M's healthCheck on other servers are done, client which was already on, was trying to hit the server M for starting the connection "POLLING", can finally talk with it and connect and finally shows as "Logging in" post TCP handshake. Adhering to the TCP and UDP protocols, I have implemented a system where clients first request the server for authentication using their own credentials. The server checks the authenticity and grants access to client to start the communication. The main server post authentication spins up a child sockets for each clients and returns to parent to listen to more comnnections or clients with a bucket size of 10. Thus each connected client session info is stored in it's own socket in main server or you can say it session based data. Post authentication, the clients then requests for services like getting it's own portfolio only and the current prices of stock(s) and buy/sell request. On proper buy/sell command, the current price of the stock gets forwarded as well . On completion of the process for user, it exits the system. Internally, on exiting the client sends a exit command to main server which then closes off the child sockets or disconnects the session. 
    
4.  • ServerA.cpp (AUTHENTICATION SERVER) loads all encrypted user credentials from "members.txt" and server is used to process authentication forclients. 
    • ServerP.cpp (PORTFOLIO SERVER) loads all users portfolio data from "portfolios.txt" which serves its DB. ServerP is for managing portfolios like any buy or sell request and checking the validity of the buy/sell request as well. Apart from that it servers it's purpose of displaying the portfolio which is the current stocks user holds and it's average cost price.
    • ServerQ.cpp (QUOTES SERVER) loads all stock's Stock Prices from "quotes.txt" which get's forwarded on every advance request and performing a circular roattion on the array on completion of it's length. ServerQ is used for giving the current time quotes of a stock or all stocks. For buy/sell requests , it forwards the time by 1 jump or updates the stockPrice of that specific stock by the next value mimicing a real time moving stockPrice in market.
    • ServerM.cpp (MAIN SERVER) The main server handles talking to all other servers and the clients and performs extra calculations such as encrypting password to be sent to serverA and also user's gain or updating the average cost price of a stock per user as required. 
    • Client.cpp (Client ) It is available to the end users as a screen to authenticate with the server and once logged in can see their holdings/current price of stock/ and buy/sell requests as well.
    • Helper.cpp : It has constants and some helper functions to create the TCP and UDP sockets, addresses, bindings, send and receive common requests.

5. Format of messages exchanged :
    TCP-----( client - server)
    5.1.1 username and password both (sent) to TCP socket as a separate with trailing new line character's to each to differentiate it
    5.1.2 username and password (received) from TCP socket and distinguised with trailing new line char

    5.2 all commands from client i.e. quote/ quote <stockName> / buy <quantity> <stockName> are sent in a single line with spaces and a trailing new line as delimiter ---> i.e. see "sendTCP" in helper.cpp

    5.3 when multiline message has to be retrieved via TCP socket to client, I used "" empty space as delimiter i.e. lines 204, 212 , 226 in serverM.cpp and many more --> see "receiveMultiLine" in helper.cpp. on how the receiving is done. Made it  more generic as client majorly uses receiveMultiLine 

    UDP------(server -server) 
    5.4 all UDP req and responses are sent as space separated words with trailing new line char as delimiter.     ----> see for sending i.e. line 278 in serverM.cpp.

6. Any idiosyncrasy in project. Conditions in which the project fails, if any.
    1. Implemented POLLING in client->server (TCP ports) to check serverM is on or not and sleeps for 1 sec  if not. Linked with MAX_RETRIES= 120 or 2 mins in line 22 helper.h. If the serverM isn't on till 2 mins, client can close booting up.
    2. Implemented the same POLLING in serverM via PING PONG req between server-server UDP ports. ServerM once on checks for other servers availability for same MAX_RETRIES = 120 or 2 mins. If any of the other dependent servers aren't on in 2 mins, server M can boot off.
    3. Since same UDP port is used for talking to multiple servers, parallel clients can't talk at exact same time or race condition. Simultaneous actions from separate clients are possible. If user A buys S1 , user B will see a reflected updated stock price when userB requests quote or any command, just not exact parallel but simultaneous with 0.1 sec delay is goood.
    4. The pending-connection queue for clients is 10 or up to 10 fully established handshakes (SYN→SYN-ACK→ACK) can wait to be accepted. On accept(), that connection is removed from the backlog. There is no cap on active connections. You can have unlimited theoretically. line 432 in serverM.cpp

7. Reused Code?
    Referred to Beej's socket programming and googled the syntaxes of sendTCP/recieveTCP/ best way to receive the multiline TCP response , same for UDP send and requests, (all these ref code in helper.cpp)  and for creation of child proccesses in main server using fork to handle clients concurrently 453-469 in serverM.cpp. Also commented in codebase theese parts referenced by "-------------REFERENCE-------------" sign.

8. Ubuntu 22.04 for MAC users
