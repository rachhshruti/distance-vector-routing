# Distance Vector

This project is an implementation of distributed distance vector algorithm which computes the routing table of each node by sending periodic and triggered updates to each of its neighbors. Multi-threading and mutex locks are used to support periodic and triggered updates simultaneously. Split horizon is provided as a command line option to avoid the count to infinity problem in the distance vector algoirthm.

# Requirements:
- Linux operating system

# Running the code:

This code needs to be run on separate terminals depending on the number of routers (nodes) that are present in the network.

Run the makefile which compiles the code by typing the following command in the terminal: make

To execute the program:

    ./Dist_Vector config_file port ttl infinity period split_horizon

It takes following command-line arguments:

1. config_file: It contains the information about the directly connected neighbors of the node as follows:

              Neighbor1 (IP address) yes, if directly connected neighbor
              Neighbor2 (IP address) no, if not
2. port: The port number to be used for communication. It should be same for all nodes in the network.
3. ttl: Time-to-live (TTL) is the number of seconds it waits to receive response from neighbor nodes before it considers it as unreachable.
4. infinity: The number that is used to indicate that the node is unreachable.
5. period: All the nodes send their routing table after every period seconds.
6. split_horizon: boolean (1/0) to indicate whether split horizon is supported or not.
