#makefile for writer.c

#CC = $(CROSS_COMPILE)gcc

# Compiler flags
#CFLAGS = -Wall -Werror -g

#output binary file
EXEC = writer

all: $(EXEC)

$(EXEC): writer.o
	$(CC) -o writer writer.o

writer.o: writer.c
	$(CC) -o writer.o -c writer.c


#cleaning target
clean:
	-rm *.o $(EXEC)
