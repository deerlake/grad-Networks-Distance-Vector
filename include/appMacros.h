/*
 * appMacros.h
 *
 *  Created on: Oct 22, 2013
 *      Author: dev
 */

#ifndef APPMACROS_H_
#define APPMACROS_H_

/*
 * The source files are peppered with print logs
 * The DEBUG macro is a compiler switch which when set
 * to '1' shall print all the logs :)
 * Default value of DEBUG is '0'
 * To avoid annoying print statements to console
 */
#define DEBUG 0

// a macro that specifies the command line arguments
#define ARGS_COUNT 5

// Following macros specify place holders for commandline args
#define ARGS_APP_NAME 0
#define ARGS_TOPO_ID 1
#define ARGS_TOPO_FILE_NAME 2
#define ARGS_UPDATE_INTER_ID 3
#define ARGS_UPDATE_INTERVAL 4
#define ARGS_SERVER_ID 5 // fixme
// cost
#define UNDEFINED 65535
#define INFINITY 65535

#define MESSAGE_SIZE 1500
#define SUCCESS_STRING "SUCCESS"

#endif /* APPMACROS_H_ */
