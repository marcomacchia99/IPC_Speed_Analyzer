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
#include <semaphore.h>

#define CIRCULAR_size 10 * 1000
#define BLOCK_NUM 100

const char *shm_name = "/AOS";
int shm_fd;
char *ptr;
int buffer_index = 0;

sem_t *mutex;
sem_t *not_empty;
sem_t *not_full;

pid_t producer_pid;

//variables for select function
struct timeval timeout;
fd_set readfds;

int size;

void receive_array(char buffer[])
{

    int block_size = (CIRCULAR_size / BLOCK_NUM) + (CIRCULAR_size % BLOCK_NUM != 0 ? 1 : 0);
    int cycles = size / block_size + (size % block_size != 0 ? 1 : 0);
    for (int i = 0; i < cycles; i++)
    {
        //read random string from producer
        char segment[block_size];

        if (i % BLOCK_NUM == 0 && i > 0)
        {
            ptr -= block_size * BLOCK_NUM;
        }

        sem_wait(not_empty);
        sem_wait(mutex);

        if (i == cycles - 1)
        {

            int j = 0;
            while ((i * block_size + j) < size)
            {
                buffer[i * block_size + j] = *ptr;
                ptr++;
                j++;
            }
        }
        else
        {
            for (int j = 0; j < block_size; j++)
            {
                buffer[i * block_size + j] = *ptr;
                ptr++;
            }
        }

        sem_post(mutex);
        sem_post(not_full);
    }
    // FILE *file = fopen("cons.txt", "w");
    // fprintf(file, "%s", buffer);
    // fflush(file);
    // fclose(file);
}

int main(int argc, char *argv[])
{

    //getting size from console
    if (argc < 2)
    {
        fprintf(stderr, "Consumer - ERROR, no size provided\n");
        exit(0);
    }
    size = atoi(argv[1]) * 1000000;

    //increasing stack limit to let the buffer be instantieted correctly
    struct rlimit limit;
    limit.rlim_cur = 105 * 1000000;
    limit.rlim_max = 105 * 1000000;
    setrlimit(RLIMIT_STACK, &limit);

    char buffer[size];

    char *fifo_shared_producer_pid = "/tmp/shared_producer_pid";
    mkfifo(fifo_shared_producer_pid, 0666);
    int fd_pid = open(fifo_shared_producer_pid, O_RDONLY);
    int sel;
    do
    {
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;
        FD_ZERO(&readfds);
        //add the selected file descriptor to the selected fd_set
        FD_SET(fd_pid, &readfds);
        sel = select(FD_SETSIZE + 1, &readfds, NULL, NULL, &timeout);
    } while (sel <= 0);
    read(fd_pid, &producer_pid, sizeof(producer_pid));
    close(fd_pid);
    unlink(fifo_shared_producer_pid);
    sleep(1);

    if ((shm_fd = shm_open(shm_name, O_RDONLY, 0666)) == -1)
    {
        perror("consumer - shm_open failure");
        exit(1);
    }

    if ((ptr = mmap(NULL, CIRCULAR_size, PROT_READ, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
        perror("consumer - map failed");
        exit(1);
    }

    //initialize circular buffer semaphores
    if ((mutex = sem_open("mutex", 0, 0644, 1)) == MAP_FAILED)
    {
        perror("consumer - sem_open failed");
        exit(1);
    }
    if ((not_full = sem_open("not_full", 0, 0644, BLOCK_NUM)) == MAP_FAILED)
    {
        perror("consumer - sem_open failed");
        exit(1);
    }
    if ((not_empty = sem_open("not_empty", 0, 0644, 0)) == MAP_FAILED)
    {
        perror("consumer - sem_open failed");
        exit(1);
    }

    receive_array(buffer);

    //transfer complete. Sends signal to notify the producer
    kill(producer_pid, SIGUSR1);

    if (shm_unlink(shm_name) == 1)
    {
        printf("Error removing %s\n", shm_name);
        exit(1);
    }

    sem_close(mutex);
    sem_unlink("mutex");
    sem_close(not_empty);
    sem_unlink("not_empty");
    sem_close(not_full);
    sem_unlink("not_full");

    munmap(ptr, CIRCULAR_size);
    close(shm_fd);

    return 0;
}