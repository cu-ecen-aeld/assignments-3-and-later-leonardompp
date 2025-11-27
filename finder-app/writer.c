#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>

int main(int argc, char* argv[]){
    // LOG_CONS option here was kind of arbitrary, but seems suitable. Was not specified
    openlog("writer_log", LOG_CONS, LOG_USER);

    // Requirement: needs two arguments (in addition to the default, executable name argument)
    if(argc != 3){
        syslog(LOG_ERR, "Error: Two arguments required: <WRITEFILE> <WRITESTR>");
        return 1;
    }

    // From instructions: You do not need to make your "writer" utility create directories which do not exist.  You can assume the directory is created by the caller.
    char* writefile = argv[1];
    char* writestr = argv[2];
    
    syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
    FILE *f = fopen(writefile, "w");
    if (f == NULL) {
        syslog(LOG_ERR, "Unable to open file");
        return 1;
    }
    fprintf(f, "%s", writestr);
    fclose(f);

    return 0;
}