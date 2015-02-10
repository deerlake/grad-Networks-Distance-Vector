CC=gcc -g -w
CFLAGS=-I include/
OBJS=rajaramr_proj2.o topologyFileReader.o serverOps.o routingTableOps.o commandOperations.o
SRC=src/

server:	rajaramr_proj2.o topologyFileReader.o serverOps.o routingTableOps.o commandOperations.o
	$(CC) -o server $(CFLAGS) $(OBJS)
	rm -rf $(OBJS)

rajaramr_proj2.o: 
	$(CC) -c $(SRC)rajaramr_proj2.c $(CFLAGS) 
	
topologyFileReader.o:
	$(CC) -c $(CFLAGS) $(SRC)topologyFileReader.c

serverOps.o:
	$(CC) -c $(CFLAGS) $(SRC)serverOps.c

routingTableOps.o:
	$(CC) -c $(CFLAGS) $(SRC)routingTableOps.c

commandOperations.o:
	$(CC) -c $(CFLAGS) $(SRC)commandOperations.c

clean:
	rm -rf server
	rm -rf $(OBJS)
