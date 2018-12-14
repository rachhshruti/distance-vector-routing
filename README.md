# Distance Vector

This project is an implementation of distributed distance vector algorithm which computes the routing table of each node by sending periodic and triggered updates to each of its neighbors. Multi-threading and mutex locks are used to support periodic and triggered updates simultaneously. Split horizon is provided as a command line option to avoid the count to infinity problem in the distance vector algoirthm.

# Requirements

- Docker

# Running the code

Run the following shell script

	./run.sh

This will run the code within multiple Docker containers and the number of containers will be equal to the number of nodes in the network and can be set in run.sh file.

This code has been tested for the following network:

![Network Diagram](https://github.com/rachhshruti/distance-vector-routing/blob/master/images/network.png)

The output of the algorithm is saved under src/output_files directory.

To define a network, edit the config files under the src/config_files directory. It contains the information about the directly connected neighbors of the node as follows:

              Neighbor1 (Host name/IP address) yes, if directly connected neighbor
              Neighbor2 (Host name/IP address) no, if not

Following environment variables are needed to run the code and can be set in environment/env_variables.env:

1. port: The port number to be used for communication. It should be same for all nodes in the network.
2. ttl: Time-to-live (TTL) is the number of seconds it waits to receive response from neighbor nodes before it considers it as unreachable.
3. infinity: The number that is used to indicate that the node is unreachable.
4. period: All the nodes send their routing table after every period seconds.
5. split_horizon: boolean (1/0) to indicate whether split horizon is supported or not.

# Future Work

Right now, the host names are hard-coded as node1, node2 and so on and the corresponding config files are named as node1_config.txt, node2_config.txt and so on. In future, it can be extended to take input of host names in the network or even better it should be able to read the config file information based on given network diagrams.
