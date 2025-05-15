#include <iostream>
#include <vector>
#include <limits>
#include <queue>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

const int INF = 9999;

void printDVRTable(int node, const vector<vector<int>>& table, const vector<vector<int>>& nextHop) {
    cout << "Node " << node << " Routing Table:\n";
    cout << "Dest\tCost\tNext Hop\n";
    for (int i = 0; i < table.size(); ++i) {
        cout << i << "\t" << table[node][i] << "\t";
        if (nextHop[node][i] == -1) cout << "-";
        else cout << nextHop[node][i];
        cout << endl;
    }
    cout << endl;
}

// Function to simulate Distance Vector Routing (DVR)
void simulateDVR(const vector<vector<int>>& graph) {
    int n = graph.size(); // Number of nodes in the graph
    vector<vector<int>> temp_graph = graph;

    // Replace 0s (except self-loops) with INF to represent no direct connection
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            temp_graph[i][j] = (i != j && temp_graph[i][j] == 0) ? INF : temp_graph[i][j];
        }
    }

    // Initialize distance and next-hop tables
    vector<vector<int>> dist = temp_graph; // Distance table
    vector<vector<int>> nextHop(n, vector<int>(n)); // Next-hop table

    // Set initial next-hop values
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            nextHop[i][j] = (temp_graph[i][j] < INF && i != j) ? j : -1;

    // Perform iterative updates until no changes occur
    bool updated;
    do {
        updated = false;
        for (int i = 0; i < n; ++i) { // For each source node
            for (int j = 0; j < n; ++j) { // For each neighbor
                if (temp_graph[i][j] >= INF || i == j) continue; // Skip unreachable or self-loops
                for (int k = 0; k < n; ++k) { // For each destination
                    if (dist[j][k] >= INF) continue; // Skip unreachable destinations
                    int newDist = temp_graph[i][j] + dist[j][k]; // Calculate new distance
                    if (newDist < dist[i][k]) { // Update if a shorter path is found
                        dist[i][k] = newDist;
                        nextHop[i][k] = j; // Update next-hop
                        updated = true; // Mark that an update occurred
                    }
                }
            }
        }
    } while (updated); // Repeat until no updates

    // Print the final DVR tables for all nodes
    cout << "--- DVR Final Tables ---\n";
    for (int i = 0; i < n; ++i) printDVRTable(i, dist, nextHop);
}

void printLSRTable(int src, const vector<int>& dist, const vector<int>& prev) {
    cout << "Node " << src << " Routing Table:\n";
    cout << "Dest\tCost\tNext Hop\n";
    for (int i = 0; i < dist.size(); ++i) {
        if (i == src) continue;
        cout << i << "\t" << dist[i] << "\t";
        int hop = i;
        while (prev[hop] != src && prev[hop] != -1)
            hop = prev[hop];
        cout << (prev[hop] == -1 ? -1 : hop) << endl;
    }
    cout << endl;
}

// Function to simulate Link State Routing (LSR) using Dijkstra's algorithm
void simulateLSR(const vector<vector<int>>& graph) {
    int n = graph.size(); // Number of nodes in the graph
    vector<vector<int>> temp_graph = graph;

    // Replace 0s (except self-loops) with INF to represent no direct connection
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            temp_graph[i][j] = (i != j && temp_graph[i][j] == 0) ? INF : temp_graph[i][j];
        }
    }

    // Perform Dijkstra's algorithm for each node
    for (int src = 0; src < n; ++src) {
        vector<int> dist(n, INF); // Distance from source to each node
        vector<int> prev(n, -1); // Previous node in the shortest path
        vector<bool> visited(n, false); // Visited nodes
        dist[src] = 0; // Distance to self is 0

        // Dijkstra's algorithm
        for (int i = 0; i < n; ++i) {
            int u = -1;
            // Find the unvisited node with the smallest distance
            for (int j = 0; j < n; ++j)
                if (!visited[j] && (u == -1 || dist[j] < dist[u]))
                    u = j;

            if (dist[u] == INF) break; // Stop if all remaining nodes are unreachable
            visited[u] = true; // Mark the node as visited

            // Update distances to neighbors
            for (int v = 0; v < n; ++v) {
                if (!visited[v] && temp_graph[u][v] < INF) {
                    if (dist[u] + temp_graph[u][v] < dist[v]) {
                        dist[v] = dist[u] + temp_graph[u][v];
                        prev[v] = u; // Update previous node
                    }
                }
            }
        }

        // Print the routing table for the current source node
        printLSRTable(src, dist, prev);
    }
}

vector<vector<int>> readGraphFromFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        exit(1);
    }
    
    int n;
    file >> n;
    vector<vector<int>> graph(n, vector<int>(n));

    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            file >> graph[i][j];

    file.close();
    return graph;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input_file>\n";
        return 1;
    }

    string filename = argv[1];
    vector<vector<int>> graph = readGraphFromFile(filename);

    cout << "\n--- Distance Vector Routing Simulation ---\n";
    simulateDVR(graph);

    cout << "\n--- Link State Routing Simulation ---\n";
    simulateLSR(graph);

    return 0;
}
