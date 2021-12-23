#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/mman.h>

#define SIZE 100 * 1000000
//const int SIZE = 100 * 1000000;

typedef struct
{
    char file_path[SIZE];
    int in;
    int out;

} Buffer;

int main(int argc, char *argv[])
{
    const char *shm_name = "/AOS";
    int i, shm_fd;
    Buffer *ptr;
    shm_fd = shm_open(shm_name, O_RDONLY, 0666);
    if (shm_fd == 1)
    {
        printf("Shared memory segment failed\n");
        exit(1);
    }
    ptr = (Buffer*)mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        printf("Map failed\n");
        return 1;
    }
    // shared memory
    printf("%s", (char *)ptr);
    if (shm_unlink(shm_name) == 1)
    {
        printf("Error removing %s\n", shm_name);
        exit(1);
    }
    return 0;
}