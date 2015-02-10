/*
 * serverDetails.h
 *
 *  Created on: Oct 22, 2013
 *      Author: dev
 */

#ifndef SERVERDETAILS_H_
#define SERVERDETAILS_H_

#include <netinet/in.h>


/**
 * This file has 3 structures templates
 * networkInfo is the parent structure and contains children
 *  >  serverInfo struct -- information about a server as gleaned from the topology file
 *  >  links struct -- information about adjacent edges of this server -- basically links
 * And networkInfo has all the details about the neighbours
 */


// Information about a server on the network -- gleaned from the topology file
typedef struct serverInfo
{
	short int serverID;
	char serverIP[INET_ADDRSTRLEN];
	short int serverPort;
	unsigned short int cost;
	int isNeighbour; // true or false
	struct serverInfo *next;
}serverInfo;

// Adjacent edges connect to the running server -- as gleaned from  the topology file
typedef struct links
{
	int startNode;
	int endNode;
	int linkCost;
}links;


// The parent structure that has the above 2 as children and thereby
// has all the information about the network as gleaned from the topology file
typedef struct networkInfo
{
	short int serverCount;
	short int neighbourCount;
	short int myServerID;
	short int myPort;
	char myIP[INET_ADDRSTRLEN];
	serverInfo* serversOnNetwork;
	links* edges;
}networkInfo;

#endif /* SERVERDETAILS_H_ */
