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

#define SIZE 100 * 1000000
#define CIRCULAR_SIZE 10 * 1000
#define BLOCK_NUM 100

char buffer[SIZE] = "";
const char *shm_name = "/AOS";
int shm_fd;
char *ptr;
int buffer_index = 0;

sem_t *mutex;
sem_t *not_empty;
sem_t *not_full;

void random_string_generator()
{
    printf("generating random array...");
    for (int i = 0; i < SIZE; i++)
    {
        int char_index = 32 + rand() % 94;
        buffer[i] = char_index;
    }
    printf("\n\nrandom array generated!\n\n");
}

void send_array()
{
    // FILE *file = fopen("prod.txt", "w");
    // fprintf(file, "%s", buffer);
    // fflush(file);
    // fclose(file);
    int block_size = (CIRCULAR_SIZE / BLOCK_NUM) + (CIRCULAR_SIZE % BLOCK_NUM != 0 ? 1 : 0);

    int cycles = SIZE / block_size + (SIZE % block_size != 0 ? 1 : 0);
    //sending data divided in blocks of max_write_size size
    for (int i = 0; i < cycles; i++)
    {
        char segment[block_size];
        for (int j = 0; j < block_size && ((i * block_size + j) < SIZE); j++)
        {
            segment[j] = buffer[i * block_size + j];
        }
        // while(sem_trywait(&mutex)==0){
        //     usleep(1000);
        // }
        // sem_wait(&mutex);
        // printf("%d - write: %ld\n", i, write(fd_socket_new, segment, max_write_size));
        if(i%BLOCK_NUM==0 && i>0){
            ptr-= block_size*BLOCK_NUM;
        }
        sem_wait(not_full);
        sem_wait(mutex);
        for (int k = 0; k < strlen(segment); k++)
        {
            *ptr = segment[k];
            printf("%c",segment[k]);
            ptr++;
        }
        printf("\n");
        // ptr[buffer_index] = segment;
        // printf("%d - %s\n", buffer_index, ptr[buffer_index]);
        buffer_index = (buffer_index + 1) % BLOCK_NUM;
        sem_post(mutex);
        sem_post(not_empty);

        // sem_post(&mutex);
    }
}

int main(int argc, char *argv[])
{
    if ((shm_fd = shm_open(shm_name, O_CREAT | O_RDWR | O_TRUNC, 0666)) == -1)
    {
        perror("producer - shm_open failure");
        exit(1);
    }

    // shm_size = sysconf(_SC_PAGE_SIZE);
    // printf("size %d\n",SHM_SIZE);

    //setting shared memory size
    if (ftruncate(shm_fd, CIRCULAR_SIZE) == -1)
    {
        perror("producer - ftruncate failure");
        exit(1);
    }

    if ((ptr = mmap(0, CIRCULAR_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
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

    random_string_generator();

    send_array();

    // // write the next entry and atomically update the write sequence number
    // /* Buffer *msg = &ptr->_buffer[ptr->in % SIZE];
    //     msg->_id = i++; */
    // ptr->file_path[ptr->in] = 1 + 1;
    // //ptr->in = (ptr->in + 1) % SIZE;
    // //__sync_fetch_and_add(&ptr->in, 1);
    // printf("in : %d", ptr->file_path[ptr->in]);
    // // give consumer some time to catch up


    sem_close(mutex);
    sem_unlink("mutex");
    sem_close(not_empty);
    sem_unlink("not_empty");
    sem_close(not_full);
    sem_unlink("not_full");

    munmap(ptr, CIRCULAR_SIZE);
    close(shm_fd);

    return 0;
}
