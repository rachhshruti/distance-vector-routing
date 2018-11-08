/*
 * A simple RIP protocol implementing distance vector using Bellman-Ford algortihm.
 * The config file,port number,TTL,period and split-horizon is passed as an argument
 * Created on: Nov 7, 2015
 * Author: Prerna Preeti, Shruti Rachh
 */

#include <iostream>
#include <cstdlib>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include <fstream>
#include <time.h>
#include <math.h>
#include <cmath>
#include <netdb.h>
#include <sys/time.h>
#include <map>
#include <arpa/inet.h>
#include <pthread.h>

using namespace std;

pthread_mutex_t lck;
int sock;
int cnt;
int time_to_live=90;
string null_val="null\t";
struct sockaddr_in address;
map<string,int> ipLbl;
map<int,string> lblIP;
struct RouteEntry {
    char destination[16];
    char nextHop[16];
    int cost;
    int ttl;
}*entry;

struct RecvParams {
    int recv_sock;
    int neigh_port;
    int infinity;
    int split_horizon;
}params;

void displayError(const char *errorMsg) {
    cerr<<"Error: "<<errorMsg<<endl;
    exit(1);
}

char* getIPFromHostName(const char* host) {
    struct hostent *host_addr;
    host_addr=gethostbyname(host);
    return inet_ntoa (*(struct in_addr *)*host_addr->h_addr_list);
}

void mapCurrentHostIP() {
    char host[255];
    gethostname(host, 255);
    char* myIP=getIPFromHostName(host);
    cout<<"My IP: "<<myIP<<endl;
    ipLbl[myIP]=0;
    lblIP[0]=myIP;
}

/*
 * Maps IP to Node label and vice versa
 */
void mapIPLbl(char* config_file) {
    string line;
    ifstream readFile;
    readFile.open(config_file);
    int lblCnt=1;
    mapCurrentHostIP();
    if(readFile.is_open()) {
        while(getline(readFile,line)) {
            size_t pos=line.find(" ");
            char* nodeIP=getIPFromHostName(line.substr(0,pos).c_str());
            ipLbl[nodeIP]=lblCnt;
            lblIP[lblCnt]=nodeIP;
            lblCnt++;
        }
    }
    readFile.close();
}

/* 
 * Creates socket and binds address
 */
void createSocket(int port) {
    sock=socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        displayError("The server socket could not be opened!");
    }
    int leng=sizeof(address);
    bzero(&address,leng);
    address.sin_family = AF_INET;
    address.sin_port = port;
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *) &address,leng) < 0) {
        displayError("There is some problem while binding the socket to an address!");
    }
}

/*
 * Prints the routing table
 */
void printRoutingTbl(RouteEntry *entry,int cnt) {
    struct timeval t;
    gettimeofday(&t,NULL);
    for(int i=0;i<cnt;i++) {
        cout<<entry[i].destination<<"\t"<<entry[i].nextHop<<"\t"<<entry[i].cost<<"\t"<<entry[i].ttl<<endl;
    }
    cout<<"Current time is: "<<t.tv_sec<<" seconds and "<<t.tv_usec<<" microseconds. "<<endl<<endl;
}

/* 
 * Creates the initial routing table 
 */
void createRoutingTbl(int **cost_tbl,int cnt,int curr_node,int ttl,int infinity) {
    entry=new RouteEntry[cnt];
    for(int i=0;i<cnt;i++) {
        strcpy(entry[i].destination,lblIP[i].c_str());
        entry[i].ttl=ttl;
        if(cost_tbl[curr_node][i]==infinity) {
            strcpy(entry[i].nextHop,null_val.c_str());
        } else {
            strcpy(entry[i].nextHop,lblIP[i].c_str());
        }
        entry[i].cost=cost_tbl[curr_node][i];
    }
}

/* 
 * Initializes the cost matrix and creates initial routing table
 */
void initialize(char* argv[]) {
    char *config_file=argv[1];
    int port=htons(atoi(argv[2]));
    int ttl=atoi(argv[3]);
    int infinity=atoi(argv[4]);
    int period=atoi(argv[5]);
    bool sp_horz=atoi(argv[6]);
    int node_lbl=0;
    createSocket(port);
    ifstream readFile;
    string line;
    cnt=ipLbl.size();
    int **cost_tbl;
    cost_tbl=new int*[cnt];
    for(int i=0;i<cnt;i++) {
        cost_tbl[i]=new int[cnt];
    }
    for(int i=0;i<cnt;i++) {
        for(int j=0;j<cnt;j++) {
            cost_tbl[i][j]=infinity;
        }
    }
    cost_tbl[node_lbl][node_lbl]=0;
    readFile.open(config_file);
    if(readFile.is_open()) {
        while(getline(readFile,line)) {
            size_t pos=line.find(" ");
            char* nodeIP=getIPFromHostName(line.substr(0,pos).c_str());
            int node=ipLbl[nodeIP];
            string isNeighbor=line.substr(pos+1,line.length());
            if(strcmp(isNeighbor.c_str(),"yes")==0) {
                cost_tbl[node_lbl][node]=1;
            }
        }
    }
    readFile.close();
    return createRoutingTbl(cost_tbl,cnt,node_lbl,ttl,infinity);
}

/* 
 * Checks the TTL for node failure
 */
void checkttl(int infinity) {
    for(int i=0;i<cnt;i++) {
        if(entry[i].ttl==0) {
            entry[i].cost=infinity;
        }
    }
}

/* 
 * Sends Advertisement to the neighbor nodes
 */
void sendAdv(int infinity1,int split_horizon1,int neigh_port) {
    struct sockaddr_in neighbor_addr;
    checkttl(infinity1);
    for(int i=0;i<cnt;i++) {
        if(entry[i].cost==1) {
            bzero((char *) &neighbor_addr, sizeof(neighbor_addr));
            neighbor_addr.sin_family = AF_INET;

            inet_pton(AF_INET,entry[i].destination,&(neighbor_addr.sin_addr.s_addr));
            neighbor_addr.sin_port = htons(neigh_port);
            unsigned int len=sizeof(struct sockaddr_in);
            cout<<"Sending my routing table to neighbor "<<entry[i].destination<<endl;
            if(split_horizon1==0) {
                for(int j=0;j<cnt;j++) {
                    int no=sendto(sock,&entry[j],sizeof(entry[j]),0,(const struct sockaddr *)&neighbor_addr,len);
                    if (no<0) {
                        displayError("There is problem while sending request!");
                    }
                }
            } else {
                for(int j=0;j<cnt;j++) {
                    if(!((strcmp(entry[i].destination,entry[j].nextHop)==0) && !(strcmp(entry[j].destination,entry[j].nextHop)==0))) {
                        int no=sendto(sock,&entry[j],sizeof(entry[j]),0,(const struct sockaddr *)&neighbor_addr,len);
                        if (no<0) {
                            displayError("There is problem while sending request!");
                        }
                    } else {
                        RouteEntry tmp;
                        strcpy(tmp.destination,entry[j].destination);
                        strcpy(tmp.nextHop,entry[j].nextHop);
                        tmp.cost=infinity1;
                        tmp.ttl=time_to_live;
                        int no=sendto(sock,&tmp,sizeof(tmp),0,(const struct sockaddr *)&neighbor_addr,len);
                        if (no<0) {
                                displayError("There is problem while sending request!");
                        }
                    }
                }
            }
        }
    }
    cout<<"Decrementing ttl for neighboring nodes..."<<endl;
    for(int i=0;i<cnt;i++) {
        if(entry[i].cost==1) {
            entry[i].ttl=entry[i].ttl-ceil(time_to_live/3);
        }
    }

    cout<<"Updated my routing table to: "<<endl;
    printRoutingTbl(entry,cnt);
}

/* 
 * Updates the routing table entry
 */
void update(char* recv_node,int recv_cost,int index) {
    entry[index].cost=recv_cost;
    strcpy(entry[index].nextHop,recv_node);
    cout<<"Updated my routing table to: "<<endl;
    printRoutingTbl(entry,cnt);
}

/*
 * Implementation of Bellman Ford algorithm
 */
void bellmanFord(RouteEntry *recv_entry,char* recv_node,int cost_curr_to_recv,int num_not_neigh, string *not_neigh_arr) {
    pthread_mutex_lock(&lck);
    int neighbor_cnt=cnt-num_not_neigh-1;
    for(int m=0;m<num_not_neigh;m++) {
        for(int k=0;k<cnt;k++) {
            if(strcmp(recv_entry[k].destination,not_neigh_arr[m].c_str())==0) {
                int cost_recv_to_dest=recv_entry[k].cost;
                int recv_cost=cost_curr_to_recv+cost_recv_to_dest;
                for(int j=0;j<cnt;j++) {
                    if(strcmp(recv_entry[k].destination,entry[j].destination)==0) {
                        if(entry[j].cost>recv_cost && entry[j].ttl>0) {
                            update(recv_node,recv_cost,j);
                        } else if(recv_entry[k].ttl==0) {
                            cout<<"Node failed!!"<<endl;
                            entry[j].ttl=0;
                            entry[j].cost=recv_entry[k].cost;
                        }
                    }
                }
                break;
            }
        }
    }
    neighbor_cnt--;
    if(neighbor_cnt==0) {
        sendAdv(params.infinity,params.split_horizon,params.neigh_port);
    }
    pthread_mutex_unlock(&lck);
}

/* 
 * Calculates cost for non-neighbors 
 */
void process_routing_tbl(RouteEntry *recv_entry,char* recv_node) {
    int cost_curr_to_recv;
    int num_not_neigh=0;
    string not_neigh_arr[cnt];

    for(int i=0;i<cnt;i++) {
            if(strcmp(entry[i].destination,recv_node)==0) {
                    cost_curr_to_recv=entry[i].cost;

            }
            if(entry[i].cost>1) {
                    not_neigh_arr[num_not_neigh]=(string)entry[i].destination;
                    num_not_neigh++;
            }

    }
    bellmanFord(recv_entry,recv_node,cost_curr_to_recv,num_not_neigh,not_neigh_arr);
}

/* 
 * Receives the routing table
 */
void receive(int rsock) {
    RouteEntry *recv_entry=new RouteEntry[cnt];

    unsigned int len=sizeof(struct sockaddr_in);
    char* recv_node;
    int no=1;
    while(no>0) {
        for(int j=0;j<cnt;j++) {
            no=recvfrom(rsock,&recv_entry[j],sizeof(recv_entry[j]),0,(struct sockaddr *)&address, &len);
            if(no<0) {
                displayError("recv error");
            }
            if(recv_entry[j].cost==0) {
                recv_node=recv_entry[j].destination;
                for(int k=0;k<cnt;k++) {
                    if(strcmp(recv_node,entry[k].destination)==0) {
                            cout<<"Assigning default ttl to "<<entry[k].destination<<endl;
                            entry[k].ttl=time_to_live;
                    }
                }

            }
        }
        cout<<"Routing table received from "<<recv_node<<" as: "<<endl;
        printRoutingTbl(recv_entry,cnt);
        process_routing_tbl(recv_entry,recv_node);
    }
}

/* 
 * Function to call a new thread
 */
void *recv_adv(void *recv_sock) {
    int rsock=*(int*)recv_sock;
    receive(rsock);
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t recv_thread;
    if(argc < 7) {
        displayError("ERROR, Usage:1.Config file 2.Port 3.TTL 4. Infinity 5.Period 6.Split-horizon \n");
    }
    time_to_live=atoi(argv[3]);
    mapIPLbl(argv[1]);
    initialize(argv);
    cout<<"My initial routing table: "<<endl;
    printRoutingTbl(entry,cnt);
    params.recv_sock=sock;
    params.neigh_port=atoi(argv[2]);
    params.infinity=atoi(argv[4]);
    params.split_horizon=atoi(argv[6]);

    //Initializes mutex lock
    if (pthread_mutex_init(&lck, NULL) != 0) {
        printf("\n mutex init failed\n");
        return 1;
    }
    pthread_create(&recv_thread,NULL,recv_adv,(void*)&sock);

    sleep(15);

    //Sends periodic advertisement	
    while(1) {
        sendAdv(params.infinity,params.split_horizon,params.neigh_port);
        sleep(atoi(argv[5]));
    }
    pthread_mutex_destroy(&lck);
    return 0;
}
