#include "systemcalls.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>//for system
#include <errno.h>
#include <string.h>


/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
	
	int result;
	
	result = system(cmd);
	
	//false if error
	if(result == -1)
	{
		return false;
	}
	
	//true if sucess
	return true; 
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    
    pid_t pid;
    int ret;
    int status_of_wait;
    
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
	//call for fork
	pid = fork();
	
	if( pid == -1)
	{
		//error fork failed
		//syslog(LOG_DEBUG,"Fork Failed");
		return false;
	}
	else if (pid == 0)
	{
	//call for execv in the child process
		ret = execv(command[0], command);
		if (ret == -1) 
		{
			//perror ("execv");
			exit (EXIT_FAILURE);
		}
	}
	else
	{
	//call for wait in parent
		
		//wait and exit status in status_of_wait
		waitpid(pid,&status_of_wait,0);
		
		//WIFEXITED, returns true if the process terminated normally
		if (WIFEXITED (status_of_wait))
		{
			//check if exit status is non zero
			if(WEXITSTATUS (status_of_wait))
			{
			  return false;
			}
		}
		else 
		{ return false; }
	}
	
	
    va_end(args);

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    
    pid_t pid;
    int ret;
    int status_of_wait;
    
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
	int fd = open(outputfile, (O_RDWR | O_CREAT | O_TRUNC),
			(S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH ));
	
	if ( fd < 0)
	{
		return false;
	}
	
	//call for fork
	pid = fork();
	
	if( pid == -1)
	{
		//error fork failed
		//syslog(LOG_DEBUG,"Fork Failed");
		return false;
	}
	else if (pid == 0)
	{
	//call for execv in the child process
		
		if (dup2(fd, 1) < 0)
		{
			return false;
		}
		close(fd);
		
		ret = execv(command[0], command);
		if (ret == -1) 
		{
			//perror ("execv");
			exit (EXIT_FAILURE);
		}
	}
	else
	{
	//call for wait in parent
		
		//wait and exit status in status_of_wait
		waitpid(pid,&status_of_wait,0);
		
		//WIFEXITED, returns true if the process terminated normally
		if (WIFEXITED (status_of_wait))
		{
			//check if exit status is non zero
			if(WEXITSTATUS (status_of_wait))
			{
			  return false;
			}
		}
		else 
		{ return false; }
	}


    va_end(args);

    return true;
}
