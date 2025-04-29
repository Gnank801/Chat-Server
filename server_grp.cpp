#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

using namespace std;

#define PORT 12345
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 1000

// Helper function to trim whitespace (spaces, tabs, and newline characters)
string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Server state
unordered_map<int, string> clients;               // socket -> username
unordered_map<string, string> users;                // username -> password
unordered_map<string, unordered_set<int>> groups;   // group -> sockets
mutex clients_mutex, groups_mutex;

void load_users() {
    ifstream file("users.txt");
    if (!file.is_open()) {
        cerr << "Failed to open users.txt" << endl;
        exit(EXIT_FAILURE);
    }
    
    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string username, password;
        if (getline(iss, username, ':') && getline(iss, password)) {
            // Also trim the password (or username) if necessary.
            users[trim(username)] = trim(password);
        }
    }
    file.close();
}

void send_message(int socket, const string &message) {
    send(socket, message.c_str(), message.length(), 0);
}

void broadcast_message(const string &message, int sender) {
    lock_guard<mutex> lock(clients_mutex);
    for (const auto &client : clients) {
        if (client.first != sender) {
            send_message(client.first, message);
        }
    }
}

// Modified to take sender's socket so we can send back an error if needed.
void private_message(int sender_socket, const string &sender, const string &recipient, const string &message) {
    lock_guard<mutex> lock(clients_mutex);
    bool found = false;
    for (const auto &client : clients) {
        if (client.second == recipient) {
            send_message(client.first, "[Private] " + sender + ": " + message + "\n");
            found = true;
            break;
        }
    }
    if (!found) {
        send_message(sender_socket, "User '" + recipient + "' not found\n");
    }
}

void create_group(int client_socket, const string &group_name_input) {
    string group_name = trim(group_name_input);
    if (group_name.empty()){
        send_message(client_socket, "Group name cannot be empty or whitespace only.\n");
        return;
    }
    lock_guard<mutex> lock(groups_mutex);
    if (groups.count(group_name)) {
        send_message(client_socket, "Group '" + group_name + "' already exists\n");
    } else {
        groups[group_name].insert(client_socket);
        send_message(client_socket, "Group '" + group_name + "' created & joined\n");
    }
}

void join_group(int client_socket, const string &group_name_input) {
    string group_name = trim(group_name_input);
    if (group_name.empty()){
        send_message(client_socket, "Group name cannot be empty or whitespace only.\n");
        return;
    }
    lock_guard<mutex> lock(groups_mutex);
    if (!groups.count(group_name)) {
        send_message(client_socket, "[Error]"" Group '" + group_name + "' doesn't exist\n");
        return;
    }
    groups[group_name].insert(client_socket);
    send_message(client_socket, "You joined the group '" + group_name + "'\n");
}

void leave_group(int client_socket, const string &group_name_input) {
    string group_name = trim(group_name_input);
    if (group_name.empty()){
        send_message(client_socket, "Group name cannot be empty or whitespace only.\n");
        return;
    }
    lock_guard<mutex> lock(groups_mutex);
    if (!groups.count(group_name) || !groups[group_name].count(client_socket)) {
        send_message(client_socket, "Not in group '" + group_name + "'\n");
        return;
    }
    groups[group_name].erase(client_socket);
    send_message(client_socket, "Left group '" + group_name + "'\n");
}

void group_message(int sender_socket, const string &group_name_input, const string &message_input) {
    string group_name = trim(group_name_input);
    string message = trim(message_input);
    if (message.empty()){
        send_message(sender_socket, "Group message cannot be empty.\n");
        return;
    }
    // Lock both groups and clients mutexes
    lock_guard<mutex> g_lock(groups_mutex);
    lock_guard<mutex> c_lock(clients_mutex);
    
    if (!groups.count(group_name)) {
        send_message(sender_socket, "Group '" + group_name + "' doesn't exist\n");
        return;
    }
    
    if (!groups[group_name].count(sender_socket)) {
        send_message(sender_socket, "You're not in group '" + group_name + "'\n");
        return;
    }
    
    const string &sender = clients.at(sender_socket);
    string formatted = "[Group " + group_name + "] " + sender + ": " + message + "\n";
    
    for (int member : groups[group_name]) {
        if (member != sender_socket) {
            send_message(member, formatted);
        }
    }
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    string username;

    // Authentication
    send_message(client_socket, "Username: ");
    int bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) { 
        close(client_socket); 
        return; 
    }
    username = trim(string(buffer, bytes));

    send_message(client_socket, "Password: ");
    bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) { 
        close(client_socket); 
        return; 
    }
    string password = trim(string(buffer, bytes));

    if (!users.count(username) || users[username] != password) {
        send_message(client_socket, "Authentication failed\n");
        close(client_socket);
        return;
    }

    {
        lock_guard<mutex> lock(clients_mutex);
        clients[client_socket] = username;
    }

    send_message(client_socket, "\nWelcome to the Chat Server!\n"
                  "(use /help for commands)");
    
    broadcast_message("*** " + username + " joined the chat ***\n", client_socket);
    

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0)
            break;

        // Trim the incoming message to remove extra spaces/newlines
        string msg = trim(string(buffer, bytes));
        if (msg.empty())
            continue;

        if (msg.rfind("/broadcast ", 0) == 0) {
            string content = trim(msg.substr(11));
            broadcast_message("[All] " + username + ": " + content + "\n", client_socket);
        }
        else if (msg.rfind("/msg ", 0) == 0) {
            size_t space = msg.find(' ', 5);
            if (space != string::npos) {
                string recipient = trim(msg.substr(5, space - 5));
                string content = trim(msg.substr(space + 1));
                private_message(client_socket, username, recipient, content);
            }
        }
        else if (msg.rfind("/create_group ", 0) == 0) {
            string groupName = msg.substr(14);
            create_group(client_socket, groupName);
        }
        else if (msg.rfind("/join_group ", 0) == 0) {
            string groupName = msg.substr(12);
            join_group(client_socket, groupName);
        }
        else if (msg.rfind("/leave_group ", 0) == 0) {
            string groupName = msg.substr(13);
            leave_group(client_socket, groupName);
        }
        else if (msg.rfind("/group_msg ", 0) == 0) {
            size_t space = msg.find(' ', 11);
            if (space != string::npos) {
                string group = msg.substr(11, space - 11);
                string content = msg.substr(space + 1);
                group_message(client_socket, group, content);
            }
        } else if(msg.rfind("/help", 0) == 0) {
            send_message(client_socket,
                  "/broadcast <msg>\n"
                  "/msg <user> <msg>\n"
                  "/create_group <name>\n"
                  "/join_group <name>\n"
                  "/leave_group <name>\n"
                  "/group_msg <group> <msg>\n"
                  "/exit\n");
        }
        else {
            send_message(client_socket, "Unknown command or Error in the command. Use /help for commands\n");
        }
    }

    // Cleanup on disconnect
    {
        lock_guard<mutex> lock(clients_mutex);
        clients.erase(client_socket);
    }
    {
        lock_guard<mutex> lock(groups_mutex);
        for (auto &group : groups) {
            group.second.erase(client_socket);
        }
    }
    broadcast_message("** " + username + " left the chat server **\n", client_socket);
    close(client_socket);
}

int main() {
    load_users();
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        cerr << "Socket creation failed" << endl;
        return EXIT_FAILURE;
    }

    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Use the global bind (to avoid collisions with std::bind)
    if (::bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "Bind failed" << endl;
        return EXIT_FAILURE;
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        cerr << "Listen failed" << endl;
        return EXIT_FAILURE;
    }

    cout << "Server running on port " << PORT << endl;
    while (true) {
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (sockaddr*)&client_addr, &addr_len);
        
        if (client_socket < 0) {
            cerr << "Accept error" << endl;
            continue;
        }
        
        // Handle each client in a new thread
        thread(handle_client, client_socket).detach();
    }

    close(server_fd);
    return EXIT_SUCCESS;
}
