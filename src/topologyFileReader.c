/*=============================================================================
 * File Name: topologyFileReader.c
 * Project  : cse589_project2
 * Version  : 0.1V
 * Author   : Rajaram Rabindranath (50097815)
 * Created  : October 22nd 2013
 ============================================================================*/


#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <appMacros.h>
#include <serverDetails.h>
#include <string.h>

#define FILE_BUFFER 1000
#define LINE_NUMBER_COST 8
#define LINE_LENGHT 100
#define OFFSET 2

// function prototypes
int parseTopologyData(networkInfo** network, char* topoData);

/*=============================================================================
 * Function Name: getTopologyInfo
 * Function Desc: Get information from the topology file specified at command
 * 					startup
 * Parameters   : char* filename
 * Return value : A DS of typer networkInfo*
 ============================================================================*/
networkInfo* getTopologyInfo(char* fileName)
{
	char* fileBuffer =  NULL;
	networkInfo *network = NULL;
	FILE *topo =  fopen(fileName,"r+");
	if(!topo)
	{
		printf("ERROR : file named %s does not exists\n",fileName);
		printf("ERROR : going down\n");
		return NULL;
	}

	fileBuffer = (char*)malloc(sizeof(char)*FILE_BUFFER);
	if(!fileBuffer)
	{
		printf("ERROR : Could not allocate buffer to read file\n");
		return NULL;
	}
	memset(fileBuffer,0,FILE_BUFFER);
	// read the topology file
	int readCount = fread(fileBuffer,sizeof(char),FILE_BUFFER,topo);

	network = (networkInfo*) malloc(sizeof(network));

	#if DEBUG
	printf("the number of bytes read were %d\n",readCount);
	#endif

	// parse the data in the file and have it put into "network"
	parseTopologyData(&network,fileBuffer);

	//free(fileBuffer);
	return network;
}


/*=============================================================================
 * Function Name: getTopologyInfo
 * Function Desc: Get information from the topology file specified at command
 * 					startup
 * Parameters   : networkInfo**(to store topo data),
 * 					char* topoData(read from file)
 * Return value : A DS of typer networkInfo*
 ============================================================================*/
int parseTopologyData(networkInfo** network, char* topoData)
{
	// strtok modifies the input string therefore copying to another mem location

	char *tokens=NULL;
	int totalLines=0, servers = 0,i=0 ;
	char* topoDataCopy = malloc(sizeof(char)*(strlen(topoData)));
	if(!topoDataCopy)
	{
		printf("ERROR : could not allocate memory to parse the file\n");
		return FAILURE;
	}
	strcpy(topoDataCopy,topoData);

	tokens = strtok(topoDataCopy,"\n");
	servers = (*network)->serverCount = atoi(tokens);
	tokens = strtok(NULL,"\n");
	(*network)->neighbourCount = atoi(tokens);
	totalLines = OFFSET+servers+atoi(tokens);

	// FIXME shuld be linkedlists
	(*network)->serversOnNetwork = malloc(sizeof(serverInfo)*servers);
	(*network)->edges = malloc(sizeof(links)*(*network)->neighbourCount);

	char** lines =  malloc(sizeof(char*)*(totalLines-OFFSET));
	if(!lines)
	{
		printf("ERROR : could not allocate memory to read lines\n");
		return FAILURE;
	}

	#if DEBUG
	printf("total lines %d\n",totalLines-OFFSET);
	#endif

	/*thanks to too much manipulation of topoDataCopy and some weird quirk of strtok
	have resorted to this*/
	topoDataCopy = malloc(sizeof(char)*(strlen(topoData)));
	strcpy(topoDataCopy,topoData);

	strtok(topoData,"\n"); // moving ahead removing -- server count
	strtok(NULL,"\n"); // moving ahead removing -- neighbour count

	lines[i]=strtok(NULL,"\n");

	// take it apart line by line by using the new line delimiter
	while(lines[i])
	{
		i++;
		if(i >= totalLines-OFFSET)
			break;

		lines[i]=strtok(NULL,"\n");
		#if DEBUG
		printf("the new line is : %s\n", lines[i]);
		#endif
	}


	i = 0;

	// take each line apart using space as a delimiter
	while(i < totalLines-OFFSET-(*network)->neighbourCount)
	{
		if((tokens = strtok(lines[i]," "))!=NULL)
		(*network)->serversOnNetwork[i].serverID = atoi(tokens);
		if((tokens = strtok(NULL," "))!=NULL)
		memcpy((*network)->serversOnNetwork[i].serverIP,tokens,32); // FIXME should be INET_ADDRSTRLEN
		if((tokens = strtok(NULL," "))!=NULL)
		(*network)->serversOnNetwork[i].serverPort = atoi(tokens);

		(*network)->serversOnNetwork[i].cost =  UNDEFINED;
		(*network)->serversOnNetwork[i].isNeighbour = FALSE;
		i++;
	}


	int j =0;

	#if DEBUG
	printf("the number of neighbours =%d \n",(*network)->neighbourCount);
	printf("the number of lines is =%d\n",totalLines);
	#endif

	// the edges and cost -- neighbour data
	while(i < totalLines-OFFSET)
	{
		if((tokens = strtok(lines[i]," "))!=NULL)
			(*network)->edges[j].startNode = atoi(tokens);
		if((tokens = strtok(NULL," "))!=NULL)
			(*network)->edges[j].endNode = atoi(tokens);
		if((tokens = strtok(NULL," "))!=NULL)
			(*network)->edges[j].linkCost = atoi(tokens);
		i++;
		j++;
	}


	serverInfo *a = ((*network)->serversOnNetwork);
	links *b = (*network)->edges;
	int neighbourID = 0;

	// check if neighbour and set cost
	for(j=0;j<(*network)->neighbourCount;j++)
	{
		neighbourID = b[j].endNode;

		#if DEBUG
		printf("my neighbours are %d\n",neighbourID);
		#endif

		for(i=0;i<servers;i++)
		{
			if(neighbourID == a[i].serverID)
			{
				a[i].isNeighbour =  TRUE;
				a[i].cost = b[j].linkCost;
				break;
			}
		}
	}

	//if(topoData) free(topoDataCopy);// -- FIXME
	//if(lines) free(lines); //-- FIXME

	return SUCCESS;
}
