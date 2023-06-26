// writer
// Writes the simple argument.
//

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>

static void open_logs()
{
    // Using default settings for the logging (name, etc.).
    openlog(NULL, LOG_CONS, LOG_SYSLOG); 
}

int main(int argc, char** argv)
{
    int rc, fd;
    char *content, *filepath;

    rc = 0;

    open_logs(); 

    if (argc < 2)
    {
        syslog(LOG_ERR, "usage: writer [output dir] [content]");
        rc = 1;
    }
    else
    {
        filepath = argv[1];
        content = argv[2];

        if (strlen(filepath) <= 0)
        {
            syslog(LOG_ERR, "Invalid directory argument.");
            rc = 1;
        }

        if (strlen(content) <= 0)
        {
            syslog(LOG_ERR, "Invalid filename argument");
            rc = 1;
        }

        if (rc == 0)
        {
            fd = creat(filepath, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd > 0)
            {
                write(fd, content, (strlen(content) + 1) * sizeof(char));
            }
            else
            {
                syslog(LOG_ERR, "Open failed: %m");
            }
        }
    }

    return rc;
}

