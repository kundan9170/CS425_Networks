# Assignment 2 : DNS Resolver

## Features Implemented
- **Iterative DNS Resolution**: Implements iterative resolution starting from root servers.
- **Recursive DNS Resolution**: Uses system's default resolver for recursive resolution.
- **Timeout Handling**: Each query has a timeout to prevent indefinite waiting.
- **Error Handling**: Handles failures in name resolution and missing records.
- **Performance Measurement**: Displays time taken for resolution.

## Design Decisions
### Querying Strategy
- **Iterative Resolution**: The resolver follows the DNS hierarchy, starting from root servers, querying TLD servers, and then authoritative servers.
- **Recursive Resolution**: We use dns library's dns.resolver.resolve() function for resolution.

### Error Handling
- If a DNS query fails, the next available nameserver is tried.
- If no response is received, resolution terminates with an error.
- If the given domain does not exist, then response has `rcode == NXDOMAIN`. This is checked for each response in iterative lookup.

## Implementation Details
### **Iterative Lookup**
   - Start with ROOT servers. Here we have used 5 out of the 13 root servers in the world.
   - We query each ROOT server one by one and if we get a response, we extract the list of name servers for the TLD stage and we update the `next_ns_list` to contain the list of TLD servers. If no response, then we query the next ROOT server.
   - Similarly we query each TLD server one by one and if we get a response, we extract the list of name servers for the AUTH stage and we update the `next_ns_list` to contain the list of AUTH (authoritative) servers. If no response, then we query the next TLD server.
   - Now we query the authoritatative servers one by one, if we get a response, we have found the IP address of the requested domain. Otherwise, we query the next authoritative server in the list. 
   - If no server responds, or the response is errorneous we display an error. If at any stage the `next_ns_list` becomes empty and we have not found the ip address yet, we display an error.
### **Recursive Lookup**
   - We use the `dns.resolver.resolve` function to get the authoritative name servers (NS records) for the domain and then we print those.
   - Then we use this function again to get the IPv4 A records for the IP address.
   - It uses the system's inbuilt dns resolver to do the recursive resolution.

### Important Functions
- `send_dns_query(server, domain)`: Sends a UDP query for IPv4 address to a given name server. Uses a `TIMEOUT`=3. If response is not received before this timeout, the query fails and it returns `None`.
- `extract_next_nameservers(response)`: Extracts NS records from the response and resolves their IPs. It loops through the `response.authority` and appends all name servers in it to the `ns_names` list. Then it resolves the IP address for each name server in the list and appends those to the `ns_ips` list. Returns `ns_ips` list.
- `iterative_dns_lookup(domain)`: Implements iterative resolution.
- `recursive_dns_lookup(domain)`: Implements recursive resolution.

### Code Flow Summary for Iterative Mode
1. Start with root server IPs.
2. Query root server → Get NS records for TLD.
3. Query TLD server → Get NS records for authoritative servers.
4. Query authoritative server → Get A record.
5. If answer found, return it; otherwise, repeat with next NS.

## Testing
### Correctness Testing
- Verified correct resolution of domains (e.g., `google.com`, `iitk.ac.in`, `amazon.com`, `mit.edu`, `uwaterloo.ca`).
- Compared results with `dig` and `nslookup`.

### Failure Handling
- Tested with invalid domains to ensure proper error handling.
- Tested with smaller TIMEOUT values (close to 0) to ensure timeout is handled properly.

## How to run
- **`dns_server.py`** contains the complete code.
- For iterative lookup, write the following command:
```bash
python3 dns_server.py iterative <domain>
```
- For recursive lookup, write the following command:
```bash
python3 dns_server.py recursive <domain>
```

## Team Contributors
- Dhruv Gupta (220361) [**`33.33%`**]
- Kundan Kumar (220568) [**`33.33%`**]
- Pragati Agrawal (220779) [**`33.33%`**]

All three of us contributed equally to the assignment. Every part was developed either by working together or having one person write while the other two review and debug it.
We collaboratively discussed the high level design of the server and thought about additional features to enhance its functionality in a meet. The README was also written with equal contribution from all of us.

## Sources Referred
- RFC 1035 (DNS Specification)
- `dnspython` Documentation

## Declaration
We hereby declare that this assignment, tilted **Assignment 2 : DNS Resolver** is our original work. We have not engaged in any form of plagiarism, cheating, or unauthorized collaboration. All sources of information and references used have been properly cited.

## Feedback
- The assignment helped us understand DNS resolution deeply.
- Could include more details on optimizing query performance.

### _Thank You!_
