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

#define SIZE 1 * 1000000
#define CIRCULAR_SIZE 10 * 1000
#define BLOCK_NUM 100

char buffer[SIZE] = "";

const char *shm_name = "/AOS";
int shm_fd;
caddr_t * ptr;
int buffer_index = 0;

sem_t *mutex;
sem_t *not_empty;
sem_t *not_full;

void receive_array()
{

    int block_size = (CIRCULAR_SIZE / BLOCK_NUM) + (CIRCULAR_SIZE % BLOCK_NUM != 0 ? 1 : 0);
    int cycles = SIZE / block_size + (SIZE % block_size != 0 ? 1 : 0);
    printf("cycles %d\n", cycles);
    for (int i = 0; i < cycles; i++)
    {
        printf("cycle %d\n", i);
        //read random string from producer
        char segment[block_size];

        // sem_wait(&mutex);
        // printf("%d - read: %ld\n", i, read(fd_socket, segment, max_write_size));
        sem_wait(not_empty);
        sem_wait(mutex);
        printf("%s\n", ptr[buffer_index]);
        strcpy(segment, ptr[buffer_index]);
        sem_post(mutex);
        sem_post(not_full);

        buffer_index = (buffer_index + 1) % BLOCK_NUM;
        // sem_post(&mutex);

        //add every segment to entire buffer
        if (i == cycles - 1)
        {

            int j = 0;
            while ((i * block_size + j) < SIZE)
            {
                buffer[i * block_size + j] = segment[j];
                j++;
            }
        }
        else
        {
            strcat(buffer, segment);
        }
    }
    printf("x\n");
    // FILE *file = fopen("cons.txt", "w");
    // fprintf(file, "%s", buffer);
    // fflush(file);
    // fclose(file);
}

int main(int argc, char *argv[])
{
    sleep(1);

    if ((shm_fd = shm_open(shm_name, O_RDWR, 0)) == -1)
    {
        perror("consumer - shm_open failure");
        exit(1);
    }

    if ((ptr = mmap(0, CIRCULAR_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == (caddr_t) -1)
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

    printf("a\n");
    printf("%s\n",ptr[0]);
    receive_array();

    if (shm_unlink(shm_name) == 1)
    {
        printf("Error removing %s\n", shm_name);
        exit(1);
    }

    munmap(ptr, CIRCULAR_SIZE);
    close(shm_fd);

    sem_close(mutex);
    sem_unlink("mutex");
    sem_close(not_empty);
    sem_unlink("not_empty");
    sem_close(not_full);
    sem_unlink("not_full");

    return 0;
}