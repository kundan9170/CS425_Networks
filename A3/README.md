# TCP Three-Way Handshake Implementation

## Overview
This project implements a simplified version of the TCP three-way handshake using raw sockets in C++. The server (`server.cpp`) is provided, and the task was to implement the client (`client.cpp`). The client initiates the handshake by sending a SYN packet, waits for a SYN-ACK response from the server, and completes the handshake by sending a final ACK packet.

---

## Files
1. **`server.cpp`**:
   - Provided as part of the assignment.
   - Listens for incoming SYN packets from the client.
   - Responds with a SYN-ACK packet.
   - Waits for the final ACK packet to complete the handshake.

2. **`client.cpp`**:
   - Written as part of the assignment.
   - Creates two threads: a sender thread and a receiver thread. 
   - Sender thread sends a SYN packet to the server.
   - It implements timeout and retry logic for the SYN packet.
   - Receiver thread waits for a SYN-ACK response from the server.
   - It handles duplicate SYN-ACK responses.
   - Client sends a final ACK packet to complete the handshake.

---

## How It Works
### Client (`client.cpp`):
1. Creates a raw socket with the `IP_HDRINCL` option to manually construct IP and TCP headers.
2. Sends a SYN packet to the server.
3. Waits for a SYN-ACK response using a separate receiver thread.
4. If no SYN-ACK is received within a timeout, the SYN packet is resent (up to a maximum number of retries).
5. Once the SYN-ACK is received, the client sends a final ACK packet to complete the handshake.

### Server (`server.cpp`):
1. Listens for incoming SYN packets on a raw socket.
2. Responds to SYN packets with a SYN-ACK packet.
3. Waits for the final ACK packet to confirm the handshake.

---

## Features
- **Timeout and Retry Logic**:
  - The client resends the SYN packet if no SYN-ACK is received within the specified timeout (`TIMEOUT`).
  - The number of retries is limited by `MAX_TRIALS`.

- **Multithreading**:
  - The client uses two threads:
    1. A sender thread to send the SYN packet and handle retries.
    2. A receiver thread to listen for the SYN-ACK response.

- **Duplicate SYN-ACK Handling**:
  - The client ignores duplicate SYN-ACK packets to ensure proper handshake completion.

---

## Compilation and Execution
### Compile the Server:
Use the makefile:
```bash
make
```
OR
```bash
g++ -o server server.cpp
g++ -o client client.cpp
```
### Run the server and client:
```bash
sudo ./server
```
```bash
sudo ./client
```

## Team Contributors
- Dhruv Gupta (220361) [**`33.33%`**]
- Kundan Kumar (220568) [**`33.33%`**]
- Pragati Agrawal (220779) [**`33.33%`**]

All three of us contributed equally to the assignment.
We collaboratively discussed the high level design of the client code. The README was also written with equal contribution from all of us.

## Sources Referred
- RFC 793 (https://www.rfc-editor.org/rfc/rfc793)
- Linux Raw Sockets Documentation (https://man7.org/linux/man-pages/man7/raw.7.html)

## Declaration
We hereby declare that this assignment, tilted **Assignment 3 : TCP Handshake** is our original work. We have not engaged in any form of plagiarism, cheating, or unauthorized collaboration. All sources of information and references used have been properly cited.

## Feedback
- The assignment helped us understand 3-way TCP handshake better.

### _Thank You!_