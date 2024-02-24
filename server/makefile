#cross compiler 
CC ?= $(CROSS_COMPILE)gcc

CFLAGS ?= -Wall -Werror -g

EXEC = aesdsocket

default : $(EXEC)
all : $(EXEC)

$(EXEC): $(EXEC).c
	$(CC) $(CFLAGS) -o $(EXEC) $(EXEC).c


clean:
	-rm -rf *.o $(EXEC)
