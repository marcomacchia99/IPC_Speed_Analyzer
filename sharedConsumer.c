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

void receive_array(char buffer[])
{
    //calculate size of each block of the circular buffer
    int block_size = (circular_size / BLOCK_NUM) + (circular_size % BLOCK_NUM != 0 ? 1 : 0);

    //number of cycles needed to receive all the data
    int cycles = size / block_size + (size % block_size != 0 ? 1 : 0);
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
        check(sem_wait(not_empty));
        check(sem_wait(mutex));

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
        check(sem_post(mutex));
        check(sem_post(not_full));
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
	//fprintf(logfile, "******log file created******\n");
	//fflush(logfile);

    //getting size from console
    if (argc < 2)
    {
        fprintf(stderr, "Consumer - ERROR, no size provided\n");
        exit(0);
    }
    size = atoi(argv[1]) * 1000000;

    //getting mode from console
    if (argc < 3)
    {
        fprintf(stderr, "Consumer - ERROR, no mode provided\n");
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

    //receiving pid from producer
    char *fifo_shared_producer_pid = "/tmp/shared_producer_pid";
    check(mkfifo(fifo_shared_producer_pid, 0666));
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
    check(unlink(fifo_shared_producer_pid));

    //wait until producer creates shared memory and semaphores
    usleep(100000);

    //open shared memory
    shm_fd = check(shm_open(shm_name, O_RDONLY, 0666))

    //map shared memory
    if ((shm_ptr = mmap(NULL, circular_size, PROT_READ, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
        perror("consumer - map failed");
        exit(1);
    }
    //original shared memory pointer, used with munmap
    char *original_shm_ptr = shm_ptr;

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
    check(sem_close(mutex)));
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
