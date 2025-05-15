#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread> // For thread
#include <atomic> // For atomic
#include <chrono> // For chrono
using namespace std;

// --- Configuration ---
#define SERVER_IP "127.0.0.1" // IP address of the server
#define SERVER_PORT 12345     // Port the server is listening on
#define CLIENT_IP "127.0.0.1" // Source IP address for the client packets
#define CLIENT_PORT 54321     // Source port for the client packets (can be any ephemeral port)

#define MAX_TRIALS 5 // Maximum number of SYN retries
#define TIMEOUT 10    // Timeout in seconds for SYN-ACK response

// Global variables for communication between threads
atomic<bool> syn_ack_received(false);
atomic<uint32_t> server_seq_num(0);
int sock;
struct sockaddr_in server_addr;
char packet[sizeof(struct iphdr) + sizeof(struct tcphdr)];
char recv_buffer[65536];

// Function to send the SYN packet with timeout and retries
void send_syn() {
    cout << "[+] Constructing SYN packet..." << endl;

    int max_trials = MAX_TRIALS; // Maximum number of retries
    int trial_number = 0;
    auto start_time = chrono::steady_clock::now();

    while (!syn_ack_received && trial_number < max_trials) {
        memset(packet, 0, sizeof(packet)); // Zero out the packet buffer

        // Get pointers to IP and TCP headers within the packet buffer
        struct iphdr *iph = (struct iphdr *)packet;
        struct tcphdr *tcph = (struct tcphdr *)(packet + sizeof(struct iphdr));

        // ** Fill IP Header **
        iph->ihl = 5; // Header Length: 5 * 32 bits = 20 bytes
        iph->version = 4; // IPv4
        iph->tos = 0; // Type of Service
        iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr)); // Total packet length
        iph->id = htons(rand() % 65535); // Packet ID (random)
        iph->frag_off = 0; // Fragmentation flags/offset
        iph->ttl = 64; // Time To Live
        iph->protocol = IPPROTO_TCP; // Protocol: TCP
        iph->check = 0; // Kernel will calculate checksum if IP_HDRINCL is set (usually)
        iph->saddr = inet_addr(CLIENT_IP); // Source IP address
        iph->daddr = server_addr.sin_addr.s_addr; // Destination IP address (from server_addr)

        // ** Fill TCP Header **
        tcph->source = htons(CLIENT_PORT); // Source Port
        tcph->dest = htons(SERVER_PORT);   // Destination Port (server's port)
        tcph->seq = htonl(200); // Sequence number
        tcph->ack_seq = htonl(0); // SYN packet has ACK number 0
        tcph->doff = 5;  // Data Offset: 5 * 32 bits = 20 bytes (TCP header size)
        tcph->syn = 1;   // SYN flag set
        tcph->ack = 0;   // No ACK flag
        tcph->window = htons(5840); // TCP window size
        tcph->check = 0; // Checksum: set to 0; kernel usually calculates for raw sockets

        // ** Send the SYN packet **
        cout << "[+] Sending SYN packet (SEQ=200) to " << SERVER_IP << ":" << SERVER_PORT << " (Trial " << trial_number + 1 << ")..." << endl;
        if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("[-] sendto() failed for SYN");
            close(sock);
            exit(EXIT_FAILURE);
        }
        cout << "[+] SYN packet sent." << endl;

        // Wait for timeout or SYN-ACK
        while (!syn_ack_received) {
            auto current_time = chrono::steady_clock::now();
            auto elapsed_time = chrono::duration_cast<chrono::seconds>(current_time - start_time).count();

            if (elapsed_time > TIMEOUT) { // Timeout
                trial_number++;
                start_time = chrono::steady_clock::now(); // Reset the timer
                break; // Resend the SYN packet
            }

            this_thread::sleep_for(chrono::milliseconds(100)); // Small delay to avoid busy-waiting
        }
    }

    if (!syn_ack_received) {
        cerr << "[-] SYN-ACK not received after " << max_trials << " trials. Exiting." << endl;
        close(sock);
        exit(EXIT_FAILURE);
    }
}

// Function to receive the SYN-ACK packet and handle duplicates
void receive_syn_ack() {
    cout << "[+] Waiting for SYN-ACK from server..." << endl;
    uint32_t expected_ack_seq = 201; // Our SYN seq (200) + 1
    uint32_t expected_seq_num = 400; // Server's initial sequence number 

    while (!syn_ack_received) {
        struct sockaddr_in recv_addr;
        socklen_t recv_addr_len = sizeof(recv_addr);
        memset(recv_buffer, 0, sizeof(recv_buffer));

        int bytes_received = recvfrom(sock, recv_buffer, sizeof(recv_buffer), 0,
                                      (struct sockaddr *)&recv_addr, &recv_addr_len);

        if (bytes_received < 0) {
            perror("[-] recvfrom() failed");
            continue; 
        }

        // Parse the received IP header
        struct iphdr *recv_iph = (struct iphdr *)recv_buffer;
        unsigned short ip_hdr_len = recv_iph->ihl * 4; // IP Header Length

        // Check if it's a TCP packet
        if (recv_iph->protocol != IPPROTO_TCP) {
            continue; // Ignore non-TCP packets
        }

        // Parse the received TCP header
        struct tcphdr *recv_tcph = (struct tcphdr *)(recv_buffer + ip_hdr_len);

        // --- Filter for the specific SYN-ACK we expect ---
        if (recv_iph->saddr == server_addr.sin_addr.s_addr &&         // Source IP
            ntohs(recv_tcph->source) == SERVER_PORT &&       // Source Port
            ntohs(recv_tcph->dest) == CLIENT_PORT &&         // Dest Port
            recv_tcph->syn == 1 &&                                    // SYN flag
            recv_tcph->ack == 1 &&                                    // ACK flag
            ntohl(recv_tcph->ack_seq) == expected_ack_seq &&          // Correct ACK number
            ntohl(recv_tcph->seq) == expected_seq_num)                // Correct SEQ number
        {
            if (syn_ack_received) {
                cout << "[/] Duplicate SYN-ACK received. Ignoring." << endl;
                continue;
            }

            cout << "[+] Received SYN-ACK from server." << endl;
            cout << "    Server SEQ: " << ntohl(recv_tcph->seq) << endl;
            cout << "    Server ACK: " << ntohl(recv_tcph->ack_seq) << endl;
            server_seq_num = ntohl(recv_tcph->seq); // Store server's sequence number
            syn_ack_received = true; // Notify that SYN-ACK is received
        }
        else {
            // Print details of ignored packets
           if (recv_iph->saddr == server_addr.sin_addr.s_addr && ntohs(recv_tcph->dest) == CLIENT_PORT) {
               cout << "[/] Received packet from server but not the expected SYN-ACK:" << endl;
               cout << "    Src Port: " << ntohs(recv_tcph->source) << " Dst Port: " << ntohs(recv_tcph->dest) << endl;
               cout << "    SEQ: " << ntohl(recv_tcph->seq) << " ACK: " << ntohl(recv_tcph->ack_seq) << endl;
               cout << "    Flags: SYN=" << recv_tcph->syn << " ACK=" << recv_tcph->ack << endl;
               cout << "    Expected SEQ: " << expected_seq_num << " Expected ACK: " << expected_ack_seq << endl;
               cout << "    Ignoring..." << endl;
               cout << endl;
           }
           
       }
    }
}

int main() {
    // Seed random number generator (used for IP ID)
    srand(time(nullptr));

    // --- Step 0: Create Raw Socket ---
    cout << "[+] Creating raw socket..." << endl;
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0) {
        perror("[-] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // --- Step 1: Enable IP_HDRINCL ---
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("[-] setsockopt(IP_HDRINCL) failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    cout << "[+] Raw socket created and IP_HDRINCL set." << endl;

    // --- Step 2: Prepare Server Address ---
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("[-] Invalid server IP address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // --- Step 3: Create threads for sending SYN and receiving SYN-ACK ---
    thread sender_thread(send_syn);
    thread receiver_thread(receive_syn_ack);

    // Wait for both threads to complete
    sender_thread.join();
    receiver_thread.join();

    // --- Step 4: Construct and Send Final ACK Packet ---
    cout << "[+] Constructing final ACK packet..." << endl;
    memset(packet, 0, sizeof(packet)); // Zero out the packet buffer

    // Get pointers to IP and TCP headers within the packet buffer
    struct iphdr *iph = (struct iphdr *)packet;
    struct tcphdr *tcph = (struct tcphdr *)(packet + sizeof(struct iphdr));

    // ** Fill IP Header **
    iph->ihl = 5; // Header Length: 5 * 32 bits = 20 bytes
    iph->version = 4; // IPv4
    iph->tos = 0; // Type of Service
    iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr)); // Total packet length
    iph->id = htons(rand() % 65535); // Packet ID (random)
    iph->frag_off = 0; // Fragmentation flags/offset
    iph->ttl = 64; // Time To Live
    iph->protocol = IPPROTO_TCP; // Protocol: TCP
    iph->check = 0; // Kernel will calculate checksum if IP_HDRINCL is set (usually)
    iph->saddr = inet_addr(CLIENT_IP); // Source IP address
    iph->daddr = server_addr.sin_addr.s_addr; // Destination IP address (from server_addr)

    // ** Fill TCP Header **
    tcph->source = htons(CLIENT_PORT); // Source Port
    tcph->dest = htons(SERVER_PORT);   // Destination Port (server's port)
    tcph->seq = htonl(600); // Final ACK sequence number
    tcph->ack_seq = htonl(server_seq_num + 1); // Acknowledge server's SYN sequence number + 1
    tcph->doff = 5;  // Data Offset: 5 * 32 bits = 20 bytes (TCP header size)
    tcph->syn = 0;   // Not a SYN packet
    tcph->ack = 1;   // ACK flag is set
    tcph->window = htons(5840); // TCP window size
    tcph->check = 0; // Checksum: set to 0; kernel usually calculates for raw sockets

    // ** Send the final ACK packet **
    cout << "[+] Sending final ACK packet (SEQ=600, ACK=" << server_seq_num + 1 << ") to server..." << endl;
    if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-] sendto() failed for final ACK");
        close(sock);
        exit(EXIT_FAILURE);
    }
    cout << "[+] Final ACK packet sent. Handshake complete!" << endl;

    // --- Step 4: Cleanup ---
    close(sock);
    cout << "[+] Socket closed." << endl;

    return 0;
}