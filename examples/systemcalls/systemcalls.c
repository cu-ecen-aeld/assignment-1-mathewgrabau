#include "systemcalls.h"
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    int result;
    result = system(cmd);

    if (cmd == NULL)
    {
        return result == 0 ? true : false;
    }

    // If the command is not null, then the result of -1 means that there was an error while executing the command.
    if (result == -1)
    {
        perror("system failed(): ");
        return false;
    }

    if (WIFEXITED(result))
    {
        return WEXITSTATUS(result) == 0 ? true : false;
    }

    return false;
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
    bool result = true;

    int i;
    int pid;
    int status;

    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    // Only accept absolute paths.
    if (strlen(command[0]) == 0 || command[0][0] != '/')
    {
        result = false;
    }
    else
    {
        fflush(stdout);
        pid = fork();

        if (pid == -1)
        {
            perror("fork() failed: ");
            result = false;
        }
        else if (pid == 0)
        {
            execv(command[0], command); //&command[1]);
            perror("Child error: ");
            result = false;
        }

        if (result)
        {
            if (waitpid(pid, &status, 0) == -1)
            {
                fprintf(stderr, "waitpid failed: %d\n", status);
                result = false;
            }
            else if (WIFEXITED(status))
            {
                fprintf(stdout, "Appears it worked? status = %d\n", status); 
                result = WEXITSTATUS(status) == 0 ? true : false;
            }
            else if (WIFSIGNALED(status))
            {
                fprintf(stdout, "Signalled: %d\n", WTERMSIG(status)); 
                result = false;
            }
        }
    }

    va_end(args);

    return result;
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
    char * command[count + 1];
    int i;
    int fd;
    int pid;
    int status;
    bool result;

    result = true;

    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = (char*)NULL;

    fd = open(outputfile, 
            O_CREAT | O_WRONLY | O_TRUNC | O_CLOEXEC, 
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1)
    {
        // Just keeping it neater with the earlier return here.
        perror("open() failed: ");
        va_end(args);
        return false;
    }

    // Only accept absolute paths.
    if (strlen(command[0]) == 0 || command[0][0] != '/')
    {
        result = false;
    }
    else
    {
        pid = fork();

        if (pid == -1)
        {
            perror("fork() failed: ");
            result = false;
        }
        else if (pid == 0)
        {
            if (dup2(fd, STDOUT_FILENO) < 0)
            {
                perror("dup2()");    
                result = false;
            }
            else
            {
                execv(command[0], command);
                perror("Child error: ");
                result = false;
            }
        }
        else
        {
            // Close the file in the parent
            close(fd);
            fd = 0;
        }

        if (result)
        {
            if (waitpid(pid, &status, 0) == -1)
            {
                perror("waitpid() failed: ");
                result = false;
            }
            else if (WIFEXITED(status))
            {
                result = WEXITSTATUS(status) == 0 ? true : false;
            }
            else if (WIFSIGNALED(status))
            {
                result = false;
            }
        }
    }

    va_end(args);

    return result;
}
