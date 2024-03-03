#cross compiler 
CC = $(CROSS_COMPILE)gcc

CFLAGS = -Wall -Werror -g
LDFLAGS ?= -pthread -lrt

EXEC = aesdsocket

default : $(EXEC)
all : $(EXEC)

$(EXEC): $(EXEC).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(EXEC) $(EXEC).c


clean:
	-rm -rf *.o $(EXEC)
