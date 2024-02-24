/*
Name: aesdsocket.c
Description:  socket based server program
Author: Isha Sharma
Reference: 
	https://beej.us/guide/bgnet/html/#what-is-a-socket
	https://github.com/thlorenz/beejs-guide-to-network-samples
	https://www.gnu.org/software/libc/manual/html_node/Termination-in-Handler.html
Course: AESD
Date: Feb 22, 2024
*/

// Includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h> 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdbool.h>

//Defines 
#define PORT "9000"
#define LOG_FILE "/var/tmp/aesdsocketdata" 
#define BACKLOG (10) 
#define BUFFER_LEN (1024)

//Variables
int socket_fd;		//for server 
int client_fd; 	//for client
int logfile_fd; 	//for output log file 
volatile sig_atomic_t fatal_error_in_progress = 0;
pid_t pid;

//smooth cleaup and termination 
void cleanup(void)
{	
	close(socket_fd);
	close(client_fd);
	close(logfile_fd);
	shutdown(socket_fd, SHUT_RDWR);
	remove(LOG_FILE);
	exit(EXIT_SUCCESS);
	
}

//signal handler for sigint/sigterm
void signal_handler(int signal_type)
{
	if(signal_type == SIGINT || signal_type == SIGTERM)
	{
		syslog(LOG_INFO,"Caught signal, exiting\n");
		
		//The cleanest way for a handler to terminate the process is to raise the same signal that ran the handler in the first place. 
		/* Since this handler is established for more than one kind of signal, 
		     it might still get invoked recursively by delivery of some other kind
		     of signal.  Use a static variable to keep track of that. 
		*/
		     
		  if (fatal_error_in_progress)
		    raise (signal_type);
		  fatal_error_in_progress = 1;

		  /*STEP:
		  	Gracefully exits when SIGINT or SIGTERM is received
		 */
			cleanup();

		  /* Now reraise the signal.  We reactivate the signal’s
		     default handling, which is to terminate the process.
		     We could just call exit or abort,
		     but reraising the signal sets the return status
		     from the process correctly. 
		  */
		  signal (signal_type, SIG_DFL);
		  raise (signal_type);
	}
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//entry point
int main(int argc, char *argv[])
{
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage client_address; // connector's address information
	socklen_t client_addrlen = sizeof(struct sockaddr_storage);
	char s[INET6_ADDRSTRLEN];
	int yes=1;
	
	int status;
	bool daemon_flag = false;
	
	ssize_t recv_bytes = 0;
	char buf[BUFFER_LEN];
	char read_buf[BUFFER_LEN];
	ssize_t bytes_read =0;
	ssize_t bytes_sent =0;
	
	if (argc == 2)
	{
		if(strcmp(argv[1], "-d") == 0) 
		{
			// daemon mentioned 
			// initialise daemon after getaddrinfo
			daemon_flag = true;
			syslog(LOG_INFO,"Deamon mentioned\n");
		}
	}
	
	//for syslogging throughout
	openlog("A5 Socket:",0,LOG_USER);

	//register signals SIGINT and SIGTERM
	if (signal (SIGINT, signal_handler) == SIG_ERR) 
	{
		syslog(LOG_ERR,"SIGINT handler failed \n");
		exit (EXIT_FAILURE);
	}
	if (signal (SIGTERM, signal_handler) == SIG_ERR) 
	{
		syslog(LOG_ERR,"SIGINT handler failed \n");
		exit (EXIT_FAILURE);
	}

	//open log file
	logfile_fd = open (LOG_FILE,O_RDWR | O_CREAT | O_APPEND , S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH );
	
	if(logfile_fd == -1)
	{
		syslog(LOG_ERR,"Failed to open log file \n");		
	}	

	//hints structure data for getaddrinfo
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; //dont care for ipv4/6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	
	do
	{
	/*STEP 1: 
		Open a stream socket bound to port 9000, failing and returning -1 if any of the socket connection steps fail
	*/
	if((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) 
	{
		syslog(LOG_ERR, "getaddrinfo failed \n");
		break;
    	}
    	
    	if (servinfo == NULL)
    	{
    		syslog(LOG_ERR, "malloc for serverinfo failed \n");
    		break;
    	}
    	
    	/* STEP 2:
    		bind
    		loop through all the results and bind to the first we can
    	*/
    	for(p = servinfo; p != NULL; p = p->ai_next) 
    	{
		if ((socket_fd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) 
		{
		    syslog(LOG_ERR,"Socket creation failed \n");
		    break;
		}
    		//syslog(LOG_INFO, "socket creation sucess \n");
		
		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
		{
		    syslog(LOG_ERR,"setsockopt failed \n");
		    break;
		}
    		//syslog(LOG_INFO, "setsockopt sucess \n");
    		
		if (bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1) 
		{
		    close(socket_fd);
		    syslog(LOG_ERR,"server bind failed \n");
		    break;
		}
		
    	}
    	
    	/* STEP 3:
    		free malloc for addr struct
    		freeaddrinfo()
    	*/
    	freeaddrinfo(servinfo);
    	
    	/* STEP 4:
    		if deamon specified, initialise it
    		fork->create a new sid->change directory to root-> close and redirect stdin/err/out to /dev/null
    	*/
    	if(daemon_flag)
    	{
    		 pid = fork();
    		
    		if(pid <0)
    		{
    			syslog(LOG_ERR,"deamon fork failed \n");
    			exit (EXIT_FAILURE);
    		}
    		else if (pid>0)
    		{
    			// this is the parent, terminate this
    			syslog(LOG_INFO,"Parent terminated\n");
			exit(0);
    		}
    		    		
    		umask(0);
    		//create a new session
    		pid_t sid = setsid();
    		if( sid <0)
    		{
    			syslog(LOG_ERR,"setsid failed");
    			exit(EXIT_FAILURE);
    		}
    		
    		// change dir to root
    		chdir("/");
    		
    		//close fds
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		
		//redirect to dev null
		open("/dev/null", O_RDWR);
		   			
    	}
    	
    	/* STEP 5:
    		Listens for and accepts a connection
    	*/
    	status = listen(socket_fd, BACKLOG);// how many pending connections queue will hold
    	if (status == -1)
    	{
    		syslog(LOG_ERR, "listen failed \n");
    		break;
    	}
    	
    	while(1)
    	{
    		client_fd = accept(socket_fd, (struct sockaddr *)&client_address, &client_addrlen);
    		if (client_fd ==-1)
    		{
    			syslog(LOG_ERR," accept failed \n");
    			continue;
    		}
    		
    		/*STEP 6:
    			Logs message to the syslog “Accepted connection from xxx” 
    		*/
    		inet_ntop(client_address.ss_family, get_in_addr((struct sockaddr *)&client_address),
            s, sizeof s);  // s is the ip address
            	syslog(LOG_INFO,"Accepted connection from %s",s);
            	
            	
	/* STEP 7: 
		Receives data over the connection and appends to file /var/tmp/aesdsocketdata, creating this file if it doesn’t exist.
	*/
            
		
            //move to the start of the file
            off_t seekset_to_beg = lseek(logfile_fd, 0, SEEK_SET);
	    if(seekset_to_beg == -1)
		{
			syslog(LOG_ERR,"lseek failed \n");
			break;
		}

            
            do
            {
            
		recv_bytes = recv(client_fd, buf, BUFFER_LEN, 0);
		if (recv_bytes == -1)
		{
			syslog (LOG_ERR, "receving of bytes failed \n");
			break;
		}
		
		//wrie to log file
		status = write (logfile_fd, buf, recv_bytes);
		if (status == -1)
		{
			syslog(LOG_ERR,"logging failed \n");
			break;
		}

	     } while(buf[recv_bytes-1] != '\n');
            
            /* STEP 8:
           	 Returns the full content of /var/tmp/aesdsocketdata to the client as soon as the received data packet completes.
            */
            
            //move to the start of the file
            seekset_to_beg = lseek(logfile_fd, 0, SEEK_SET);
	    if(seekset_to_beg == -1)
		{
			syslog(LOG_ERR,"lseek failed \n");
			break;
		}
		
            do
            {
		    bytes_read = read (logfile_fd, read_buf, BUFFER_LEN);
		    if (bytes_read == -1)
		    {
		    	syslog(LOG_ERR,"logging file read failed \n");
		    	break;
		    }
		    //syslog (LOG_INFO,"read from log\n");
		    //syslog (LOG_INFO,"Bytes Read : %ld",bytes_read);
		    //syslog (LOG_INFO,"Read Buffer : \n%s",read_buf);
		    
		    bytes_sent = send (client_fd, read_buf, bytes_read, 0);
		    if (bytes_sent == -1)
		    {
		    	syslog(LOG_ERR,"send failed \n");
		    	break;
		    }
		    //syslog (LOG_INFO,"sent to client\n");
		    //syslog (LOG_INFO,"Bytes sent : %ld",bytes_sent);
		    //syslog (LOG_INFO,"sent read Buffer : \n%s",read_buf);
		    
            }while(read_buf[bytes_sent-1] != '\n'&& bytes_read >1);
            
            close(client_fd);
            syslog(LOG_INFO,"Closed connection from %s",s);
            
            }
           
    	}while(0);//exit	
    	cleanup(); //cleanup after end
    	closelog();
}











