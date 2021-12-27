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

struct timeval start_time, stop_time;
int flag_transfer_complete = 0;
int transfer_time;

sem_t *mutex;
sem_t *not_empty;
sem_t *not_full;

int size;
int mode;

void transfer_complete(int sig)
{
    if (sig == SIGUSR1)
    {
        gettimeofday(&stop_time, NULL);
        //calculating time in milliseconds
        transfer_time = 1000 * (stop_time.tv_sec - start_time.tv_sec) + (stop_time.tv_usec - start_time.tv_usec) / 1000;
        flag_transfer_complete = 1;
    }
}

void random_string_generator(char buffer[])
{
    // printf("generating random array...");
    for (int i = 0; i < size; i++)
    {
        int char_index = 32 + rand() % 94;
        buffer[i] = char_index;
    }
    // printf("\n\nrandom array generated!\n\n");
}

void send_array(char buffer[])
{
    int block_size = (CIRCULAR_size / BLOCK_NUM) + (CIRCULAR_size % BLOCK_NUM != 0 ? 1 : 0);

    //number of cycles needed to send all the data
    int cycles = size / block_size + (size % block_size != 0 ? 1 : 0);
    for (int i = 0; i < cycles; i++)
    {
        char segment[block_size];
        for (int j = 0; j < block_size && ((i * block_size + j) < size); j++)
        {
            segment[j] = buffer[i * block_size + j];
        }

        if (i % BLOCK_NUM == 0 && i > 0)
        {
            ptr -= block_size * BLOCK_NUM;
        }

        sem_wait(not_full);
        sem_wait(mutex);
        for (int k = 0; k < strlen(segment); k++)
        {
            *ptr = segment[k];
            ptr++;
        }
        sem_post(mutex);
        sem_post(not_empty);
    }
}

int main(int argc, char *argv[])
{

    //getting size from console
    if (argc < 2)
    {
        fprintf(stderr, "Producer - ERROR, no size provided\n");
        exit(0);
    }
    size = atoi(argv[1]) * 1000000;

    //getting mode from console
    if (argc < 3)
    {
        fprintf(stderr, "Producer - ERROR, no mode provided\n");
        exit(0);
    }
    mode = atoi(argv[2]);

    //randomizing seed for random string generator
    srand(time(NULL));

    signal(SIGUSR1, transfer_complete);

    char *fifo_shared_producer_pid = "/tmp/shared_producer_pid";
    mkfifo(fifo_shared_producer_pid, 0666);
    int fd_pid = open(fifo_shared_producer_pid, O_WRONLY);
    pid_t pid = getpid();
    write(fd_pid, &pid, sizeof(pid));
    close(fd_pid);
    unlink(fifo_shared_producer_pid);

    if ((shm_fd = shm_open(shm_name, O_CREAT | O_RDWR | O_TRUNC, 0666)) == -1)
    {
        perror("producer - shm_open failure");
        exit(1);
    }

    // shm_size = sysconf(_SC_PAGE_size);
    // printf("size %d\n",SHM_size);

    //setting shared memory size
    if (ftruncate(shm_fd, CIRCULAR_size) == -1)
    {
        perror("producer - ftruncate failure");
        exit(1);
    }

    if ((ptr = mmap(0, CIRCULAR_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
        perror("producer - map failed");
        exit(1);
    }

    // ptr->in = ptr->out = 0;

    //initialize circular buffer semaphores
    if ((mutex = sem_open("mutex", O_CREAT, 0644, 1)) == MAP_FAILED)
    {
        perror("producer - sem_open failed");
        exit(1);
    }
    if ((not_full = sem_open("not_full", O_CREAT, 0644, BLOCK_NUM)) == MAP_FAILED)
    {
        perror("producer - sem_open failed");
        exit(1);
    }
    if ((not_empty = sem_open("not_empty", O_CREAT, 0644, 0)) == MAP_FAILED)
    {
        perror("producer - sem_open failed");
        exit(1);
    }

    if (sem_init(mutex, 1, 1) == -1)
    {
        perror("producer - sem_init failed");
        exit(1);
    }

    if (sem_init(not_full, 1, BLOCK_NUM) == -1)
    {
        perror("producer - sem_init failed");
        exit(1);
    }

    if (sem_init(not_empty, 1, 0) == -1)
    {
        perror("producer - sem_init failed");
        exit(1);
    }

    //switch between dynamic allocation or standard allocation
    if (mode == 0)
    {
        //dynamic allocation of buffer
        char *buffer = (char *)malloc(size);

        //generating random string
        random_string_generator(buffer);

        //get time of when the transfer has started
        gettimeofday(&start_time, NULL);

        //writing buffer on pipe
        send_array(buffer);

        //delete buffer from memory
        free(buffer);
    }
    else
    {
        //increasing stack limit to let the buffer be instantieted correctly
        struct rlimit limit;
        limit.rlim_cur = (size + 5) * 1000000;
        limit.rlim_max = (size + 5) * 1000000;
        setrlimit(RLIMIT_STACK, &limit);

        char buffer[size];

        //generating random string
        random_string_generator(buffer);

        //get time of when the transfer has started
        gettimeofday(&start_time, NULL);

        //writing buffer on pipe
        send_array(buffer);
    }

    while (flag_transfer_complete == 0)
    {
        ;
    }

    printf("\tshared memory time: %d ms\n", transfer_time);
    fflush(stdout);

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
