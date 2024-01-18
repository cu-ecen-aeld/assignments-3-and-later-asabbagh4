#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>

int main(int argc, char *argv[]) 
{
    FILE *fp;
    char *filename;
    char *text;

    // Check for correct argument count
    if (argc != 3) 
    {
        fprintf(stderr, "Usage: %s <file> <string>\n", argv[0]);
        return 1;
    }

    // Open syslog
    openlog("writer", LOG_PID|LOG_CONS, LOG_USER);
    filename = argv[1];
    text = argv[2];

    // Log writing operation
    syslog(LOG_DEBUG, "Writing %s to %s", text, filename);

    // Try to open file for writing
    fp = fopen(filename, "w");
    if (fp == NULL) 
    {
        syslog(LOG_ERR, "Error opening file %s", filename);
        closelog();
        return 1;
    }

    // Write text to file
    if (fprintf(fp, "%s", text) < 0) 
    {
        syslog(LOG_ERR, "Error writing to file %s", filename);
        fclose(fp);
        closelog();
        return 1;
    }

    // Close file and syslog
    fclose(fp);
    closelog();

    return 0;
}
