/*
 * messageTemplate.h
 *
 *  Created on: Oct 22, 2013
 *      Author: dev
 */

#ifndef MESSAGETEMPLATE_H_
#define MESSAGETEMPLATE_H_


#include <netinet/in.h>


/*
 * This structure is used to read the update fields
 * Within the message that has been received from the other servers
 */
typedef struct messageNugget
{
	char serverIP[INET_ADDRSTRLEN];
	short int serverPort;
	short int serverID;
	unsigned short int cost;

}msgNugget;

/*
 * This structure is used to read the message coming
 * from other servers -- this structure specifically
 * is used to parse the header of the message
 */
typedef struct messageFormat
{
	short int numUpdateFields;
	short int serverPort;
	char serverIP[INET_ADDRSTRLEN];
}msgFormat;

#endif /* MESSAGETEMPLATE_H_ */
