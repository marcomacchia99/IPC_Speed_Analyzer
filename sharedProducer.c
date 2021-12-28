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

//variables for time measurement
struct timeval start_time, stop_time;
//flag if the consumer received all the data
int flag_transfer_complete = 0;
//amount of transfer milliseconds
int transfer_time;

//semaphores used for circular buffer and shared memory

sem_t *mutex;
sem_t *not_empty;
sem_t *not_full;

//buffer size
int size;

//memory mode
int mode;

//circular buffer size
int circular_size;

FILE *logfile;

//This function checks if something failed, exits the program and prints an error in the logfile
int check(int retval)
{
	if(retval == -1)
	{
		fprintf(logfile,"\nERROR (" __FILE__ ":%d) -- %s\n",__LINE__,strerror(errno));
        fflush(logfile);
        fclose(logfile);
		exit(-1);
	}
	return retval;
}

void transfer_complete(int sig)
{
    if (sig == SIGUSR1)
    {
        check(gettimeofday(&stop_time, NULL));
        //calculating time in milliseconds
        transfer_time = 1000 * (stop_time.tv_sec - start_time.tv_sec) + (stop_time.tv_usec - start_time.tv_usec) / 1000;
        flag_transfer_complete = 1;
    }
}

void random_string_generator(char buffer[])
{
    for (int i = 0; i < size; i++)
    {
        int char_index = 32 + rand() % 94;
        buffer[i] = char_index;
    }
}

void send_array(char buffer[])
{
    //calculate size of each block of the circular buffer
    int block_size = (circular_size / BLOCK_NUM) + (circular_size % BLOCK_NUM != 0 ? 1 : 0);

    //number of cycles needed to send all the data
    int cycles = size / block_size + (size % block_size != 0 ? 1 : 0);
    for (int i = 0; i < cycles; i++)
    {
        //divide data in blocks
        char segment[block_size];
        for (int j = 0; j < block_size && ((i * block_size + j) < size); j++)
        {
            segment[j] = buffer[i * block_size + j];
        }

        //every time BLOCK_NUM blocks have been sent, the pointer moves back to the first address of the circular buffer
        if (i % BLOCK_NUM == 0 && i > 0)
        {
            shm_ptr -= block_size * BLOCK_NUM;
        }

        //coordinate with consumer
        check(sem_wait(not_full));
        check(sem_wait(mutex));

        for (int k = 0; k < strlen(segment); k++)
        {
            //write each character
            *shm_ptr = segment[k];
            shm_ptr++;
        }

        //coordinate with producer
        check(sem_post(mutex));
        check(sem_post(not_empty));
    }
}

int main(int argc, char *argv[])
{

//open Log file
    logfile = fopen("SharedMemory.txt","w");
    if(logfile==NULL){
	printf("an error occured while creating SharedMemory's log File\n");
	return 0;
	} 
	fprintf(logfile, "******log file created******\n");
	fflush(logfile);
	
	
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

    //getting circular budffer size from console
    if (argc < 4)
    {
        fprintf(stderr, "Producer - ERROR, no circular size provided\n");
        exit(0);
    }
    circular_size = atoi(argv[3]) * 1000;

    //randomizing seed for random string generator
    srand(time(NULL));

    //the process must handle SIGUSR1 signal
    signal(SIGUSR1, transfer_complete);

    //sending pid to consumer
    char *fifo_shared_producer_pid = "/tmp/shared_producer_pid";
    check(mkfifo(fifo_shared_producer_pid, 0666));
    int fd_pid = check(open(fifo_shared_producer_pid, O_WRONLY));
    pid_t pid = getpid();
    check(write(fd_pid, &pid, sizeof(pid)));
    check(close(fd_pid));
    check(unlink(fifo_shared_producer_pid));

    //open shared memory
    shm_fd = check(shm_open(shm_name, O_CREAT | O_RDWR | O_TRUNC, 0666));


    //setting shared memory size
    check(ftruncate(shm_fd, circular_size)) ;

    //map shared memory
    if ((shm_ptr = mmap(0, circular_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
        perror("producer - map failed");
        exit(1);
    }
    //original shared memory pointer, used with munmap
    char *original_shm_ptr = shm_ptr;

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

    check(sem_init(mutex, 1, 1);

    check(sem_init(not_full, 1, BLOCK_NUM));

   check(sem_init(not_empty, 1, 0));

    //switch between dynamic allocation or standard allocation
    if (mode == 0)
    {
        //dynamic allocation of buffer
        char *buffer = (char *)malloc(size);

        //generating random string
        random_string_generator(buffer);

        //get time of when the transfer has started
        check(gettimeofday(&start_time, NULL));

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
        check(setrlimit(RLIMIT_STACK, &limit));

        char buffer[size];

        //generating random string
        random_string_generator(buffer);

        //get time of when the transfer has started
        check(gettimeofday(&start_time, NULL));

        //writing buffer on pipe
        send_array(buffer);
    }

    //wait until transfer is complete
    while (flag_transfer_complete == 0)
    {
        ;
    }

    printf("\tshared memory time: %d ms\n", transfer_time);
    fflush(stdout);

    //close and delete semaphores
    check(sem_close(mutex));
    check(sem_unlink("mutex"));
    check(sem_close(not_empty));
    check(sem_unlink("not_empty"));
    check(sem_close(not_full));
    check(sem_unlink("not_full"));

    //deallocate and close shared memory
    check(munmap(original_shm_ptr, circular_size));
    check(close(shm_fd));

    return 0;
}
