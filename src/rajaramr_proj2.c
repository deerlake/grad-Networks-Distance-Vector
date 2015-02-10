/*=============================================================================
 * File Name: rajaramr_proj2.c
 * Project  : cse589_project2
 * Version  : 0.1V
 * Author   : Rajaram Rabindranath (50097815)
 * Created  : October 22nd 2013
 ============================================================================*/

#include <error.h>
#include <appMacros.h>
#include <stdio.h>
#include <serverOps.h>
#include <serverDetails.h>
#include <topologyFileReader.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <commandOperations.h>

#define STARTUP_COMMAND_LENGTH 1024

void usage(char* applicationName);
void getThisMachineIP();
void printNetworkInfo(networkInfo * network);

#define SERVER_COMMAND "server"
#define TOPO_ID "-t"
#define UPDATE_INTERVAL_ID "-i"


#define COMMAND_SIZE 100
#define SERVER_STARTUP_ARGS 5

char* myIP;
char** commandArgs = NULL;

/*=============================================================================
 * Function Name: main
 * Function Desc: The starting point of the application
 * Parameters   : int argc, char** argv
 * Return value : SUCCESS / FAILURE
 ============================================================================*/
int main(int argc, char** argv)
{
	int updateInterval = 0;
	networkInfo* network;
	int i =0;
	//int serverID = 0;

	// wrong number of arguments
	if(argc != ARGS_COUNT)
	{
		printf("ERROR : Insufficient number of arguments entered\n");
		usage(argv[ARGS_APP_NAME]);
		return FAILURE;
	}

	// right command line arguments format
	if(!strcmp(argv[ARGS_TOPO_ID],TOPO_ID) && !strcmp(argv[ARGS_UPDATE_INTER_ID],UPDATE_INTERVAL_ID))
	{
		// get topo info from the file specified
		network = getTopologyInfo(argv[ARGS_TOPO_FILE_NAME]);
		if(!network)
		{
			printf("ERROR : An exception has been encountered while reading the topological file\n");
			printf("ERROR : Going down as a consequence\n");
			return FAILURE;
		}

		updateInterval = atoi(argv[ARGS_UPDATE_INTERVAL]);
		getThisMachineIP();
		//serverID = atoi(argv[ARGS_SERVER_ID]);

		#if DEBUG
		printf("my ip is =%s\n",myIP);
		#endif

		int foundSelf=FALSE;

		// discover self in the topology :)
		for(;i<network->serverCount;i++)
		{
			//if(serverID == network->serversOnNetwork[i].serverID)
			if(!strcmp(myIP,network->serversOnNetwork[i].serverIP))
			{
				//set serverID and port number
				network->myPort = network->serversOnNetwork[i].serverPort;
				network->myServerID = network->serversOnNetwork[i].serverID;
				network->serversOnNetwork[i].cost = 0;
				memcpy(network->myIP,network->serversOnNetwork[i].serverIP,sizeof(network->serversOnNetwork[i].serverIP));

				printf("My server id is =%d\n",network->myServerID);
				foundSelf=TRUE;
				break;
			}
		}

		if(foundSelf==FALSE)
		{
			printf("ERROR : Can't Find myself in the topo file given %s\n",argv[ARGS_TOPO_FILE_NAME]);
			printf("ERROR : Please check the file\n");
			return FAILURE;
		}

		#if DEBUG
		printNetworkInfo(network);
		#endif

		startServer(updateInterval,network);
	}
	else
	{
		printf("ERROR : Wrong identifiers have been used for file name and Update interval\n");
		usage(argv[ARGS_APP_NAME]);
		return FAILURE;
	}

	return SUCCESS;

}

/*=============================================================================
 * Function Name: usage
 * Function Desc: Shall output to the console the application's usage
 * Parameters   : char* appName
 * Return value : void
 ============================================================================*/
void usage(char* appName)
{
	printf("--------------------------------------------------------------------------------\n");
	printf("	Usage: %s -t <topology-file-name> -i <routing-update-interval>\n",appName);
	printf("---------------------------------------------------------------------------------\n");
}

/*=============================================================================
 * Function Name: getThisMachineIP
 * Function Desc: This function finds the IPAddress of the host (server/client)
 * 				  machine
 * Parameters   : void
 * Return value : void
 ============================================================================*/
void getThisMachineIP()
{
	struct ifaddrs *myAddrs, *iterateAddr;
	struct sockaddr_in* ip;

	if(getifaddrs(&myAddrs) != 0)
	{
		perror("ERROR:");
		printf("ERROR : failed to get own IP address\n");
		return;
	}

	iterateAddr = myAddrs;

	// iterate through the address begotten
	while(iterateAddr)
	{
		// THERE IS AN IP ADDRESS & THE INTERFACE IS UP
		if((!iterateAddr->ifa_addr) || ((iterateAddr->ifa_flags & IFF_UP) == 0))
		{
			iterateAddr = iterateAddr->ifa_next;
			continue;
		}
		// extract IP address
		ip = (struct sockaddr_in *)iterateAddr->ifa_addr;
		if(ip->sin_family == AF_INET)
		{
			myIP = (char*) malloc(sizeof(char)*INET6_ADDRSTRLEN);
			inet_ntop(AF_INET, &(ip->sin_addr),myIP,INET6_ADDRSTRLEN);
			//break; the first element is loopback ... but we do not want that ... so we have commented out break
		}

		iterateAddr = iterateAddr->ifa_next;
	}
	free(myAddrs);
}

/*=============================================================================
 * Function Name: printNetworkInfo
 * Function Desc: prints the contents of the network DS
 * Parameters   : void
 * Return value : void
 ============================================================================*/
void printNetworkInfo(networkInfo * network)
{

	printf("======================Printing network info=============================\n");
	printf("The number of servers on the network is =%d\n",network->serverCount);
	printf("The number of neighbours are =%d\n", network->neighbourCount);
	int i =0;
	serverInfo *a = NULL;
	links *b = NULL;

	for(;i<network->serverCount;i++)
	{
		a = (network->serversOnNetwork+i);
		printf("%d %s %d\n", a->serverID, a->serverIP, a->serverPort);
	}

	for(i=0;i<network->neighbourCount;i++)
	{
		b = (network->edges+i);
		printf("%d %d %d\n",b->startNode, b->endNode, b->linkCost);
	}
}
/*=============================================================================
 * End of File
 *============================================================================*/
