
# Routing Protocols Simulation (DVR and LSR)

## Overview

This assignment simulates two major routing protocols:

- **Distance Vector Routing (DVR)**
- **Link State Routing (LSR)**

These protocols help routers find the best path to send data across a network. The simulation reads a graph of nodes from a file and prints the routing tables computed by each algorithm.

---

## Files

### `routing_sim.cpp`
This is the main C++ program that:
- Reads the graph from an input file.
- Runs simulations for both DVR and LSR.
- Prints the final routing tables for each node.

### Input Files
You must provide an input text file containing the adjacency matrix of the graph.  
The format of the file is:
```
n
a11 a12 ... a1n  
a21 a22 ... a2n  
...  
an1 an2 ... ann
```
Where:
- `n` = number of nodes
- `aij` = cost from node `i` to node `j`  
(Use `0` if there is no direct connection, except for self-loops.)

---

## How It Works

### Distance Vector Routing (DVR)
- Each node keeps a table with the shortest distance to every other node.
- Nodes exchange their routing tables with neighbors and update their own tables if a shorter path is found.
- This continues until no more changes happen.
- Final routing tables show:
  - Destination node
  - Cost 
  - Next hop node

### Link State Routing (LSR)
- Each node uses Dijkstra’s algorithm to find the shortest path to all other nodes.
- Each node knows the full network topology.
- The routing table shows:
  - Destination node
  - Cost
  - Next hop node along the shortest path

---

## How to Run

1. **Compile the code**:
   ```bash
   g++ routing_sim.cpp -o routing_sim
   ```

2. **Run the simulation**:
   ```bash
   ./routing_sim input.txt
   ```

---

## Team Contributors

- **Dhruv Gupta (220361)** - 33.33%
- **Kundan Kumar (220568)** - 33.33%
- **Pragati Agrawal (220779)** - 33.33%

All members contributed equally to the design, coding, and documentation of this project.

---

## Sources Referred

- *Computer Networking: A Top-Down Approach* by Kurose and Ross
- GeeksforGeeks articles on DVR and Dijkstra’s Algorithm

---

## Declaration

We declare that this assignment, titled **"Assignment 4: Routing Protocols"**, is our original work. We have not copied or used unauthorized help. Any references used have been properly cited.

---

## Feedback

This assignment helped us understand routing protocols better, especially how routers find the shortest paths and update their information.

---

_Thank You!_