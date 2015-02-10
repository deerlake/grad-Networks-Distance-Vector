/*
 * commandOperations.h
 *
 *  Created on: Oct 22, 2013
 *      Author: dev
 */

#ifndef COMMANDOPERATIONS_H_
#define COMMANDOPERATIONS_H_

#define INVALID_COMMAND 40

#define OP_CODE_UPDATE 0
#define OP_CODE_STEP 1
#define OP_CODE_PACKETS 2
#define OP_CODE_DISPLAY 3
#define OP_CODE_DISABLE 4
#define OP_CODE_CRASH 5
#define OP_CODE_EXIT 6
#define OP_CODE_HELP 7

int commandMaster(char* command);

#endif /* COMMANDOPERATIONS_H_ */
