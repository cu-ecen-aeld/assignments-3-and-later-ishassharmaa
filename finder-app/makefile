#makefile for writer.c

CC = $(CROSS_COMPILE)gcc

# Compiler flags
CFLAGS = -Wall -Werror -g

#output binary file
EXEC = writer

writer: writer.o
	$(CC) $(CFLAGS) -o $(EXEC) writer.o

writer.o: writer.c
	$(CC) $(CFLAGS) -c writer.c


#cleaning target
clean:
	-rm *.o $(EXEC)
