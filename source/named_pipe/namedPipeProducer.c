#define _GNU_SOURCE
#include <errno.h>
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

//pipe file descriptor
int fd_pipe;
//maximum size of writed data
int max_write_size;

//variables for time measurement
struct timeval start_time, stop_time;
//flag if the consumer received all the data
int flag_transfer_complete = 0;
//amount of transfer milliseconds
int transfer_time;

//buffer size
int size;

//memory mode
int mode;

//variable use to get and set resource limits
struct rlimit limit;

//pointer to log file
FILE *logfile;

//This function checks if something failed, exits the program and prints an error in the logfile
int check(int retval)
{
    if (retval == -1)
    {
        fprintf(logfile, "\nProducer - ERROR (" __FILE__ ":%d) -- %s\n", __LINE__, strerror(errno));
        fflush(logfile);
        fclose(logfile);
        printf("\tAn error has been reported on log file.\n");
        fflush(stdout);
        exit(-1);
    }
    return retval;
}

void random_string_generator(char buffer[])
{
    for (int i = 0; i < size; i++)
    {
        int char_index = 32 + rand() % 94;
        buffer[i] = char_index;
    }
    //write on log file
    fprintf(logfile, "producer - random string generated\n");
    fflush(logfile);
}

void transfer_complete(int sig)
{
    if (sig == SIGUSR1)
    {
        //write on log file
        fprintf(logfile, "producer - received signal!\n");
        fflush(logfile);

        check(gettimeofday(&stop_time, NULL));
        //calculating time in milliseconds
        transfer_time = 1000 * (stop_time.tv_sec - start_time.tv_sec) + (stop_time.tv_usec - start_time.tv_usec) / 1000;
        flag_transfer_complete = 1;
    }
}

void send_array(char buffer[])
{

    //number of cycles needed to send all the data
    int cycles = size / max_write_size + (size % max_write_size != 0 ? 1 : 0);

    //write on log file
    fprintf(logfile, "producer - starting sending array...\n");
    fprintf(logfile, "producer - there will be %d cycles\n", cycles);
    fflush(logfile);

    //sending data to consumer divided into blocks of dimension max_write_size
    for (int i = 0; i < cycles; i++)
    {
        char segment[max_write_size];
        for (int j = 0; j < max_write_size && ((i * max_write_size + j) < size); j++)
        {
            segment[j] = buffer[i * max_write_size + j];
        }
        check(write(fd_pipe, segment, max_write_size));
    }
    //write on log file
    fprintf(logfile, "producer - array sent!\n");
    fflush(logfile);
}

int main(int argc, char *argv[])
{
    //open Log file
    logfile = fopen("./../logs/named_pipe_log.txt", "a");
    if (logfile == NULL)
    {
        printf("an error occured while creating Named_pipe's 	log File\n");
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
    //write on log file
    fprintf(logfile, "producer - received size of %dMB\n", size);
    fflush(logfile);

    //getting mode from console
    if (argc < 3)
    {
        fprintf(stderr, "Consumer - ERROR, no mode provided\n");
        exit(0);
    }
    mode = atoi(argv[2]);
    //write on log file
    fprintf(logfile, "producer - received mode %d\n", mode);
    fflush(logfile);

    //randomizing seed for random string generator
    srand(time(NULL));

    //the process must handle SIGUSR1 signal
    signal(SIGUSR1, transfer_complete);

    //defining fifo path
    char *fifo_named_pipe = "/tmp/named_pipe";
    char *fifo_named_producer_pid = "/tmp/named_producer_pid";

    //create fifo
    mkfifo(fifo_named_pipe, 0666);
    mkfifo(fifo_named_producer_pid, 0666);

    //write on log file
    fprintf(logfile, "producer - open pipe for pid\n");
    fflush(logfile);

    //sending pid to consumer
    int fd_pid_producer = check(open(fifo_named_producer_pid, O_WRONLY));
    pid_t pid = getpid();
    check(write(fd_pid_producer, &pid, sizeof(pid)));
    check(close(fd_pid_producer));
    unlink(fifo_named_producer_pid);

    //write on log file
    fprintf(logfile, "producer - pid %d sent and open pipe for communication\n", pid);
    fflush(logfile);

    //open fifo
    fd_pipe = check(open(fifo_named_pipe, O_WRONLY));

    //defining max size for operations and files
    check(getrlimit(RLIMIT_NOFILE, &limit));
    max_write_size = limit.rlim_max;
    check(fcntl(fd_pipe, F_SETPIPE_SZ, max_write_size));

    //write on log file
    fprintf(logfile, "producer - limit extended\n");
    fflush(logfile);

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
        free(buffer); /******check(*/
    }
    else
    {
        //increasing stack limit to let the buffer be instantieted correctly
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

    printf("\tnamed pipe time: %d ms\n", transfer_time);
    fflush(stdout);
    //write on log file
    fprintf(logfile, "time: %d ms\n", transfer_time);
    fflush(logfile);

    //close and delete fifo
    check(close(fd_pipe));
    unlink(fifo_named_pipe);

    //write on log file
    fprintf(logfile, "producer - pipe closed\n");
    fflush(logfile);

    //close log file
    fclose(logfile);

    return 0;
}
