/*
Name: aesdsocket.c
Description:  socket based server program
Author: Isha Sharma
Reference: 
	https://beej.us/guide/bgnet/html/#what-is-a-socket
	https://github.com/thlorenz/beejs-guide-to-network-samples
	https://www.gnu.org/software/libc/manual/html_node/Termination-in-Handler.html
    http://cslibrary.stanford.edu/103/LinkedListBasics.pdf

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

#include <pthread.h>
#include <sys/queue.h>
#include <time.h> 

//Defines 
#define PORT        "9000"
#define USE_AESD_CHAR_DEVICE (1) 

#if (USE_AESD_CHAR_DEVICE == 1)
	#define LOG_FILE "/dev/aesdchar"
#else
	#define LOG_FILE "/var/tmp/aesdsocketdata"
#endif

#define BACKLOG     (10) 
#define BUFFER_LEN  (1024)

//Variables
int socket_fd;		//for server 
int client_fd; 	//for client
int logfile_fd; 	//for output log file 
volatile sig_atomic_t fatal_error_in_progress = 0;
pid_t pid;
bool daemon_flag = false;

struct sockaddr_storage client_address; // connector's address information
socklen_t client_addrlen = sizeof(struct sockaddr_storage);
ssize_t recv_bytes = 0;
char buf[BUFFER_LEN];
char read_buf[BUFFER_LEN];
ssize_t bytes_read =0;
ssize_t bytes_sent =0;
char s[INET6_ADDRSTRLEN];

int status;

//structure for thread
typedef struct 
{
    pthread_t thread_id;
    pthread_mutex_t *thread_mutex;
    bool complete_flag;
    int client_fd;
    struct sockaddr_storage *client_addr;
}thread_data_t;

//structure linked list
typedef struct slist_node
{
    thread_data_t thread_data;
    SLIST_ENTRY(slist_node) entries;
} slist_node_t;

#if (USE_AESD_CHAR_DEVICE != 1)
//structure for timestamp in thread
typedef struct
{
    pthread_t thread_id;
    pthread_mutex_t *thread_mutex;
    int intervalSecs;//time interval in seconds
}thread_timestamp_t;
#endif

//for linkedlist
SLIST_HEAD(head_s, slist_node) head;
slist_node_t * new_node = NULL;

//for mutex
pthread_mutex_t mutex;

#if (USE_AESD_CHAR_DEVICE != 1)
//for timestamp
thread_timestamp_t thread_timestamp;
#endif

//smooth cleaup and termination 
void cleanup(void)
{	
    syslog(LOG_INFO, "cleaning up \n");
    	//pthread 
    	#if (USE_AESD_CHAR_DEVICE != 1)
	pthread_join(thread_timestamp.thread_id, NULL);
	#endif
    //mutex
    pthread_mutex_destroy(&mutex);
	close(socket_fd);
	close(client_fd);
	close(logfile_fd);
    //unlink(LOG_FILE);
	shutdown(socket_fd, SHUT_RDWR);
	#if (USE_AESD_CHAR_DEVICE != 1)
	remove(LOG_FILE);
	#endif

    //free queue 
    while (!SLIST_EMPTY(&head))
    {
        new_node = (slist_node_t *)SLIST_FIRST(&head);
        SLIST_REMOVE(&head, new_node, slist_node, entries);
        free(new_node);
        new_node = NULL;
    }


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
			shutdown(socket_fd, SHUT_RDWR);
            close(socket_fd);
	        close(client_fd);
	        close(logfile_fd);
            remove(LOG_FILE);
            syslog(LOG_INFO,"cleaned up\n");

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

//function to make deamon
void make_deamon(void)
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

//function to open a stream socket bound to port 9000
int open_socket()
{
    struct addrinfo hints, *servinfo;

    int yes=1;

    //hints structure data for getaddrinfo
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; //dont care for ipv4/6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
    hints.ai_protocol = 0;              /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

    if((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) 
    {
        syslog(LOG_ERR, "getaddrinfo failed \n");  
        return -1;
    }
        
    if (servinfo == NULL)
    {
        syslog(LOG_ERR, "malloc for serverinfo failed \n");
        return -1;
    }

    if ((socket_fd = socket(servinfo->ai_family, servinfo->ai_socktype,servinfo->ai_protocol)) == -1) 
    {
        syslog(LOG_ERR,"Socket creation failed \n");
        return -1;
    }
    syslog(LOG_INFO, "socket creation sucess \n");

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
    {
        syslog(LOG_ERR,"setsockopt failed \n");
        return -1;       
    }
    syslog(LOG_INFO, "setsockopt sucess \n");
            
    if (bind(socket_fd, servinfo->ai_addr, sizeof(struct sockaddr)) == -1) 
    {
        close(socket_fd);
        syslog(LOG_ERR,"server bind failed : %s \n", strerror(errno));
        return -1; 
    }
    syslog(LOG_INFO, "bind sucess \n");

            
    freeaddrinfo(servinfo);
    servinfo = NULL;

    return 0;
}
#if (USE_AESD_CHAR_DEVICE != 1)
//function for timestamp thread to log timestamp
void *timestamp_threadFunc(void *thread_param)
{
    thread_timestamp_t *threadDataTimeStamp = (thread_timestamp_t *)thread_param;
    
    time_t current_time ;
    struct tm *local_time ;
    char time_stamp_format[50];
    struct timespec timeSpec;
    int time_length;

    syslog(LOG_INFO, "Timestamp thread started\n");

    while(1)
    {
        // monotonic current time mode
        if (clock_gettime(CLOCK_MONOTONIC, &timeSpec))
        {
            syslog(LOG_ERR, "clock_gettime failed \n");
        }
    
        //add interval
        timeSpec.tv_sec += threadDataTimeStamp->intervalSecs;

        //sleep for interval
        if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &timeSpec, NULL))
        {
            syslog(LOG_ERR, "clock_nanosleep failed\n");
        }

        //get current time
        time(&current_time);
        local_time = localtime(&current_time);

        //format current time
        time_length = strftime(time_stamp_format, sizeof(time_stamp_format), "timestamp: %Y, %b %d, %H:%M:%S\n", local_time);

        //lock mutex
        if (pthread_mutex_lock(threadDataTimeStamp->thread_mutex) == -1)
        {
            syslog(LOG_ERR, "mutex lock for timestamp failed \n");
        }

        //write data to file
        status = write(logfile_fd, time_stamp_format, time_length);
        if (status == -1)
        {
            syslog(LOG_ERR, "timestamp write to file failed: %s\n", strerror(errno));
        }


        //unlock mutex
        if (pthread_mutex_unlock(threadDataTimeStamp->thread_mutex) == -1)
        {
            syslog(LOG_ERR, "mutex unlock for timestamp failed \n");
        }

    }
}
#endif
#if (USE_AESD_CHAR_DEVICE != 1)
//function to setup timestamp
void timestamp_setup (void)
{
    thread_timestamp.thread_mutex = &mutex;
    thread_timestamp.intervalSecs =10;

    status = pthread_create(&(thread_timestamp.thread_id), NULL, \
                                timestamp_threadFunc, &(thread_timestamp));
    if(status == -1)
    {
        syslog(LOG_ERR, "Timestamp Thread creation failed");
    }

}
#endif

void *recv_send_thread(void *thread_param)
{
    memset(buf, 0, BUFFER_LEN);
    memset(read_buf, 0, BUFFER_LEN);

    thread_data_t *thread_data = (thread_data_t*)thread_param;
    
    /*STEP 6:
        Logs message to the syslog “Accepted connection from xxx” 
    */
    inet_ntop(thread_data->client_addr->ss_family, get_in_addr((struct sockaddr *)&(thread_data->client_addr)), s, sizeof s);  // s is the ip address
    syslog(LOG_INFO,"Accepted connection from %s",s);
                      
    /* STEP 7: 
        Receives data over the connection and appends to file /var/tmp/aesdsocketdata, creating this file if it doesn’t exist.
    */
    //lock mutex
    status = pthread_mutex_lock(thread_data->thread_mutex);
    if(status == -1)
    {
        syslog(LOG_ERR,"rec and send thread mutex lock failed\n");
    }
    
    //move to the start of the file
    off_t seekset_to_beg = lseek(logfile_fd, 0, SEEK_SET);
    if(seekset_to_beg == -1)
    {
        syslog(LOG_ERR,"lseek failed \n");
        
    }

    do
    {
            
        recv_bytes = recv(thread_data->client_fd, buf, BUFFER_LEN, 0);
        if (recv_bytes == -1)
        {
            syslog (LOG_ERR, "receving of bytes failed \n");
           
        }
        
        //wrie to log file
        status = write (logfile_fd, buf, recv_bytes);
        if (status == -1)
        {
            syslog(LOG_ERR,"logging failed \n");
           
        }

    } while(buf[recv_bytes-1] != '\n');
    
    //unlock mutex
    status = pthread_mutex_unlock(thread_data->thread_mutex);
    if(status == -1)
    {
        syslog(LOG_ERR,"rec and send thread mutex unlock failed\n");
    }

    /* STEP 8:
        Returns the full content of /var/tmp/aesdsocketdata to the client as soon as the received data packet completes.
    */
    //lock mutex
    status = pthread_mutex_lock(thread_data->thread_mutex);
    if(status == -1)
    {
        syslog(LOG_ERR,"rec and send thread mutex lock failed\n");
    }
    
    //move to the start of the file
    seekset_to_beg = lseek(logfile_fd, 0, SEEK_SET);
    if(seekset_to_beg == -1)
    {
    syslog(LOG_ERR,"lseek failed \n");
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
        
        if(bytes_read > 0)
        {
            bytes_sent = send (thread_data->client_fd, read_buf, bytes_read, 0);
            if (bytes_sent == -1)
            {
                syslog(LOG_ERR,"send failed \n");
                break;
            }
        }
        //syslog (LOG_INFO,"sent to client\n");
        //syslog (LOG_INFO,"Bytes sent : %ld",bytes_sent);
        //syslog (LOG_INFO,"sent read Buffer : \n%s",read_buf);
    
    }while(read_buf[bytes_sent-1] != '\n'&& bytes_read >1);   

    //unlock mutex
    status = pthread_mutex_unlock(thread_data->thread_mutex);
    if(status == -1)
    {
        syslog(LOG_ERR,"rec and send thread mutex unlock failed\n");
    }

    close(thread_data->client_fd);
    syslog(LOG_INFO,"Closed connection from %s",s);

    thread_data->complete_flag = true;
    return thread_param;
}

//entry point
int main(int argc, char *argv[])
{
    //init ll
	SLIST_INIT(&head);
    int pthread_return;

    void * thread_return = NULL;

	//for syslogging throughout
	openlog("A5 Socket:",0,LOG_USER);
    syslog(LOG_INFO,"starting socket\n");

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
	
	//initialise mutex
	status = pthread_mutex_init(&mutex, NULL);
	if(status != 0)
	{
		syslog(LOG_ERR, "mutex init failed \n");
	}
	
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
	

            /*STEP 1: 
                Open a stream socket bound to port 9000, failing and returning -1 if any of the socket connection steps fail
            */
           /* STEP 2:
                bind
                loop through all the results and bind to the first we can
            */
           /* STEP 3:
                free malloc for addr struct
                freeaddrinfo()
            */
            if (open_socket() == -1)
            {
                syslog(LOG_ERR, "open socket failed \n");
            }
                
            /* STEP 4:
                if deamon specified, initialise it
                fork->create a new sid->change directory to root-> close and redirect stdin/err/out to /dev/null
            */
            if(daemon_flag)
            {
                make_deamon();
            }
                
            /* STEP FOR A6: 
                set up timestamp
            */
#if (USE_AESD_CHAR_DEVICE != 1)
		    ret = timestamp_setup();
		    if(ret == RET_ERROR)
		    {
			return -1;
		    }
#endif

        /* STEP 5:
            Listens for and accepts a connection
        */
        status = listen(socket_fd, BACKLOG);// how many pending connections queue will hold
        if (status == -1)
        {
            syslog(LOG_ERR, "listen failed \n");
        }

        while(1)
        {
            client_fd = accept(socket_fd, (struct sockaddr *)&client_address, &client_addrlen);
            if (client_fd ==-1)
            {
                //syslog(LOG_ERR," accept failed \n");
               
                continue;
            }
            
            /* STEP FOR A6:
            
                1) allocate memory and create a new thread node
                2) insert node in linked list
                3) join thread
            */
            //allocate memory
            new_node = malloc(sizeof(slist_node_t));
            if(new_node == NULL)
            {
                syslog(LOG_ERR,"new node for ll malloc failed");
                return -1;
            }
            new_node->thread_data.thread_mutex = &mutex;
            new_node->thread_data.complete_flag = false;
            new_node->thread_data.client_fd = client_fd;
            new_node->thread_data.client_addr = (struct sockaddr_storage *)&client_address;

            // create new thread
            pthread_return = pthread_create(&(new_node->thread_data.thread_id), NULL, \
                                    recv_send_thread, &(new_node->thread_data));
            if(pthread_return == -1)
            {
                syslog(LOG_ERR, "receive and send thread creation failed \n");
                free(new_node);
                return -1;
            }    

            //insert node in linked list
            SLIST_INSERT_HEAD(&head, new_node, entries);
            new_node = NULL;

            // check for thread completion and join thread
            SLIST_FOREACH(new_node, &head, entries)
            {
                if(new_node->thread_data.complete_flag)
                {
                    pthread_return = pthread_join(new_node->thread_data.thread_id,&thread_return);
                    if(pthread_return == -1)
                    {
                        syslog(LOG_ERR, "slist thread join failed \n");
                        return -1;
                    }
                    if(thread_return == NULL)
                    {
                        return -1;
                    }
                    syslog(LOG_INFO, "Thread join %ld",new_node->thread_data.thread_id);
                }
            }      
        }
           
    cleanup(); //cleanup after end
    closelog();
    return 0;
}

