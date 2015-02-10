/*=============================================================================
 * File Name: routingTablesOps.c
 * Project  : cse589_project2
 * Version  : 0.1V
 * Author   : Rajaram Rabindranath (50097815)
 * Created  : October 22nd 2013
 ============================================================================*/

#include <stdlib.h>
#include <routingTableTemplate.h>
#include <serverDetails.h>
#include <appMacros.h>
#include <error.h>
#include <stdio.h>
#include <neighbourDetails.h>


/*=============================================================================
 * Function Name: distanceVector_Calc
 * Function Desc: Used to calculate the distanceVector using Bellman-ford algo
 * Parameters   :
 * Return value : indicates if the cost of transmission to other servers has
 * 					changed or not (TRUE / FALSE)
 ============================================================================*/
routingTableRecord* contructRoutingTable(networkInfo * network)
{
	routingTableRecord* routingTable = malloc(sizeof(routingTableRecord)*network->serverCount);
	if(!routingTable)
	{
		printf("ERROR : Could not create mem to store routing table\n");
		return NULL;
	}
	serverInfo *servers = network->serversOnNetwork;
	int i = 0;
	for(i=0;i<network->serverCount;i++)
	{
		routingTable[i].destServerID = servers[i].serverID;
		routingTable[i].costOfPath = servers[i].cost;
		if(servers[i].isNeighbour == TRUE) routingTable[i].nextHopServerID = routingTable[i].destServerID;
		else
			routingTable[i].nextHopServerID = UNDEFINED;
	}
	return (routingTable);
}


/*=============================================================================
 * Function Name: updateRoutingTable
 * Function Desc: Updates the routingTable of the server when the costMatrix
 * 					changes
 * Parameters   : int myServerID,int senderServerID,int nodes,
 * 						routingTableRecord** table,unsigned short int** costMatrix
 * Return value : SUCCESS / FAILURE
 ============================================================================*/
int updateRoutingTable(int myServerID,int* nextHop,int nodes,routingTableRecord** table,unsigned short int** costMatrix)
{
	/***
	 * How to determine next hop router
	 * Can only hop to neighbours -- if ignore == TRUE cannot hop to that router
	 * if intermediate is a neighbour then choose intermediate
	 * or choose the sender whose packet caused this update to happen
	 * when manual update / disable happens ---
	 * senderServerID must be 0
	 */
	int i =0, destServerID=0;
	for(;i<nodes;i++)
	{
		destServerID = (*table)[i].destServerID;
		if((*table)[i].costOfPath != costMatrix[myServerID-1][destServerID-1])
		{
			#if DEBUG
			printf("Updating the cost to %d with next hop at %d\n",destServerID,nextHop[destServerID-1]);
			#endif
			(*table)[i].costOfPath = costMatrix[myServerID-1][destServerID-1];
			(*table)[i].nextHopServerID = nextHop[destServerID-1];
		}
	}
	return SUCCESS;
}

/*=============================================================================
 * Function Name: printRoutingTable
 * Function Desc: Outputs the routingTable on to the console
 * Parameters   : routingTable* rec
 * Return value : void
 ============================================================================*/
void printRoutingTable(routingTableRecord * rec,int nodes)
{
	int i =0;
	printf("Dest\tNextHop\tCost\n");
	for(;i<nodes;i++)
	{
		printf("%d\t ",rec[i].destServerID);
		if(rec[i].nextHopServerID == UNDEFINED) printf("NONE\t ");
		else printf("%d\t ",rec[i].nextHopServerID);
		if (rec[i].costOfPath == INFINITY) printf("infinity\t ");
		else printf("%d\t ",rec[i].costOfPath);
		printf("\n");
	}
}
/*=============================================================================
 * End of File
 *============================================================================*/


