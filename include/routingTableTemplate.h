/*
 * routingTableTemplate.h
 *
 *  Created on: Oct 23, 2013
 *      Author: dev
 */

#ifndef ROUTINGTABLETEMPLATE_H_
#define ROUTINGTABLETEMPLATE_H_

// The following structure defines a routing table record
// The table shall be of structure of type routingTableRecord
typedef struct routingTableRecord
{
	int destServerID;
	int nextHopServerID;
	unsigned short int costOfPath; // cost of the total path from src to destination
}routingTableRecord;

#endif /* ROUTINGTABLETEMPLATE_H_ */
