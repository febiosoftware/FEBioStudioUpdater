#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <sys/stat.h>

int main(int argc, char* argv[])
{
    if(argc < 3) return -1;

    int start = 2;
    bool dev = false;

    if(strcmp(argv[2], "-d") == 0)
    {
        start++;
        dev = true;
    }

    if((argc - start) % 2 != 0) return -1;

    for(int index = start; index < argc; index += 2)
    {
        std::rename(argv[index], argv[index + 1]);
        
#ifndef WIN32
        chmod(argv[1], S_IRWXU|S_IXGRP|S_IXOTH);
#endif
    }

    char command[100];

    if(dev)
    {
        sprintf(command, "%s --noUpdaterCheck --devChannel", argv[1]);
    }
    else
    {
        sprintf(command, "%s --noUpdaterCheck", argv[1]);
    }

    std::system(command);
}
