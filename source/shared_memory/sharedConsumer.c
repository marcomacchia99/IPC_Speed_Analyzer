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
#include <errno.h>

#define BLOCK_NUM 100

//shared memory name
const char *shm_name = "/AOS";
//shared memory file descriptor
int shm_fd;
//shared memory pointer
char *shm_ptr;

//semaphores used for circular buffer and shared memory

sem_t *mutex;
sem_t *not_empty;
sem_t *not_full;

//pid of producer process used to send signals
pid_t producer_pid;

//buffer size
int size;

//memory mode
int mode;

//circular buffer size
int circular_size;

//pointer to log file
FILE *logfile;

//This function checks if something failed, exits the program and prints an error in the logfile
int check(int retval)
{
    if (retval == -1)
    {
        fprintf(logfile, "\nConsumer - ERROR (" __FILE__ ":%d) -- %s\n", __LINE__, strerror(errno));
        fflush(logfile);
        fclose(logfile);
        printf("\tAn error has been reported on log file.\n");
        fflush(stdout);
        exit(-1);
    }
    return retval;
}

void receive_array(char buffer[])
{

    //calculate size of each block of the circular buffer
    int block_size = (circular_size / BLOCK_NUM);

    //number of cycles needed to receive all the data
    int cycles = size / block_size + (size % block_size != 0 ? 1 : 0);

    //write on log file
    fprintf(logfile, "consumer - starting receiving array...\n");
    fprintf(logfile, "consumer - there will be %d cycles\n", cycles);
    fflush(logfile);

    int a = 1;
    int b = 0;
    int c = BLOCK_NUM;
    for (int i = 0; i < cycles; i++)
    {
        //read string from producer
        char segment[block_size];

        //every time BLOCK_NUM blocks have been received, the pointer moves back to the first address of the circular buffer
        if (i % BLOCK_NUM == 0 && i > 0)
        {
            shm_ptr -= block_size * BLOCK_NUM;
        }

        //coordinate with producer
        sem_wait(not_empty);
        sem_wait(mutex);

        //copy received data into the buffer
        if (i == cycles - 1)
        {

            int j = 0;
            while ((i * block_size + j) < size)
            {
                buffer[i * block_size + j] = *shm_ptr;
                shm_ptr++;
                j++;
            }
        }
        else
        {
            for (int j = 0; j < block_size; j++)
            {
                buffer[i * block_size + j] = *shm_ptr;
                shm_ptr++;
            }
        }

        //coordinate with producer
        sem_post(mutex);
        sem_post(not_full);
    }
    //write on log file
    fprintf(logfile, "consumer - array received!\n");
    fflush(logfile);
}

int main(int argc, char *argv[])
{

    //open Log file
    logfile = fopen("./../logs/shared_memory_log.txt", "a");
    if (logfile == NULL)
    {
        printf("an error occured while creating SharedMemory's log File\n");
        return 0;
    }

    //getting size from console
    if (argc < 2)
    {
        fprintf(stderr, "Consumer - ERROR, no size provided\n");
        exit(0);
    }
    size = atoi(argv[1]) * 1000000;
    //write on log file
    fprintf(logfile, "consumer - received size of %dMB\n", size);
    fflush(logfile);

    //getting mode from console
    if (argc < 3)
    {
        fprintf(stderr, "Consumer - ERROR, no mode provided\n");
        exit(0);
    }
    mode = atoi(argv[2]);
    //write on log file
    fprintf(logfile, "consumer - received mode %d\n", mode);
    fflush(logfile);

    //getting circular budffer size from console
    if (argc < 4)
    {
        fprintf(stderr, "Producer - ERROR, no circular size provided\n");
        exit(0);
    }
    circular_size = atoi(argv[3]) * 1000;
    //write on log file
    fprintf(logfile, "consumer - received circular size of %dKB\n", circular_size);
    fflush(logfile);

    //receiving pid from producer
    char *fifo_shared_producer_pid = "/tmp/shared_producer_pid";
    mkfifo(fifo_shared_producer_pid, 0666);

    //write on log file
    fprintf(logfile, "consumer - open pipe for pid\n");
    fflush(logfile);

    int fd_pid = check(open(fifo_shared_producer_pid, O_RDONLY));

    //variables for select function
    struct timeval timeout;
    fd_set readfds;

    int sel_val;
    do //wait until pid is ready
    {
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;

        FD_ZERO(&readfds);
        //add the selected file descriptor to the selected fd_set
        FD_SET(fd_pid, &readfds);

        sel_val = check(select(FD_SETSIZE + 1, &readfds, NULL, NULL, &timeout));

    } while (sel_val <= 0);

    check(read(fd_pid, &producer_pid, sizeof(producer_pid)));
    check(close(fd_pid));
    unlink(fifo_shared_producer_pid);

    //write on log file
    fprintf(logfile, "consumer - pid %d readed and open shared memory\n", producer_pid);
    fflush(logfile);

    //wait until producer creates shared memory and semaphores
    usleep(100000);

    //open shared memory
    shm_fd = check(shm_open(shm_name, O_RDONLY, 0666));

    //map shared memory
    if ((shm_ptr = mmap(NULL, circular_size, PROT_READ, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
        check(-1);
    }
    //original shared memory pointer, used with munmap
    char *original_shm_ptr = shm_ptr;

    //write on log file
    fprintf(logfile, "consumer - shared memory mapped\n");
    fflush(logfile);

    //initialize circular buffer semaphores
    if ((mutex = sem_open("mutex", 0, 0644, 1)) == MAP_FAILED)
    {
        check(-1);
    }
    if ((not_full = sem_open("not_full", 0, 0644, BLOCK_NUM)) == MAP_FAILED)
    {
        check(-1);
    }
    if ((not_empty = sem_open("not_empty", 0, 0644, 0)) == MAP_FAILED)
    {
        check(-1);
    }

    //write on log file
    fprintf(logfile, "consumer - semaphores opened\n");
    fflush(logfile);

    //switch between dynamic allocation or standard allocation
    if (mode == 0)
    {
        //dynamic allocation of buffer
        char *buffer = (char *)malloc(size);

        //receive array from producer
        receive_array(buffer);

        //delete buffer from memory
        free(buffer);
    }
    else
    {
        //increasing stack limit to let the buffer be instantieted correctly
        struct rlimit limit;
        limit.rlim_cur = (size + 5) * 1000000;
        limit.rlim_max = (size + 5) * 1000000;
        check(setrlimit(RLIMIT_STACK, &limit));

        char buffer[size];
        //receive array from producer
        receive_array(buffer);
    }

    //transfer complete. Sends signal to notify the producer
    check(kill(producer_pid, SIGUSR1));

    check(shm_unlink(shm_name));

    //close and delete semaphores
    check(sem_close(mutex));
    sem_unlink("mutex");
    check(sem_close(not_empty));
    sem_unlink("not_empty");
    check(sem_close(not_full));
    sem_unlink("not_full");

    //write on log file
    fprintf(logfile, "consumer - semaphores closed\n");
    fflush(logfile);

    //deallocate and close shared memory
    check(munmap(original_shm_ptr, circular_size));
    check(close(shm_fd));

    //write on log file
    fprintf(logfile, "consumer - shared memory closed\n");
    fflush(logfile);

    //close log file
    fclose(logfile);

    return 0;
}
