CC      = gcc
CFLAGS  = -O 
LDFLAGS = 
LIBS    = -lncurses -ltermcap
INC	= 
BIN     = .

SRCS 	= mp3.c queues.c

all:    mp3

clean:  
	rm -f mp3 mp3.report mp3.s

mp3:	
	$(CC) $(CFLAGS) $(SRCS) -o $(BIN)/mp3 $(LIBS) $(LDFLAGS) $(INC)

