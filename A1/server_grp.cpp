#include <cstddef>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <bits/stdc++.h>
#include <mutex>

using namespace std;

#define PORT 12345
#define BUFF_SZ 1024 // username buffer size
#define MSG_SZ 2048  // message buffer size

const char *banner = R"(
██╗    ██╗███████╗██╗      ██████╗ ██████╗ ███╗   ███╗███████╗
██║    ██║██╔════╝██║     ██╔════╝██╔═══██╗████╗ ████║██╔════╝
██║ █╗ ██║█████╗  ██║     ██║     ██║   ██║██╔████╔██║█████╗  
██║███╗██║██╔══╝  ██║     ██║     ██║   ██║██║╚██╔╝██║██╔══╝  
╚███╔███╔╝███████╗███████╗╚██████╗╚██████╔╝██║ ╚═╝ ██║███████╗
 ╚══╝╚══╝ ╚══════╝╚══════╝ ╚═════╝ ╚═════╝ ╚═╝     ╚═╝╚══════╝             
)";

map<string, string> auth;                // map of username to password
set<int> client_set;                     // set of all client sockets
map<string, int> userToSocket;           // map of username to socket
map<int, string> socketToUser;           // map of client fd (socket) to username
map<string, set<string>> groupToMembers; // map of group name to set of usernames in group

mutex mtx;   // lock
int sock_fd; // server socket

// Helper functions
bool isEmpty(const char str[]);
void send_message(int client_sock, string message);
void group_mssg(string message, string group_name, int client_fd);
void private_mssg(string message, int recv_fd);
void broadcast(string message, int broadcast_fd);

// Handler functions
void client_handle(int client_fd);                                              // handles all activities of a client
void handle_msg(char *message, int client_fd, string &username);           // handles private messaging feature 
void handle_broadcast(std::string &username, char *message, int &client_fd);    // handles broadcast messaging feature
void handle_create_group(char *message, int &client_fd);                        // handles creating a new group feature
void handle_join_group(char *message, int &client_fd, std::string &username);   // handles join group feature
void handle_leave_group(char *message, int &client_fd, std::string &username);  // handles leave group feature
void handle_group_msg(char *message, int &client_fd, string &username);         // handles group messaging feature
void handle_exit(string &username, int &client_fd);                             // handles client exit feature
void handle_sigint(int sig);                                                    // handles abrupt server shutdown

// Additional functions
void handle_list_all_members(int &client_fd);                                   // lists all members active on the server
void handle_list_all_groups(int &client_fd);                                    // lists all groups active on the server
void handle_list_group_members(char *message, int &client_fd);                  // lists all active members of the requested group
void handle_help(int &client_fd);                                               // prints a help message for usage 

int main()
{
    signal(SIGINT, handle_sigint);

    // populate user to password map (auth)
    ifstream file("users.txt");
    if (!file)
    {
        perror("Error opening file.");
    }

    string line;
    while (getline(file, line))
    {
        size_t pos = line.find(':');
        if (pos != string ::npos)
        {
            string username = line.substr(0, pos);
            string password = line.substr(pos + 1);
            auth[username] = password;
        }
    }

    // creating server socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        perror("Socket creation failed.");
        return 0;
    }

    // setting socket options
    int opt = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(sock_fd);
        return 0;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // binding socket to port
    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(sock_fd);
        perror("Socket binding error.");
    }

    // listening on server socket
    if (listen(sock_fd, 10) < 0)
    {
        close(sock_fd);
        perror("listing failed.");
    }
    cout << "\033[32mTCP server listing on PORT : \033[93m" << PORT << "\033[0m" << endl;

    // accepting multiple clients
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock;
        client_sock = accept(sock_fd, (struct sockaddr *)(&client_addr), &client_len);
        if (client_sock < 0)
        {
            perror("Connection failed!");
            continue;
        }
        unique_lock<mutex> lock(mtx);
        client_set.insert(client_sock);
        lock.unlock();
        
        // thread to handle activities of that client
        thread client_thread(client_handle, client_sock);
        client_thread.detach();
    }
    return 0;
}

void client_handle(int client_fd)
{
    const char *wel_msg = "Welcome to Shadow Room!\nEnter your username: ";
    send(client_fd, wel_msg, strlen(wel_msg), 0);
    char username_buff[BUFF_SZ] = {0};
    char passwd_buff[BUFF_SZ] = {0};
    string username;
    if (recv(client_fd, username_buff, BUFF_SZ, 0) <= 0) // receive username
    {
        close(client_fd);
        perror("TCP receive failed");
        return;
    }
    else
    {
        username = username_buff;
        const char *passwd = "Enter your password: ";
        send(client_fd, passwd, strlen(passwd), 0);
        if (recv(client_fd, passwd_buff, BUFF_SZ, 0) <= 0) // receive password
        { 
            close(client_fd);
            perror("TCP receive failed");
            return;
        }

        // Authenticate
        if (auth.find(username) != auth.end() && (string)passwd_buff == auth[username])
        {
            send(client_fd, banner, strlen(banner), 0);
            // Notify all the users
            thread broadcast_thread(broadcast, "\033[093m" + username + " has joined the chat!\033[0m", client_fd);
            broadcast_thread.detach();
        }
        else
        {
            const char *auth_failed = "Authentication failed.";
            send(client_fd, auth_failed, strlen(auth_failed), 0);
            close(client_fd);
            return;
        }
    }

    // update the data structures
    unique_lock<mutex> lock(mtx);
    userToSocket[username] = client_fd;
    socketToUser[client_fd] = username;
    lock.unlock();

    handle_help(client_fd); // print help/usage message

    while (1) // handle client actions
    {
        char msg[MSG_SZ] = {0};
        int bytes_received = recv(client_fd, msg, MSG_SZ, 0);

        // handle abrupt client shutdown
        if (bytes_received == 0)
        {
            // Client disconnected
            cout << username << " disconnected.\n";
            handle_exit(username, client_fd);
            return;
        }
        else if (bytes_received < 0)
        {
            perror("Error receiving data");
            handle_exit(username, client_fd);
            return;
        }

        // parse received message
        char *action = NULL;
        char *message = NULL;
        char *sep = strchr(msg, ' ');
        if (sep != NULL)
        {
            size_t len1 = sep - msg;
            action = (char *)malloc(len1 + 1);
            strncpy(action, msg, len1);
            action[len1] = '\0';
            message = strdup(sep + 1);
        }
        else
        {
            action = strdup(msg);
            message = strdup("");
        }

        if ((string)action == "/exit")
        {
            if (!isEmpty(message) || sep != NULL) // appropriate usage message for "/exit" function
            {
                const char *err_msg = "\033[93mUsage : /exit\n(Do not add any whitespace or other characters)\n\033[0m";
                send(client_fd, err_msg, strlen(err_msg), 0);
                continue;
            }
            handle_exit(username, client_fd);
            return;
        }
        else if ((string)action == "/msg") 
        {
            handle_msg(message, client_fd, username);
        }
        else if ((string)action == "/broadcast") 
        {
            handle_broadcast(username, message, client_fd);
        }
        else if ((string)action == "/create_group")
        {
            handle_create_group(message, client_fd);
        }
        else if ((string)action == "/join_group")
        {
            handle_join_group(message, client_fd, username);
        }
        else if ((string)action == "/leave_group")
        {
            handle_leave_group(message, client_fd, username);
        }
        else if ((string)action == "/group_msg")
        {
            handle_group_msg(message, client_fd, username);
        }
        else if ((string)action == "/list_all_members")
        {
            handle_list_all_members(client_fd);
        }
        else if ((string)action == "/list_all_groups")
        {
            handle_list_all_groups(client_fd);
        }
        else if ((string)action == "/list_group_members")
        {
            handle_list_group_members(message, client_fd);
        }
        else if ((string)action == "/help")
        {
            handle_help(client_fd);
        }
        else  // error if none of the above actions
        {
            const char *err_msg = "\033[31mError : Invalid Action.\033[0m";
            send(client_fd, err_msg, strlen(err_msg), 0);
            continue;
        }
    }
}

void handle_msg(char *message, int client_fd, std::string &username)
{
    // parse message to extract username
    char recpt[BUFF_SZ];
    int j = 0;
    while (message[j] != '\0' && message[j] != ' ')
    {
        recpt[j] = message[j];
        j++;
    }
    recpt[j] = '\0';


    if (isEmpty(recpt)) // Send usage message  if username is empty
    {
        const char *err_msg = "\033[93mUsage : /msg <recipient_username> <message>\033[0m";
        send(client_fd, err_msg, strlen(err_msg), 0);
    }
    else if (userToSocket.find((string)recpt) == userToSocket.end()) // Send error message if username does not exist in the chat
    {
        const char *err_msg = "\033[31mError : Recipient not found in network.\033[0m";
        send(client_fd, err_msg, strlen(err_msg), 0);
    }
    else // Send the message if everything is correct
    {
        int recv_fd = userToSocket[(string)recpt];
        thread private_thread(private_mssg, (string)("[" + username + "]: " + message), recv_fd);
        private_thread.detach();
    }
}

void handle_broadcast(std::string &username, char *message, int &client_fd)
{
    // Create a thread to broadcast the message to all members except that client
    thread broadcast_thread(broadcast, (string)("[" + username + " on broadcast]: " + message), client_fd);
    broadcast_thread.detach();
}

void handle_create_group(char *message, int &client_fd)
{
    // parse the message to extract group name
    // group name does not contain whitespaces
    char group_name[BUFF_SZ];
    int j = 0;
    while (message[j] != '\0' && message[j] != ' ')
    {
        group_name[j] = message[j];
        j++;
    }
    group_name[j] = '\0'; 

    if (isEmpty(group_name)) // Send usage message if group name is empty
    {
        const char *err_msg = "\033[93mUsage : /create_group <group_name>\033[0m";
        send(client_fd, err_msg, strlen(err_msg), 0);
    }
    else if (groupToMembers.find((string)group_name) != groupToMembers.end()) // Send error message if group already exists
    {
        const char *err_msg = "\033[31mError : This group already exists!\033[0m";
        send(client_fd, err_msg, strlen(err_msg), 0);
    }
    else // Create the group and update groupToMembers map 
    {
        unique_lock<mutex> lock(mtx);
        groupToMembers[group_name] = {socketToUser[client_fd]};
        lock.unlock();
        const char *server_msg = "\033[93mGroup created.\033[0m";
        send(client_fd, server_msg, strlen(server_msg), 0);
    }
}

void handle_join_group(char *message, int &client_fd, string &username)
{
    // parse the message to extract group name
    char group_name[BUFF_SZ];
    int j = 0;
    while (message[j] != '\0' && message[j] != ' ')
    {
        group_name[j] = message[j];
        j++;
    }
    group_name[j] = '\0';

    if (isEmpty(group_name)) // Send usage message if group name is empty
    {
        const char *err_msg = "\033[93mUsage : /join_group <group_name>\033[0m";
        send(client_fd, err_msg, strlen(err_msg), 0);
    }
    else if (groupToMembers.find((string)group_name) == groupToMembers.end()) // Send error message if group does not exist
    {
        const char *err_msg = "\033[31mError : This group does not exists!\033[0m";
        send(client_fd, err_msg, strlen(err_msg), 0);
    }
    else if (groupToMembers[group_name].find(socketToUser[client_fd]) != groupToMembers[group_name].end()) // Send error message if the client is already in that group
    {
        const char *server_msg = "\033[93mYou are already in this group.\033[0m";
        send(client_fd, server_msg, strlen(server_msg), 0);
    }
    else  // add the client to that group and notify all existing members of that group about the new member 
    {
        unique_lock<mutex> lock(mtx);
        groupToMembers[group_name].insert(socketToUser[client_fd]);
        lock.unlock();
        string server_msg = "\033[93mYou joined " + (string)group_name + ".\033[0m";
        thread send_thread(send_message, client_fd, server_msg);
        send_thread.detach();
        server_msg = "\033[93m" + username + " joined " + (string)group_name + ".\033[0m";
        thread group_mssg_thread(group_mssg, server_msg, (string)group_name, client_fd);
        group_mssg_thread.detach();
    }
}

void handle_leave_group(char *message, int &client_fd, string &username)
{
    // parse the message to extract group name
    char group_name[BUFF_SZ];
    int j = 0;
    while (message[j] != '\0' && message[j] != ' ')
    {
        group_name[j] = message[j];
        j++;
    }
    group_name[j] = '\0';
    if (isEmpty(group_name)) // Send usage message if group name is empty
    {
        const char *err_msg = "\033[93mUsage : /leave_group <group_name>\033[0m";
        send(client_fd, err_msg, strlen(err_msg), 0);
    }
    else if (groupToMembers.find((string)group_name) == groupToMembers.end()) // Send error message if group does not exist
    {
        const char *err_msg = "\033[31mError : This group does not exists!\033[0m";
        send(client_fd, err_msg, strlen(err_msg), 0);
    }
    else if (groupToMembers[group_name].find(socketToUser[client_fd]) == groupToMembers[group_name].end()) // Send error message if the client is not in that group
    {
        const char *server_msg = "\033[31mError : You are not in this group.\033[0m";
        send(client_fd, server_msg, strlen(server_msg), 0);
    }
    else // remove the client from that group and notify all existing members of that group about the exit member
    {
        unique_lock<mutex> lock(mtx);
        groupToMembers[group_name].erase(socketToUser[client_fd]);
        lock.unlock();
        string server_msg = "\033[93mYou left " + (string)group_name + ".\033[0m";
        thread send_thread(send_message, client_fd, server_msg);
        send_thread.detach();
        server_msg = "\033[93m" + username + " left " + (string)group_name + ".\033[0m";
        thread group_mssg_thread(group_mssg, server_msg, (string)group_name, client_fd);
        group_mssg_thread.detach();
    }
}

void handle_group_msg(char *message, int &client_fd, string &username)
{
    // parse the message to extract group name
    char group_name[BUFF_SZ];
    int j = 0;
    while (message[j] != '\0' && message[j] != ' ')
    {
        group_name[j] = message[j];
        j++;
    }
    group_name[j] = '\0';

    // parse the message to extract message body
    char message_body[MSG_SZ];
    int k = 0;
    while (message[j] != '\0')
    {
        message_body[k++] = message[j++];
    }
    message_body[k] = '\0';

    if (isEmpty(group_name)) // Send usage message if group name is empty
    {
        const char *err_msg = "\033[93mUsage : /group_msg <group_name> <message>\033[0m";
        send(client_fd, err_msg, strlen(err_msg), 0);
    }
    else if (groupToMembers.find((string)group_name) == groupToMembers.end()) // Send error message if group does not exist
    {
        const char *err_msg = "\033[31mError : This group does not exist!\033[0m";
        send(client_fd, err_msg, strlen(err_msg), 0);
    }
    else // Create a thread to send the message to all members of the group except that client
    {
        string group_msg = "[" + username + " on Group " + (string)group_name + "]: " + (string)message_body;
        thread group_mssg_thread(group_mssg, group_msg, (string)group_name, client_fd);
        group_mssg_thread.detach();
    }
}

void handle_exit(string &username, int &client_fd)
{
    // update the data structures to remove that client
    unique_lock<mutex> lock(mtx);
    userToSocket.erase(userToSocket.find(username));
    socketToUser.erase(socketToUser.find(client_fd));

    client_set.erase(client_fd);

    // remove that client from the groups it was joined in
    for (auto &[group_name, members] : groupToMembers)
    {
        if (members.find(username) != members.end())
        {
            string server_msg = "\033[93m" + username + " left " + (string)group_name + ".\033[0m";
            thread group_mssg_thread(group_mssg, server_msg, (string)group_name, client_fd);
            group_mssg_thread.detach();
        }
    }
    close(client_fd);
    lock.unlock();
    
    // Notify all clients about that client leaving
    thread broadcast_thread(broadcast, "\033[093m" + username + " has left the chat! \033[0m", client_fd);
    broadcast_thread.detach();
    return;
}

void handle_list_all_members(int &client_fd)
{
    // print list of all clients active on the server
    stringstream ss;
    ss << "\033[93m";
    unique_lock<mutex> lock(mtx);
    for (auto &[username, socket] : userToSocket)
    {
        ss << username << "\n";
    }
    lock.unlock();
    ss << "\033[0m";
    string user_list = ss.str();
    send_message(client_fd, user_list);
}

void handle_list_all_groups(int &client_fd)
{
    // print list of all groups active on the server
    stringstream ss;
    ss << "\033[93m";
    unique_lock<mutex> lock(mtx);
    for (auto &[group_name, members] : groupToMembers)
    {
        ss << group_name << "\n";
    }
    lock.unlock();
    ss << "\033[0m";
    string group_list = ss.str();
    send_message(client_fd, group_list);
}

void handle_list_group_members(char *message, int &client_fd)
{
    // print list of all members of a particular group on the server
    char group_name[BUFF_SZ];
    int j = 0;
    while (message[j] != '\0' && message[j] != ' ')
    {
        group_name[j] = message[j];
        j++;
    }
    group_name[j] = '\0';

    if (isEmpty(group_name)) // Send usage message if group name is empty
    {
        const char *err_msg = "\033[93mUsage : /list_group_members <group_name>\033[0m";
        send(client_fd, err_msg, strlen(err_msg), 0);
    }
    else if (groupToMembers.find((string)group_name) == groupToMembers.end()) // Send error message if group does not exist
    {
        const char *err_msg = "\033[31mError : This group does not exist!\033[0m";
        send(client_fd, err_msg, strlen(err_msg), 0);
    }
    else
    {
        stringstream ss;
        unique_lock<mutex> lock(mtx);
        for (auto &member : groupToMembers[group_name])
        {
            ss << member << "\n";
        }
        lock.unlock();
        string member_list = ss.str();
        send_message(client_fd, member_list);

    }
}

void handle_help(int &client_fd)
{
    // print help message 
    stringstream ss;
    ss << "\033[32mList of availabe actions : \033[0m\n"
       << "\033[93m/msg <recipient's username> <message>\033[0m\tSend a private message to another user in the chat\n"
       << "\033[93m/broadcast <message>\033[0m\t\t\tSend a message to all the users in the chat\n"
       << "\033[93m/create_group <group_name>\033[0m\t\tCreate a group for messaging\n"
       << "\033[93m/join_group <group_name>\033[0m\t\tJoin an existing group\n"
       << "\033[93m/group_msg <group_name> <message>\033[0m\tSend a message to all the members of the group\n"
       << "\033[93m/leave_group <group_name>\033[0m\t\tLeave a group\n"
       << "\033[93m/list_all_members\033[0m\t\t\tPrint a list of all members present in the chat\n"
       << "\033[93m/list_all_groups\033[0m\t\t\tPrint a list of all groups in the chat\n"
       << "\033[93m/list_group_members <group_name>\033[0m\tPrint a list of all members in a group\n"
       << "\033[93m/help\033[0m\t\t\t\t\tPrint this help message\n"
       << "\033[93m/exit\033[0m\t\t\t\t\tExit the chat\n";
    string help_msg = ss.str();
    send_message(client_fd, help_msg);
}

void handle_sigint(int sig) // handle abrupt shutdown
{
    printf("\nCaught signal %d (SIGINT). Shutting down gracefully...\n", sig);
    unique_lock<mutex> lock(mtx);
    // Close the server socket
    for (auto fd : client_set)
    {
        close(fd);
    }
    lock.unlock();
    if (sock_fd != -1)
    {
        close(sock_fd);
        printf("Server socket closed.\n");
    }

    // Perform other cleanup if necessary
    printf("Server shutdown complete.\n");
    exit(0); // Exit the program
}

bool isEmpty(const char str[]) // returns true if a string is empty or has just whitespaces, else false
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        if (!isspace(str[i]))
        {
            return false;
        }
    }
    return true;
}

void send_message(int client_sock, string message) // send message to a particular client socket
{
    send(client_sock, message.c_str(), message.length(), 0);
}

void group_mssg(string message, string group_name, int client_fd) // send message to all members of a group except the sending client
{
    unique_lock<mutex> lock(mtx);
    for (auto it = groupToMembers[group_name].begin(); it != groupToMembers[group_name].end(); ++it)
    {
        if (userToSocket[*it] != client_fd)
        {
            thread client_thread(send_message, userToSocket[*it], message);
            client_thread.detach();
        }
    }
    lock.unlock();
}

void private_mssg(string message, int recv_fd) // send message to a particular client 
{ 
    unique_lock<mutex> lock(mtx);
    thread pvt_mssg_thread(send_message, recv_fd, message);
    pvt_mssg_thread.detach();
    lock.unlock();
}

void broadcast(string message, int broadcast_fd) // send message to all active members on the server except the sending client
{
    unique_lock<mutex> lock(mtx);
    for (auto it = userToSocket.begin(); it != userToSocket.end(); ++it)
    {
        if (it->second != broadcast_fd)
        {
            thread client_thread(send_message, it->second, message);
            client_thread.detach();
        }
    }
    lock.unlock();
}
