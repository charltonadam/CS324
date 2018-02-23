#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char**argv)
{
    int descriptor = open("JelloWorldFile.txt", O_RDWR);

    char* address = mmap(NULL, sizeof(char*) * 20, PROT_READ | PROT_WRITE, MAP_PRIVATE, descriptor, 0);

    address[0] = 'J';

    write(descriptor, address, sizeof(char*) * 20);

}
