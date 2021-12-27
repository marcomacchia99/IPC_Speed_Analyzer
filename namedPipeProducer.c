#define _GNU_SOURCE

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

int fd_pipe;

struct timeval start_time, stop_time;
int flag_transfer_complete = 0;
int transfer_time;
int max_write_size;

int size;


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

void send_array(char buffer[])
{
    // FILE * file = fopen("prod.txt","w");
    // fprintf(file,"%s",buffer);
    // fflush(file);
    // fclose(file);

    int cycles = size / max_write_size + (size % max_write_size != 0 ? 1 : 0);
    for (int i = 0; i < cycles; i++)
    {
        char segment[max_write_size];
        for (int j = 0; j < max_write_size && ((i * max_write_size + j) < size); j++)
        {
            segment[j] = buffer[i * max_write_size + j];
        }
        // printf("write: %ld\n",write(fd_pipe, segment, max_write_size));
        write(fd_pipe, segment, max_write_size);
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

    //increasing stack limit to let the buffer be instantieted correctly
    struct rlimit limit;
    limit.rlim_cur = 105 * 1000000;
    limit.rlim_max = 105 * 1000000;
    setrlimit(RLIMIT_STACK, &limit);
    
    char buffer[size];

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

    //sending pid to consumer
    int fd_pid_producer = open(fifo_named_producer_pid, O_WRONLY);
    pid_t pid = getpid();
    write(fd_pid_producer, &pid, sizeof(pid));
    close(fd_pid_producer);
    unlink(fifo_named_producer_pid);

    //open fifo
    fd_pipe = open(fifo_named_pipe, O_WRONLY);

    //defining max size for operations and files

    getrlimit(RLIMIT_NOFILE, &limit);
    max_write_size = limit.rlim_max;
    fcntl(fd_pipe, F_SETPIPE_SZ, max_write_size);

    //generating random string
    random_string_generator(buffer);

    //get time of when the transfer has started
    gettimeofday(&start_time, NULL);

    //writing buffer on pipe
    send_array(buffer);

    while (flag_transfer_complete == 0)
    {
        ;
    }

    printf("named pipe time: %d ms\n", transfer_time);
    fflush(stdout);

    //close and delete fifo
    close(fd_pipe);
    unlink(fifo_named_pipe);

    return 0;
}