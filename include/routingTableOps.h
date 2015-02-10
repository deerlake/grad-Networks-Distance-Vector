/*
 * routingTableOps.h
 *
 *  Created on: Oct 26, 2013
 *      Author: dev
 */

#ifndef ROUTINGTABLEOPS_H_
#define ROUTINGTABLEOPS_H_


#include <serverDetails.h>

routingTableRecord* contructRoutingTable(networkInfo * network);
int printRoutingTable(routingTableRecord * rec,int nodes);
int updateRoutingTable(int myServerID,int* nextHop,int nodes,routingTableRecord** routingTable,unsigned short int** costMatrix);

#endif /* ROUTINGTABLEOPS_H_ */
