Contents:

1. readme.txt -- This very document
2. assignment_DistanceVector.pdf -- The assignment given

########### <code> folder

1. src (Folder) -- Has all the source files
2. include (Folder) -- Has all the header files
3. makefile -- run the make command to get executable "server"
4. makefile_32bit -- makefile for 32 bit machine
5. topofiles (Folder) -- has all the topology files that i used to test the code

########### <report> folder

1. MNC 589 Project 2 Documentation & Reprot.pdf

########### <screen_shots> folder

Screen shots of the application at work

1. Application startup.png
2. Step+display+timeout+crash.png


How to run the application ?
*****************************************************************************
$ make
$ ./server -t topoFiles/topofile_<x>.txt -i <updateInterval>
****************************************************************************

What to expect ?
	On execution the program shall display the following:
	> my server id = <id>
	> <routing table based on file read>
	DEST	NextHop	Cost
	1		X		X
	2		X		X
	3		X		X
	4		X		X
	5		X		X
