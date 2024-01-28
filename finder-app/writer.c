/*
Name: write.c
Author: Isha Sharma
Reference:  LSP chapter 2 and A1
Course: AESD
Date: Jan 28, 2024
*/


//from tb
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//for writing
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int fd;//file
	ssize_t wr_in_file;//values to write in file
	
	//openlog with user facility
	openlog(NULL,0,LOG_USER);
	
	//check input params
	if (argc != 3) 
	{
        syslog(LOG_ERR,"Error: Invalid number of arguments : %d\n",argc);
        return 1;
    	}
    	
    	//parameters into varibales
    	char *writefile = argv[1];
    	char *writestr  = argv[2];
    	
    	
    	//open the file
    	fd = open (writefile, O_RDWR | O_CREAT | O_TRUNC, //read-write,create if doesnt exist, 
    			S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH); //from tb
	if (fd == -1)
	{
		syslog(LOG_ERR,"Error: Cannot open file : %s\n",writefile);
		return 1;
	}
    	
    	//write in file
    	wr_in_file =  write(fd, writestr, strlen (writestr));
    	if (wr_in_file == -1)
	{
		syslog(LOG_ERR,"Error: Cannot write in file");
		return 1;
	}

    	//debug syslog for sucessful write
    	syslog(LOG_DEBUG,"Writing %s to %s",writestr,writefile);

    	return 0;
}

