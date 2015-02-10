/*
 * neighbourDetails.h
 *
 *  Created on: Nov 3, 2013
 *      Author: dev
 */

#ifndef NEIGHBOURDETAILS_H_
#define NEIGHBOURDETAILS_H_

#include <netinet/in.h>

/*
 * This structure is used to store information about
 * the servers neighbours -- thus has almost all neighbour related
 * 							information
 */
typedef struct neighbour
{
	short int serverID;
	char ipAddress[INET_ADDRSTRLEN];
	int port; // fixme
	int ignore;
	int timeoutCount;
	int disAllowUpdate;
	unsigned short int cost;
	struct sockaddr_in neighbourAddress;
}neighbour;

#endif /* NEIGHBOURDETAILS_H_ */
