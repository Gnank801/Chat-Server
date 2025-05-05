# Chat Server

## Introduction

This project challenged us to build a lightweight chat server that allows multiple users to connect and interact in real time. Our server supports user authentication, broadcast messages, private messages, and group chats. Throughout this project, we learned a lot about socket programming, multithreading, and the importance of careful design decisions.

## Features

### What We Built

- **User Authentication:**  
  Users log in with a unique username and password (stored in a `users.txt` file). Only registered users can join the conversation.

- **Broadcast Messaging:**  
  Using the `/broadcast <msg>` command, a user can send a message to all connected clients.

- **Private Messaging:**  
  With the `/msg <user> <msg>` command, you can send a direct message to someone without others knowing.

- **Group Chats:**  
  - **Create a Group:** Use `/create_group <name>` to start a new chat group.
  - **Join or Leave a Group:** Join with `/join_group <name>` and leave with `/leave_group <name>`.
  - **Group Messaging:** Chat with everyone in a group using `/group_msg <group> <msg>`.
  - **Helper Commands:** Get the commands list using `/help`.
  - **Exiting the server:** Exit the server using `/exit`.

- **Error Handling:** 
  An error message is generated if the command is typed incorrectly or invalid.

- **Input Validation:**  
  We’ve added input checks so that messages or group names filled with only spaces aren’t accepted. A handy `trim()` function cleans up the input for you!

- **Thread-per-Connection:**  
  Each new connection spawns its own thread. This means that multiple people can chat at the same time without interfering with each other.

### What We Didn’t Implement

- **Chat History:**  
  The server doesn’t store previous messages. Once you disconnect, your conversation history is gone.

- **Encryption/Security Enhancements:**  
  All communication happens in plain text. We didn’t add encryption like SSL/TLS in this project.

## Design Decisions

### Why Threads?

We chose to handle each client connection with a new thread rather than using separate processes. Threads are lightweight, and since they share memory, it was easier to manage the list of connected clients and groups. This design helped keep the code simple and efficient.

### Synchronization

Because many threads access shared data (like our clients and groups lists), we use mutexes to prevent race conditions. This ensures that when two threads try to modify the same data, they do it one at a time, keeping everything in order.

### Handling Function Name Collisions

We encountered a small issue where `std::bind` and the system call `bind` had the same name. This caused confusion about which one was being called. We fixed it by using `::bind`, making sure we called the correct function. A small change, but it made a big difference!

### Input Validation

To avoid issues like empty messages or groups with names that are just spaces, we wrote a helper function called `trim()`. This makes sure all inputs are clean before processing, improving the overall user experience.

## Implementation Details

### Code Structure

- **`load_users()`:**  
  Reads the `users.txt` file and loads user credentials into memory for login authentication.

- **`send_message()`:**  
  A simple wrapper around the socket’s `send()` function. It sends messages to a client.

- **`broadcast_message()`:**  
  Loops through all clients (except the sender) and sends them the broadcast message.

- **`private_message()`:**  
  Searches for a recipient by username and sends a direct message. If the recipient isn’t found, it notifies the sender.

- **Group Management Functions:**  
  Functions to create, join, leave, and send messages to groups keep group chats organized.

- **`handle_client()`:**  
  This is the heart of our project—it handles a client’s lifecycle from authentication through message processing to eventual disconnection and cleanup.

### Code Flow Diagram
    [Server Startup]
            │
            ▼
    [Load Users File]
            │
            ▼
    [Create, Bind, and Listen on Socket]
            │
            ▼
    [Accept Incoming Connections]
            │
            ▼
    [Spawn a New Thread for Each Client]
            │
            ▼
    [Handle Client Commands & Messaging]
            │
            ▼
    [Client Disconnects & Cleanup]

## Example Code Output

 - running the server executable in one terminal
```
./server_grp                                        
Server running on port 12345
```

 - Running the client2 executable in one terminal
```
./client_grp                                        
Connected to the server.
Username: alice
Password: password123

Welcome to the Chat Server!
(use /help for commands)
/help
/broadcast <msg>
/msg <user> <msg>
/create_group <name>
/join_group <name>
/leave_group <name>
/group_msg <group> <msg>
/exit

*** eve joined the chat ***

*** frank joined the chat ***

/broadcast hiall
[Private] eve: lets make a group

/create_group assign1
Group 'assign1' created & joined

/msg frank join assign1
/msg eve join assign1
/group_msg assign1 hii     all 
** frank left the chat server **

** eve left the chat server **

/exit
Disconnected from server.
```

 - Running the client3 executable in one terminal
```
./client_grp   
Connected to the server.
Username: eve
Password: trustno1

Welcome to the Chat Server!
(use /help for commands)
*** frank joined the chat ***

[All] alice: hiall

/msg alice lets make a group
[Private] alice: join assign1

/join_group assign2
[Error] Group 'assign2' doesn't exist

/join_group assign1
You joined the group 'assign1'

[Group assign1] alice: hii     all

/leave_group assign1
Left group 'assign1'

** frank left the chat **

/exit
Disconnected from server.
```

 - Running the client1 executable in one terminal
```
./client_grp   
Connected to the server.
Username: frank
Password: letmein

Welcome to the Chat Server!
(use /help for commands)
[All] alice: hiall

[Private] alice: join assign1

/join_group assign1
You joined the group 'assign1'

[Group assign1] alice: hii     all

/exit
Disconnected from server.
```


## Testing

### How We Tested

- **Correctness Testing:**  
  We manually tested out all the commands (broadcast, private, group messages) to make sure everything worked as expected.

- **Stress Testing:**  
  We simulated multiple clients connecting at the same time to see how well our server handled concurrent activity. This helped us ensure that our use of mutexes and threading kept everything stable.

## Server Restrictions

- **Maximum Clients:**  
  Our server supports up to **128 simultaneous connections**.

- **Maximum Groups:**  
  There isn’t a strict limit on the number of groups—this is only limited by the available system resources.

- **Maximum Members per Group:**  
  There’s no explicit cap; any number of clients can join a group as long as they’re connected.

- **Message Size:**  
  Each message can be up to **1024 bytes** long (set by `BUFFER_SIZE`).

## Challenges and Learnings

We faced some real challenges along the way:

- **Concurrency and Synchronization:**  
  Managing shared resources between multiple threads was tricky. We learned the importance of using mutexes to avoid data corruption.

- **Namespace Collisions:**  
  The clash between `std::bind` and the system’s `bind` was a subtle bug that taught us to be mindful of naming conflicts.

- **Input Validation:**  
  Ensuring that we reject empty or whitespace-only inputs required extra attention and testing. The `trim()` function turned out to be an essential part of our solution.

Overall, every challenge was a learning opportunity, and we’re proud of the robust and user-friendly solution we developed.

## Contributions

- **Rachana (34%):**  
  Led the overall design and architecture, especially the threading.

- **Gnan (33%):**  
  Implemented the core functionality—authentication, messaging, synchronization part.

- **Mithun (33%):**  
  Assisted with testing,group management,input validation, debugging, and helped draft this README.


## Declaration

We hereby declare that this project is our original work, and we have not engaged in any plagiarism.

## Feedback

Working on this project was a unique learning experience. We enjoyed the challenge of building a multithreaded chat server from scratch, and it let us valuable lessons about design, synchronization, and debugging. For future projects, more detailed stress testing guidelines would be helpful.

