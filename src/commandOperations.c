/*=============================================================================
 * File Name: commandOperations.c
 * Project  : cse589_project1
 * Version  : 0.1V
 * Author   : Rajaram Rabindranath (50097815)
 * Created  : October 22nd 2013
 ============================================================================*/

#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <appMacros.h>
#include <string.h>
#include <commandOperations.h>
#include <globalVars.h>

#define MAX_COMMAND_ARGS 4 // maximum argument a user can input on a console (includes command)
#define COMMAND_COUNT 8// total number of commands

// COMMANDS at different indices .. shall be of great help
char* commands[COMMAND_COUNT] = {"UPDATE","STEP","PACKETS","DISPLAY","DISABLE","CRASH","EXIT","HELP"};
char* commandsLower[COMMAND_COUNT] = {"update","step","packets","display","disable","crash","exit","help"};

/*=============================================================================
 * Function Name: commandMaster
 * Function Desc: Takes what the user enters on the console as input and
 * 					parses the input to make sense of the same
 * Parameters   : char* command
 * Return value : FAILURE / SUCCESS
 ============================================================================*/
int commandMaster(char* command)
{
	#if DEBUG
	printf("the console input is %s",command);
	#endif

	#if DEBUG
	printf("the length of command is %d\n",strlen(command));
	#endif

	// strtok modifies the input string therefore copying to another mem location
	char* commandCopy = (char*)malloc(sizeof(char)*(strlen(command)+1));
	strcpy(commandCopy,command);

	#if DEBUG
	printf("I want to find this\n");
	#endif

	int tokenCount = 0, i =0;
	char** tokens = (char**) malloc(sizeof(char*)*MAX_COMMAND_ARGS);
	int commandID=0;

	#if DEBUG
	printf("After allocating values for the tokens\n");
	#endif
	tokens[tokenCount] = strtok(commandCopy," ");

	// check if command is valid
	if(tokenCount == 0)
	{
	   for(commandID=0;commandID<COMMAND_COUNT;commandID++)
	   {
		   if((!(strcmp(tokens[tokenCount],commands[commandID])))||(!(strcmp(tokens[tokenCount],commandsLower[commandID]))))
		   {
			   break;
		   }
	   }

	   // cannot find command in command_library
	   if(commandID == COMMAND_COUNT)
	   {
		   printf("ERROR : Invalid command %s\n",tokens[tokenCount]);
		   free(commandCopy);
		   free(tokens);
		   return INVALID_COMMAND;
	   }
	}

	// get all arguments given & only process 2 args anything more than that discard
	while(tokens[tokenCount] && tokenCount+1 < MAX_COMMAND_ARGS)
	{
		tokens[++tokenCount]=strtok(NULL," ");
	}

	#if DEBUG
	for(i=0;i<=tokenCount;i++)
	{
		printf("user input is %s at %d\n",tokens[i],i);
	}
	#endif


	// call appropriate functions
	if(!commandArgs) free(commandArgs);

	#if DEBUG
	printf("before doing malloc for command args\n");
	#endif

	commandArgs =(char**) malloc(sizeof(char*)*(tokenCount+1));

	#if DEBUG
	printf("After doing malloc for command args\n");
	#endif

	for(i=0;i<=tokenCount;i++)
	{
		if(tokens[i]!=NULL)
		{
			commandArgs[i] = malloc(sizeof(char)*100);
			if(!commandArgs[i]) commandArgs[i] = malloc(sizeof(char)*100);
			if(!commandArgs[i]) printf("Unable to allocate memory\n");
			memcpy(commandArgs[i],tokens[i],strlen(tokens[i])+1);
		}
		else commandArgs[i] = NULL;
	}

	//if(!commandCopy)free(commandCopy);
	//free(tokens);
	return commandID;
}
/*=============================================================================
 * End of File
 *============================================================================*/

