/*=============================================================================
 * File Name: serverOps.c
 * Project  : cse589_project2
 * Version  : 0.1V
 * Author   : Rajaram Rabindranath (50097815)
 * Created  : October 22nd 2013
 ============================================================================*/

#include <appMacros.h>
#include <error.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <routingTableTemplate.h>
#include <serverDetails.h>
#include <appMacros.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <messageTemplate.h>
#include <commandOperations.h>
#include <stdlib.h>
#include <routingTableOps.h>
#include <globalVars.h>
#include <neighbourDetails.h>
#include <time.h>

#define MSG_LINE_ITEM_SIZE 96
#define COMMAND_LENTH_MINSIZE 4 // in correspondence with the 'step' command
#define MSG_HEADER_SIZE 64
#define TIMEOUT_LIMIT 3
#define INIFINITY_STRING "inf"
#define TIMEOUT_UPDATE_CODE 0
#define MANUAL_UPDATE_CODE 0
// function prototype
int getServerID(char* serverIP,networkInfo* network);
char* constructMessagePacket(networkInfo* network,int* messageSize,unsigned short int** costMatrix);
int parseMessage_n_updateCostMatrix(char* message,int messageSize,networkInfo* network,int** nextHop,routingTableRecord** rt,int myServerID,int senderServerID,unsigned short int*** costMatrix);
unsigned short int** createCostMatrix(unsigned short int nodes,int neighbourCount,links* edges);
int distanceVector_Calc(int senderServerID,neighbour* neighbourList,int** nextHop,networkInfo** network,unsigned short int*** matrix );
void sendData_to_Neighbours(int sockFD,neighbour* neighbourList,int neighbourCount,char *message,int messageSize);
int isNeighbour(neighbour* neighbourList,int neighbourCount,unsigned short int targetServerID);

// functions to handle user inputs from the console
void packets(int *pkts);
int update(char** commandArgs,networkInfo** network,int** nextHop,unsigned short int*** costMatrix,neighbour* neighbourList);
int disable(int** nextHop,neighbour *neighbourList,networkInfo** network,unsigned short int*** costMatrix);
int help();
/*=============================================================================
 * Function Name: startServer
 * Function Desc: Shall output to the console the application's usage
 * Parameters   : int myPort, int myServerID, int neighbourCount
 * Return value : void
 ============================================================================*/
int startServer(int updateInterval, networkInfo* network)
{
	printf("Starting server now\n");

	int mySocket,recvBytes=0,commandID,selectedSocket, fdMax =0, i =0,j=0,k=0, pkts=0;
	int selectRetValue = 0;
	int crash = FALSE;
	socklen_t len;
	fd_set read_fds, write_fds, write_fdsMirror, read_fdsMirror;
	char* recvMsg = NULL;
	struct sockaddr_in myAddress, fromAddress; // to find who is sending info
	struct in_addr neighbourIP;
	char ipAddress[INET_ADDRSTRLEN];

	int senderServerID = 0;
	int pass = FALSE;
	int mySocketSend = 0;

	// A timeout event for select to send updates to neighbours at regular intervals
 	struct timeval recvTimeout;
	recvTimeout.tv_sec = updateInterval;
	recvTimeout.tv_usec = 0;

	// have pointers to elements within network to avoid frequent de-referencing
	links *edges = network->edges;
	serverInfo *servers = network->serversOnNetwork;
	neighbour *neighbourList = malloc(sizeof(neighbour)*network->neighbourCount);


	// routing level information
	routingTableRecord *routingTable = contructRoutingTable(network);
	unsigned short int** costMatrix = NULL;
	int* nextHop = calloc(network->serverCount,sizeof(int));
	for(i=0;i<network->serverCount;i++)
	{
		nextHop[i] = UNDEFINED;
	}

	// resetting all file descriptor sets
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	FD_ZERO(&write_fdsMirror);
	FD_ZERO(&read_fdsMirror);

	costMatrix = createCostMatrix(network->serverCount,network->neighbourCount,network->edges);
	printRoutingTable(routingTable,network->serverCount);

	printf("\n\n ATTENTION:: type \"help\" for well help\n\n");


	#if DEBUG // print the cost matrix
	for(i=0;i<network->serverCount;i++)
	{
		for(j=0;j<network->serverCount;j++)
		{
			printf("%d ",costMatrix[i][j]);
		}
		printf("\n");
	}
	#endif

	/*
	 * has a seperate ds for neighbours -- does complicate things but is fine
	 * here I is the index used to iterate thru the networkInfo ds
	 * and J is the index used to iterate thru the neighbourList ds
	 */
	for(i=0,j=0;i<network->serverCount && j< network->neighbourCount;i++)
	{
		if(servers[i].isNeighbour == TRUE)
		{
			neighbourList[j].neighbourAddress.sin_family = AF_INET;
			inet_aton(servers[i].serverIP,&neighbourIP);
			neighbourList[j].neighbourAddress.sin_addr = neighbourIP;
		    neighbourList[j].neighbourAddress.sin_port = htons(servers[i].serverPort);
		    neighbourList[j].port = servers[i].serverPort;
		    neighbourList[j].serverID = servers[i].serverID;
		    neighbourList[j].cost = servers[i].cost; //
		    memcpy(neighbourList[j].ipAddress, servers[i].serverIP,INET_ADDRSTRLEN); // FIXME : should the ipAddress only occupy 16 bits
		    neighbourList[j].ignore = FALSE;
		    neighbourList[j].disAllowUpdate = FALSE; // could be used instead of ignore;
		    neighbourList[j].timeoutCount = 0;

		    // set next hop
		    nextHop[servers[i].serverID-1] =servers[i].serverID;

		    j++;
		}
	}

	// get a socket
	if((mySocket=socket(AF_INET,SOCK_DGRAM,0))<0)
	{
		perror("ERROR:");
		printf("ERROR: Could not open socket on my machine! DAMN!\n");
		return FAILURE;
	}



	if((mySocketSend=socket(AF_INET,SOCK_DGRAM,0))<0)
	{
		perror("ERROR :");
		printf("ERROR : Could not get a dedicated socket for sending data\n");
	}

	memset(&myAddress,0,sizeof(myAddress));

	myAddress.sin_family = AF_INET;
	myAddress.sin_addr.s_addr=htonl(INADDR_ANY);
	myAddress.sin_port=htons(network->myPort);

	if((bind(mySocket,(struct sockaddr *)&myAddress,sizeof(myAddress)))<0)
	{
		perror("ERROR :");
		printf("ERROR : Unable to bind to the socket that i have created! DAMN\n");
		return FAILURE;
	}

	int messageSize =0;
	char* message = constructMessagePacket(network,&messageSize,costMatrix);


	fdMax = STDIN_FILENO > mySocket ?STDIN_FILENO:mySocket;

	recvMsg = malloc(sizeof(char)*MESSAGE_SIZE); // buffer to receive the routing tables from neighbours

	FD_SET(STDIN_FILENO,&read_fds);
	FD_SET(mySocket,&read_fds);

	// which one should i use -- FIXME
	FD_SET(mySocketSend,&write_fds);

	fdMax = fdMax > mySocketSend ? fdMax:mySocketSend;

	while (TRUE)
	{
		read_fdsMirror = read_fds;
		write_fdsMirror = write_fds;

		#if DEBUG
		printf("Calling select with timeoutValue =%ld!\n",recvTimeout.tv_sec);
		#endif

		if ((selectRetValue = select(fdMax+1, &read_fdsMirror,NULL, NULL, &recvTimeout)) == -1)
		{
			perror("ERROR : Select error");
			return FAILURE;
		}

		if(selectRetValue == 0) // there has been a timeout yikes
		{
			#if DEBUG
			printf("Select : returned because of timeout\n");
			#endif
			if(crash == FALSE)
			{
				for(i=0;i<network->neighbourCount;i++)
				{
					neighbourList[i].timeoutCount++;
					if(neighbourList[i].timeoutCount >= TIMEOUT_LIMIT && neighbourList[i].ignore != TRUE)
					{
						// please start ignoring this neighbour
						neighbourList[i].ignore = TRUE;
						neighbourList[i].disAllowUpdate = TRUE;
						neighbourList[i].cost = INFINITY;
						costMatrix[network->myServerID-1][neighbourList[i].serverID-1] = INFINITY;


						// Must set the Direct link between myself and this neighbour to INFINITY
						for(j=0;j<network->neighbourCount;j++)
						{
							if(edges[j].endNode == neighbourList[i].serverID)
							{
								edges[j].linkCost = INFINITY;
							}
						}

						// if we are getting to this server directly then we need to set link gone in next hop router
						// + we need to set the link to infinity to all connections going thru this id
						for(j=0;j<network->serverCount;j++)
						{
							if(nextHop[j] == neighbourList[i].serverID)
							{
								for(k=0;k<network->neighbourCount;k++)
								{
									if(edges[k].endNode == j+1)
									{
										costMatrix[network->myServerID-1][j] = edges[k].linkCost;
										if(edges[k].linkCost !=INFINITY) nextHop[j] = j+1;
										else nextHop[j] = UNDEFINED;
										break;
									}
								}
								if(k == network->neighbourCount)
								{
									#if DEBUG
									printf("Timeout Induced Infinity cost!!");
									#endif
									nextHop[j] = UNDEFINED;
									costMatrix[network->myServerID-1][j] = INFINITY;
								}
							}
						}

						printf("\nSERVER %d has timed-out\n\n",neighbourList[i].serverID);
						updateRoutingTable(network->myServerID,nextHop,network->serverCount,&routingTable,costMatrix);
					}
				}// END OF FOR LOOP

				char* message = constructMessagePacket(network,&messageSize,costMatrix);
				sendData_to_Neighbours(mySocketSend,neighbourList,network->neighbourCount,message,messageSize);
				recvTimeout.tv_sec = updateInterval;
				continue;
			}// END OF IF CRASH == FALSE
		} // END OF IF(SELECT == 0)

		// check which socket is selected
		for(selectedSocket = 0;selectedSocket<=fdMax;selectedSocket++)
		{
			// CHECK IF THE SELECTED fd IS A PART OF read_fds
			if(FD_ISSET(selectedSocket,&read_fdsMirror))
			{
				if(selectedSocket == mySocket)
				{
					len = sizeof(fromAddress);
					memset(recvMsg,0,MESSAGE_SIZE);
					recvBytes = recvfrom(mySocket,recvMsg,MESSAGE_SIZE,0, (struct sockaddr *)&fromAddress,&len);
					if(recvBytes > 0)
					{
						inet_ntop(AF_INET, &(fromAddress.sin_addr),ipAddress, INET_ADDRSTRLEN);

						/*
						 *  check if the sender is in the ignore list
						 *  sender goes into the ignore list when
						 *  the user asks for that connection to be disabled
						 *  if yes then ignore the message
						 */
						for(i=0;i<network->neighbourCount;i++)
						{
							if(!strcmp(ipAddress,neighbourList[i].ipAddress) )
							{
								senderServerID = neighbourList[i].serverID;
								if(neighbourList[i].ignore == TRUE)
								{
									#if DEBUG
									printf("Data from ignored server");
									#endif
									pass = TRUE;
								}
								else // resetting timeoutCount does not matter what it is
								{
									neighbourList[i].timeoutCount = 0;
								}
								break;
							}
						}

						// data from ignored server next iteration then
						if(pass == TRUE)
						{
							pass = FALSE;
							continue;
						}

						printf("Received a DV INFO from SERVER ID - %d\n",senderServerID);

						// if got a message from some random guy running his tests on servers
						if(i == network->neighbourCount) continue; // FIXME -- random errors

						pkts++;

						#if DEBUG
						/* find the sender's name serverID */
						ptr=(msgFormat*) recvMsg;
						printf("number of bytes received =%d\n",recvBytes);
						printf("from the following ip=%s\n",ptr->serverIP);
						#endif

						/*
						 * having received message -- understand/ parse the same
						 * + update the cost matrix -- if there has been an update we shall know
						 */
						if(parseMessage_n_updateCostMatrix(recvMsg,messageSize,network,&nextHop,&routingTable,network->myServerID,senderServerID,&costMatrix)==TRUE)
						{
							// run bellman-ford
							distanceVector_Calc(senderServerID,neighbourList,&nextHop,&network,&costMatrix);
							updateRoutingTable(network->myServerID,nextHop,network->serverCount,&routingTable,costMatrix);
						}
						senderServerID = 0; // reset sender id
					}
					else if(recvBytes == 0)
					{
						printf("A neighbour closed a connection");
					}
					else// error in recvfrom()
					{
						perror("ERROR recvfrom():");
					}
				}
				else if(selectedSocket == STDIN_FILENO)
				{
					if ((recvBytes = read(selectedSocket, recvMsg, MESSAGE_SIZE)) <= 0)
					{
						perror("ERROR :");
						printf("ERROR : Porblem encountered when reading data from console\n");
					}
					else// we got some user input data
					{
						#if DEBUG
						printf("the message from console: %s\n", recvMsg);
						#endif

						if(recvBytes >= COMMAND_LENTH_MINSIZE) // All valid command at least has length(including new line character) > 4
						{
							recvMsg[recvBytes-1] = '\0';

							//if(!commandArgs) free(commandArgs); // free this up

							if((commandID = commandMaster(recvMsg))== INVALID_COMMAND)
							{
								continue;
							}

							#if DEBUG
							printf("Command id is =%d\n",commandID);
							#endif

							// what if the server is in crash state
							if(crash == TRUE && commandID != OP_CODE_EXIT)
							{
								printf("ERROR : Cannot service this command as the server is in crash mode\n");
								printf(" \'EXIT\' or \'exit\' is the only command that will be serviced when the server is crash mode is\n");
								continue;
							}
							// having received the command id
							switch(commandID)
							{
								case OP_CODE_UPDATE:
									if(update(commandArgs,&network,&nextHop,&costMatrix,neighbourList) == SUCCESS)
									{
										printf("Update : SUCCESS\n");
										updateRoutingTable(network->myServerID,nextHop,network->serverCount,&routingTable,costMatrix);
									}
									break;
								case OP_CODE_STEP:
									message = constructMessagePacket(network,&messageSize,costMatrix);
									sendData_to_Neighbours(mySocketSend,neighbourList,network->neighbourCount,message,messageSize);
									printf("Step : SUCCESS\n");
									break;
								case OP_CODE_PACKETS:
									packets(&pkts);
									printf("%s : SUCCESS\n",commandArgs[0]);
									break;
								case OP_CODE_DISPLAY:
									printRoutingTable(routingTable,network->serverCount);
									printf("%s : SUCCESS\n", commandArgs[0]);
									break;
								case OP_CODE_DISABLE:
									/*
									 * Make the cost to that particular serverID to go to INFINITY
									 * + Let it be known that updating of link for that particular
									 * serverID is disallowed
									 * + update the routing table as well
									 */
									if(disable(&nextHop,neighbourList,&network,&costMatrix) == SUCCESS)
									{
										printf("Disable : Success\n");
										updateRoutingTable(network->myServerID,nextHop,network->serverCount,&routingTable,costMatrix);
									}
									break;
								case OP_CODE_CRASH:
									crash = TRUE;
									// sever all communications
									FD_CLR(mySocket,&read_fds);
									FD_CLR(mySocketSend,&write_fds);
									close(mySocket);
									close(mySocketSend);
									fdMax = STDIN_FILENO; // keep console input open
									printf("CRASH : SUCCESS\n");
									break;
								case OP_CODE_EXIT:

									FD_CLR(mySocket,&read_fds);
									FD_CLR(mySocketSend,&write_fds);
									FD_CLR(STDIN_FILENO,&read_fds);
									// i should check if the FDs have already
									// been closed by OP_CODE_CRASH
									if(!mySocket)close(mySocket);
									if(!mySocketSend)close(mySocketSend);
									printf("EXIT : SUCCESS\n");
									return FAILURE;
									break;
								case OP_CODE_HELP:
									help();
									break;
								default:
									printf("%s : sorry command is not supported!",commandArgs[0]);
									break;
							}

							memset(recvMsg,0,MESSAGE_SIZE); // reset buffer
						}
						else
						{
							printf("ERROR : Insufficient information from the user\n");
							printf("ERROR : User input cannot be processed\n");
						}
					}
				}
			}

		}
	}
	return SUCCESS;
}
/*=============================================================================
 * Function Name: distanceVector_Calc
 * Function Desc: Used to calculate the distanceVector using Bellman-ford algo
 * Parameters   :
 * Return value : indicates if the cost of transmission to other servers has
 * 					changed or not (TRUE / FALSE)
 ============================================================================*/
int distanceVector_Calc(int senderServerID,neighbour* neighbourList,int** nextHop,networkInfo** network,unsigned short int*** costMatrix )
{
	int nodes = (*network)->serverCount;
	serverInfo *servers = (*network)->serversOnNetwork;
	links* edges = (*network)->serversOnNetwork;

	int src = (*network)->myServerID-1;
	int i =0;
	int dest = 0;
	int inter = 0; // finding an intermediate node for all dests
	int isUpdated = FALSE;

	/*
	 * only have to update my row
	 * will be using the cost values from other rows!!!
	 * other rows are sent to me by my neighbours
	 */
	for(;dest<nodes;dest++) // find the min cost from src to dest
	{
		if (dest == src) continue; // don't want to find the minimum path from me to me
		for(inter=0;inter<nodes;inter++) // find a intermediate node that shall reduce the cost
		{
			// don't want invalid intermediate node -- i.e me or destination
			if(inter == src || inter == dest || inter != senderServerID-1) continue;

			if(!isNeighbour(neighbourList,(*network)->neighbourCount,inter+1))
			{
				continue;
			}

			// check if cost(me-dest) > cost(me-inter) + cost(inter-dest)
			if((*costMatrix)[src][dest] > (*costMatrix)[src][inter]+(*costMatrix)[inter][dest])
			{
				if((*nextHop)[inter] != inter+1) // 2-1=7 2-3=1 3-1=4 1-4=5---> 2-4 = 10 but thru 3 not 1
				{
					#if DEBUG
					printf("HaZZZZZZZZZZZZZZZZZA!\n");
					#endif
					continue;
				}

				/************* RAJARAM ****************/
				// Network updates sometimes throw the spanner into the works (mislead)
				// and since the cost matrix doesn't have a record of the direct link costs
				// what with all the DV updates happening all the time and DV being run all the time
				// we are making a special check to see if the dest is indeed a neighbour
				// and if the direct link cost is less than the one computed by DV
				for(i=0;i<(*network)->neighbourCount;i++)
				{
					if(edges[i].endNode == dest+1 && edges[i].linkCost < (*costMatrix)[src][inter]+(*costMatrix)[inter][dest])
					{
						#if DEBUG
						printf("Special case\n");
						#endif
						(*costMatrix)[src][dest] = edges[i].linkCost;
						(*nextHop)[dest] = dest+1;
						continue;
					}
				}
				/*************** RAJARAM ****************/
				// check if the intermediate is a
				//printf("To %d with %d as inter\n",dest+1,inter+1);
				#if DEBUG
				printf("\nthe src=%d and dest=%d and inter=%d\n",src+1,dest+1,inter+1);
				printf(" Old cost =%d",(*costMatrix)[src][dest]);
				printf(" New cost =%d add %d\n",(*costMatrix)[src][inter],(*costMatrix)[inter][dest]);
				#endif

				(*costMatrix)[src][dest]=(*costMatrix)[src][inter]+(*costMatrix)[inter][dest];

				// set the nextHop
				if(isNeighbour(neighbourList,(*network)->neighbourCount,inter+1))
				{
					(*nextHop)[dest] = inter+1 ;
				}
				else
				{
					(*nextHop)[dest] = senderServerID;
				}
				isUpdated = TRUE;
			}
		}
	}

	return isUpdated;
}

/*=============================================================================
 * Function Name: constructMessagePacket
 * Function Desc: The message packet constructed by this function is used to
 * 					transmit to neighbour -- HAS the DV
 * Parameters   : networkInfo* network,int *messageSize,
 * 							unsigned short int** costMatrix
 * Return value : char*
 ============================================================================*/
char* constructMessagePacket(networkInfo* network,int *messageSize,unsigned short int** costMatrix)
{
	/**
	 *  Number of update fields (number of neighbours) : count of server IP, serverPort, serverID and Cost tuples
	 *	my port
	 *	my ip address
	 *  Multiple update fields
	 *  first 32 bits= update fields + my server port
	 *  second 32 bits = my IP address
	 *  serverCount * 96 bits
	 */
	int dataSize = network->serverCount * MSG_LINE_ITEM_SIZE;
	int i =0;
	serverInfo *ptr = NULL;
	*messageSize = MSG_HEADER_SIZE+dataSize;
	char* msg = malloc(sizeof(char)*(*messageSize));
	if(!msg)
	{
		printf("ERROR : Could not allocate memory to construct the packet\n");
		return NULL;
	}

	#if DEBUG
	printf("size of the msg =%d\n", MSG_HEADER_SIZE+dataSize);
	#endif

	char* msgMirror = msg;
	// create header
	memcpy(msgMirror,&(network->serverCount),sizeof(network->serverCount));
	msgMirror += sizeof(network->serverCount);

	memcpy(msgMirror,&(network->myPort),sizeof(network->myPort));
	msgMirror += sizeof(network->myPort);

	memcpy(msgMirror,(network->myIP),sizeof(network->myIP));
	msgMirror += sizeof(network->myIP);

	// construct the data part of the message
	for(i=0;i<network->serverCount;i++)
	{
		ptr = ((network->serversOnNetwork)+i);

		memcpy(msgMirror,ptr->serverIP,sizeof(ptr->serverIP));
		msgMirror +=sizeof(ptr->serverIP);
		memcpy(msgMirror,&(ptr->serverPort),sizeof(ptr->serverPort));
		msgMirror += sizeof(ptr->serverPort);
		// i am supposed to save 16 bits for buffer ... no reason
		memcpy(msgMirror,&(ptr->serverID), sizeof(ptr->serverID));
		msgMirror += sizeof(ptr->serverID);

		memcpy(msgMirror,&(costMatrix[network->myServerID-1][ptr->serverID-1]),sizeof(unsigned short int));
		msgMirror += sizeof(unsigned short int);
	}
	return msg;
}

/*=============================================================================
 * Function Name: parseMessage_UpdateCostMatrix
 * Function Desc: parses the received message packet and updates the costMatrix
 * Parameters   : char* message,int senderServerID,
 * 					unsigned short int*** costMatrix
 * Return value : return value indicates if the costMatrix was updated or
 * 					left unchanged
 ============================================================================*/
int parseMessage_n_updateCostMatrix(char* message,int messageSize,networkInfo* network,int** nextHop,routingTableRecord** rt,int myServerID,int senderServerID,unsigned short int*** costMatrix) // can send only that particular row
{
	msgFormat *ptr=(msgFormat*) message;
	links* adjEdges =network->edges;
	int i =0,j=0, k=0;
	int isUpdated = FALSE;

	#if DEBUG
	printf("Have received data from %d\n",senderServerID);
	printf("update fields=%d\n",ptr->numUpdateFields);
	printf("server port =%d\n",ptr->serverPort);
	printf("ip address =%s\n",ptr->serverIP);
	#endif

	msgNugget *nuggPtr = (msgNugget*) (message+sizeof(ptr->numUpdateFields)+sizeof(ptr->serverPort)+sizeof(ptr->serverIP));

	if(messageSize < (MSG_HEADER_SIZE+ptr->numUpdateFields*MSG_LINE_ITEM_SIZE))
	{
		return FALSE;
	}

	int oldCost[ptr->numUpdateFields];

	// updating the costMatrix
	for(i=0;i<ptr->numUpdateFields;i++)
	{
		oldCost[nuggPtr[i].serverID-1] = (*costMatrix)[senderServerID-1][nuggPtr[i].serverID-1];
		(*costMatrix)[senderServerID-1][nuggPtr[i].serverID-1] = nuggPtr[i].cost;
		isUpdated = TRUE;
	}

	i =0;
	/*
	 * not doing this inside the prev loop coz
	 * as cost[myServerID-1][senderServerID-1] - may not have been set
	 */
	for(j=0;j<ptr->numUpdateFields;j++)
	{
		if((*rt)[j].nextHopServerID == senderServerID)
		{
			for(i=0;i<ptr->numUpdateFields;i++)
			{
				if(j==nuggPtr[i].serverID-1)
				{
					if(nuggPtr[i].cost == INFINITY || (*costMatrix)[myServerID-1][senderServerID-1] == INFINITY) // added rajaram
					{
						// but need to check if a direct phy link exits
						for(k=0;k<network->neighbourCount;k++)
						{
							if(adjEdges[k].endNode == j+1) // is J a neighbour
							{
								(*costMatrix)[myServerID-1][j] = adjEdges[k].linkCost;
								if(adjEdges[k].linkCost!=INFINITY)(*nextHop)[j] = j+1;
								else (*nextHop)[j] = UNDEFINED;
								break;
							}

						}
						if(k == network->neighbourCount) // J is not a neighbour
						{
							#if DEBUG
							printf("Infinity by induction for !\n");
							#endif
							(*costMatrix)[myServerID-1][j] = INFINITY; // directly this safe
							(*nextHop)[j] = UNDEFINED;
						}
					}
					else if(oldCost[j]!=(*costMatrix)[senderServerID-1][j])// rajaram but this will happen all the time
					{
						#if DEBUG
						printf("Game on.... sender %d dest %d and their link cost %d!\n",senderServerID,j+1,nuggPtr[i].cost);
						#endif

						// what if J is a neighbour -- need to check if direct link is cheaper
						for(k=0;k<network->neighbourCount;k++)
						{
							if(adjEdges[k].endNode ==j+1)
							{
								#if DEBUG
								printf("bad cost %d and good cost %d\n",((*costMatrix)[myServerID-1][senderServerID-1] + (*costMatrix)[senderServerID-1][j]),adjEdges[k].linkCost);
								#endif

								// >= because when the network is mislead it may so happen that BAD_COST == GOOD_COST [in scenarios like that better to stick with what we know]
								if(((*costMatrix)[myServerID-1][senderServerID-1] + (*costMatrix)[senderServerID-1][j])>=adjEdges[k].linkCost)
								{
									(*costMatrix)[myServerID-1][j] = adjEdges[k].linkCost;
									(*nextHop)[j] = j+1;
								}
								else
								{
									(*costMatrix)[myServerID-1][j] = (*costMatrix)[myServerID-1][senderServerID-1] + (*costMatrix)[senderServerID-1][j]; // new cost for count to infinity
								}
								break;
							}
						}
						if(k == network->neighbourCount)
						{
							(*costMatrix)[myServerID-1][j] = (*costMatrix)[myServerID-1][senderServerID-1] + (*costMatrix)[senderServerID-1][j]; // new cost for count to infinity
						}

					}

				}
			}
		}
	}
	return isUpdated;
}

/*=============================================================================
 * Function Name: getServerID
 * Function Desc: Given a server's IP address find the server's ID
 * Parameters   : char* IP and networkInfo*
 * Return value : void
 ============================================================================*/
int getServerID(char* serverIP,networkInfo* network)
{
	int i =0;
	serverInfo* servers = network->serversOnNetwork;

	for(;i< network->serverCount;i++)
	{
		if(!strcmp(serverIP,servers[i].serverIP))
		{
			return servers[i].serverID;
		}
	}
	return FAILURE; // did not find the server in my list
}

/*=============================================================================
 * Function Name: packets
 * Function Desc: Display the number of packets received
 * Parameters   :
 * Return value : void
 ============================================================================*/
void packets(int *pkts)
{
	printf("The number of packets received since last call - %d\n",*pkts);
	*pkts = 0;
	return;
}

/*=============================================================================
 * Function Name: disable
 * Function Desc: Display connection to a particular server
 * Parameters   :
 * Return value : void
 ============================================================================*/
int disable(int** nextHop,neighbour* neighbourList,networkInfo** network,unsigned short int*** costMatrix)
{
	int i=0,j=0,k=0;
	links* edges = (*network)->edges;
	/*
	 *  check if the number of argument comming in are sufficient
	 *  else signal command failure
	 */
	for(;i<2;i++)
	{
		if(commandArgs[i] == NULL){break;}
		#if DEBUG
		printf("the value is =%s\n",commandArgs[i]);
		#endif
	}
	if(i<2)
	{
		printf("ERROR : The arguments given for the %s command are not sufficient\n",commandArgs[0]);
		return FAILURE;
	}

	int myServerID = (*network)->myServerID;
	int neighbourCount = (*network)->neighbourCount;
	int serverCount = (*network)->serverCount;
	int dest = atoi(commandArgs[1]);

	/*
	 * Check if it is self link disable -- if yes command failure notification
	 */
	if(dest == (*network)->myServerID)
	{
		printf("ERROR : I can't disable a link to myself!\n");
		return FAILURE;
	}
	// check if the link being disabled is a neighbour link
	// command failure notifcation otherwise
	for(i=0;i<neighbourCount;i++)
	{
		if (dest == neighbourList[i].serverID)
		{
			neighbourList[i].ignore = TRUE;
			neighbourList[i].disAllowUpdate = TRUE;
			(*costMatrix)[myServerID-1][dest-1] = INFINITY;
			neighbourList[i].cost = INFINITY;

			for(k=0;k<neighbourCount;k++) // updating the link cost
			{
				if(edges[k].endNode == dest)
					edges[k].linkCost = INFINITY;
			}

			// if we are getting to this server directly then we need to set link gone in next hop router
			// + we need to set the link to infinity to all connections going thru this id
			for(j=0;j<serverCount;j++)
			{
				if((*nextHop)[j] == dest)
				{
					// but what is j was a neighbour then what and direct link was still available with j
					if(isNeighbour(neighbourList,neighbourCount,j+1))
					{
						(*nextHop)[j] = j+1;
						for(k=0;k<neighbourCount;k++) // updating edges as well as finding alternate routes
						{
							if(edges[k].endNode == j+1)
							{
								(*nextHop)[j] = j+1;
								(*costMatrix)[myServerID-1][j] =  edges[k].linkCost;
							}
						}

					}
					else
					{
						// since my link to next hop is down --- my connections to other
						// servers thru next hop should be set to infinity
						// and the nex hop to undefined ----- yes
						(*nextHop)[j] = UNDEFINED;
						(*costMatrix)[myServerID-1][j] = INFINITY;
					}
				}
			}
			break;
		}
	}
	// could not find the serverID given by user in the neighbour list
	if(i == neighbourCount)
	{
		printf("%s : Could not find the furnished serverID in the set of neighbours\n",commandArgs[0]);
		printf("%s : Therefore cannot disable a link that does not exits\n",commandArgs[0]);
		return FAILURE;
	}
	return SUCCESS;
}
/*=============================================================================
 * Function Name: sendData_to_Neighbours
 * Function Desc:
 * Parameters   :
 * Return value : void
 ============================================================================*/
void sendData_to_Neighbours(int sockFD,neighbour* neighbourList,int neighbourCount,char *message,int messageSize)
{
	int i =0,sendCount =0;
	for(;i<neighbourCount;i++)
	{
		if(neighbourList[i].ignore != TRUE )
		{
			sendCount = sendto(sockFD,message,messageSize,0,(struct sockaddr*)&(neighbourList[i].neighbourAddress),sizeof(neighbourList[i].neighbourAddress));
			if(sendCount != messageSize) printf("ERROR : Have not sent the right number of bytes to server=%d\n",neighbourList[i].serverID);
		}
	}
	return;
}
/*=============================================================================
 * Function Name: createCostMatrix
 * Function Desc: creates a matrix of size N*N using contiguous mem locations
 * Parameters   : nodes, neighbour count, edges
 * Return value : unsigned short int**
 ============================================================================*/
unsigned short int** createCostMatrix(unsigned short int nodes,int neighbourCount,links* edges)
{
	// create the matrix
	int i =0, j =0;
	unsigned short int **matrix = malloc(sizeof(unsigned short int*)*nodes);
	if(!matrix) return NULL;
	unsigned short int *mem =  malloc(sizeof(unsigned short int)*(nodes*nodes));
	if(!mem) return NULL;

	// Make 2D array using contiguous block of memory
	for(;i<nodes;i++)
	{
		matrix[i] = mem+(i*nodes);
	}

	// initialize the matrix
	for(i=0;i<nodes;i++)
	{
		for(j=0;j<nodes;j++)
			matrix[i][j] = (i == j) ? 0 : INFINITY;
	}

	// fill up neighbour link cost
	for(i=0;i<neighbourCount;i++)
	{
		matrix[edges[i].startNode-1][edges[i].endNode-1] =
				matrix[edges[i].endNode-1][edges[i].startNode-1] = edges[i].linkCost;
	}
	return matrix;
}

/*=============================================================================
 * Function Name: update
 * Function Desc: This function helps in implementing the update command
 * Parameters   :
 * Return value : SUCCESS / FAILURE
 ============================================================================*/
int update(char** commandArgs,networkInfo** network,int** nextHop,unsigned short int*** costMatrix,neighbour* neighbourList)
{
	// if i were to update .. network ... cost .... edges and routing table
	int i =0,j=0;
	int myServerID = (*network)->myServerID;
	short int dest;
	unsigned short int cost;
	unsigned short int oldCost = 0;

	// check if the number of argument comming in are sufficient
	for(i=0;i<4;i++)
	{
		if(commandArgs[i] == NULL){break;}
	}
	if(i<4)
	{
		printf("ERROR : The arguments given for the %s command are not sufficient\n",commandArgs[0]);
		return FAILURE;
	}

	// check if the update is for a link to be set to INFINITY
	if(!strcmp(commandArgs[3],INIFINITY_STRING))
	{
		cost = INFINITY;
	}
	else // the link being updated is not being set to infinity
	{
		 cost = atoi(commandArgs[3]);
		 if(cost == 0) // atoi shall give 0 if we have an actual zero / character values
		 {
			 printf("%s : there seems to be a problem with the cost update value=%s\n",commandArgs[0],commandArgs[3]);
			 return FAILURE;
		 }
	}

	/**
	 * Error checking
	 *  -- at least one of nodes has to have same serverID as mine
	 *  -- Both the nodes cannot have the same serverID as me
	 *  -- The link whose cost is being updated must have a neighbour
	 *  	as the other node
	 *  -- Check if the destination is unknown -- if yes then please reject the request
	 */
	if(atoi(commandArgs[1]) == myServerID)
	{
		dest = atoi(commandArgs[2]);
	}
	else if(atoi(commandArgs[2]) == myServerID)
	{
		dest = atoi(commandArgs[1]);
	}
	else//get out
	{
		printf("%s : Wrong command; serverID1 & serverID2 neither have the same ID as mine (%d)\n",commandArgs[0],myServerID);
		return FAILURE;
	}

	if(dest == myServerID)
	{

		printf("%s : Both destination and source server ids are the same as mine\n",commandArgs[0]);
		printf("ERROR : Therefore ignoring the update command\n");
		return FAILURE;
	}


	serverInfo* servers = (*network)->serversOnNetwork;
	links* adjacentEdges = (*network)->edges;

	// first check if update is allowed
	for(i=0;i<(*network)->neighbourCount;i++)
	{
		if(neighbourList[i].serverID == dest)
		{
			if( neighbourList[i].disAllowUpdate == TRUE)
			{
				printf("%s : You disabled this link previously/The serverID(%d) timed-out so cannot update the cost now, sorry!",commandArgs[0],neighbourList[i].serverID);
				return FAILURE;
			}
		}
	}

	//Update the edges linked list
	// data structure
	for(i=0;i<(*network)->neighbourCount;i++)
	{
		if(adjacentEdges[i].endNode == dest)
		{
			if(adjacentEdges[i].linkCost != cost)
			{
				adjacentEdges[i].linkCost = cost;
			}
			else
			{
				printf("%s : No change in cost of the link\n",commandArgs[0]);
			}
			break;
		}
	}

	// also takes case of bogus update -- where serverID is one which the system is not aware of
	// allows only neighbour link update ... cannot update the link cost to nodes that are not neighbours
	if(i == (*network)->neighbourCount)
	{

		printf("%s : Cannot update links whose end nodes are not a neighbour\n",commandArgs[0]);
		return FAILURE;
	}
	// FIRST -- update network ds -- important to have the same global information right
	for(i=0;i<(*network)->serverCount;i++)
	{
		if(dest == servers[i].serverID)
		{
			servers[i].cost = cost;
			break;
		}
	}

	// SECOND -- set the costMatrix right
	oldCost = (*costMatrix)[myServerID-1][dest-1];

	if((*nextHop)[dest-1] == dest)
	{
		// can do this only if the nextHop is that very same server
		(*costMatrix)[myServerID-1][dest-1] =  cost;
		if (cost == INFINITY) (*nextHop)[dest-1] = UNDEFINED;
	}
	else // we were reaching it thru some other server
	{
		#if DEBUG
		printf("I should have been called\n");
		#endif
		if(cost < (*costMatrix)[myServerID-1][dest-1])
		{
			(*costMatrix)[myServerID-1][dest-1] = cost;
			(*nextHop)[dest-1] = dest;
		}
	}


	#if DEBUG
	printf("taking care of the ripple effect of an update\n");
	#endif

	int myNeighbour = FALSE;
	int k =0;
	/*
	 * When my path to other servers are thru this neighbour in consideration
	 * i will have to update the cost of such a path
	 */
	for(i=0;i<(*network)->serverCount;i++)
	{
		/*
		 * check if the neighbour in focus is a next hop to some other destination
		 * other than itself ofcourse
		 */
		if((*nextHop)[i] == dest && i != dest-1)
		{
			myNeighbour = isNeighbour(neighbourList,(*network)->neighbourCount,i+1);
			if(cost != INFINITY)
			{
				if(myNeighbour) // I is my neighbour and me-dest link is !INFINITY
				{
					for(k=0;k<(*network)->neighbourCount;k++)
					{
						if(adjacentEdges[k].endNode == i+1)
						{
							if((*costMatrix)[myServerID-1][i]-oldCost+cost > adjacentEdges[i].linkCost)
							{
								(*costMatrix)[myServerID-1][i] = adjacentEdges[k].linkCost;
								(*nextHop)[i] = i+1;
							}
							else
							{
								(*costMatrix)[myServerID-1][i] =  (*costMatrix)[myServerID-1][i]-oldCost+cost;
							}

						}
					}
				}
				else // then the following math will make sense
				{
					// the chance of oldCost being infinity is taken care above :)
					(*costMatrix)[myServerID-1][i] = (*costMatrix)[myServerID-1][i]-oldCost+cost ;
					// post this the DV_CALC will take care of setting the paths right :) !!
					#if DEBUG
					printf("Recalculate new cost between %d and %d to %d",myServerID,i+1,(*costMatrix)[myServerID-1][i]);
					#endif
				}

			}
			else if(myNeighbour)// new cost is infinity  -- and 'I' is neighbour
			{
				#if DEBUG
				printf("%d is a neighbour prev routed thru %d\n",i+1,dest);
				#endif
				// ServerID = i+1 is a my neighbour whose path is routed thru another neighbour dest !!!
				for(j=0;j<(*network)->neighbourCount;j++)
				{
					if(adjacentEdges[j].endNode == i+1)
					{
						// if that direct edge to I is also infinity
						if(adjacentEdges[j].linkCost == INFINITY)
						{
							(*nextHop)[i] = UNDEFINED;
							(*costMatrix)[myServerID-1][i] = INFINITY;
						}
						else // if the phy link/edge is not INFINITY then
						{
							(*nextHop)[i] = i+1; // setting nextHop of I to I itself
							(*costMatrix)[myServerID-1][i] = adjacentEdges[j].linkCost; // and cost to that cost
						}
					}
				}
			}
			else // new cost is infinity and 'I' is not neighbour -- should force DV_CALC to find a new path to reach I (A non-neighbour)
			{
				#if DEBUG
				printf("%d is not a neighbour so setting path cost to INF and hop to NONE\n",i+1);
				#endif
				(*nextHop)[i] = UNDEFINED;
				(*costMatrix)[myServerID-1][i] = INFINITY;
			}
		}

	}
	return SUCCESS;
}
/*=============================================================================
 * Function Name: isNeighbour
 * Function Desc: checks if the given serverID is that of a valid neighbour or not
 * Parameters   : neighbour* neighbourList,int neighbourCount,
 * 				unsigned short int targetServerID
 * Return value : TRUE / FALSE
 ============================================================================*/
int isNeighbour(neighbour* neighbourList,int neighbourCount,unsigned short int targetServerID)
{
	int i =0;
	for(i=0;i<neighbourCount;i++)
	{
		// has to be the neighbour list and also one not to be ignored
		if(neighbourList[i].serverID == targetServerID && neighbourList[i].ignore == FALSE)
		{
			#if DEBUG
			printf("Hey this guy %d is a neighbour\n",targetServerID);
			#endif

			return TRUE; // yes neighbour
		}
	}
	return FALSE; // sorry not a neighbour
}


/*=============================================================================
 * Function Name: help
 * Function Desc: Lists the different commands at that are at the users disposal
 * Parameters   : void
 * Return value : SUCCESS
 ============================================================================*/
int help()
{
	printf("\tupdate <server-ID1> <server-ID2> <Link Cost>\n");
	printf("\t\tserver-ID1, server-ID2: The link for which the cost is being updated.\n \
\t\tLink Cost: The new link cost between the source and the destination server.\n \
\t\tNote: This command should be issued to both server-ID1 and server-ID2\n\n");
	printf("\tstep - Sends routing update to neighbors (triggered/force update)\n\n");
	printf("\tpackets  - Displays the number of distance vector packets this server has received since the last invocation\n\n");
	printf("\tdisplay  - Displays the current routing table in sorted order\n\n");
	printf("\tdisbale <server-ID> - Disables the link to a given server. Doing this “closes” the connection to a given server with\n\n");
	printf("\tcrash - Simulates a server crash\n\n");
	printf("\texit - Exit application\n\n");
	return SUCCESS;
}
/*=============================================================================
 * End of File
 *============================================================================*/

#if 0
// third -- need to reconfigure routing paths only if old cost was not INFINITY
/*
if(oldCost == INFINITY)
{
	#if 1
	printf("The old cost was infinity\n");
	#endif


	for(i=0;i<(*network)->neighbourCount;i++)
	{

	}

	// need to check if it was routed thru someone else
	//nextHop

	return SUCCESS; // cos the routing table has to be updates
}
*/
#endif
