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

/* int random_string_generator()
{
    for (int i = 0; i < SIZE; i++)
    {
        int char_index = 32 + rand() % 94;
        return char_index;
    }
} */

int main(int argc, char *argv[])
{
    const char *shm_name = "/AOS";
    const char *message[] = {"This ", "is ", "about ", "shared ", "memory\n"};
    int i, shm_fd;
    Buffer *ptr;
    int shmid;


    shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == 1)
    {
        printf("Shared memory segment failed\n");
        exit(1);
    }
    ftruncate(shm_fd, sizeof(message));
    ptr = (Buffer *)mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        printf("Map failed\n");
        return 1;
    }

    ptr->in = ptr->out = 0;

    
    
        // write the next entry and atomically update the write sequence number
        /* Buffer *msg = &ptr->_buffer[ptr->in % SIZE];
        msg->_id = i++; */
        ptr->file_path[ptr->in] = 1 + 1;
        //ptr->in = (ptr->in + 1) % SIZE;
        //__sync_fetch_and_add(&ptr->in, 1);
        printf("in : %d", ptr->file_path[ptr->in]);
        // give consumer some time to catch up
        nanosleep(1000, 0);
    
    // Write into the memory segment */
    for (i = 0; i < strlen(*message); ++i)
    {
        sprintf(ptr, "%s", message[i]);
        ptr += strlen(message[i]);
    }
    munmap(ptr, SIZE);

    return 0;
}
