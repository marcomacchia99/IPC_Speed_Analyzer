#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#define SIZE 100 * 1000000

int fd_pipe;
char buffer[SIZE] = "";
int max_write_size;

pid_t producer_pid;


void receive_array()
{
    //variables for select function
    struct timeval timeout;
    fd_set readfds;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;

    int cycles = SIZE / max_write_size + (SIZE % max_write_size != 0 ? 1 : 0);
    for (int i = 0; i < cycles; i++)
    {
        FD_ZERO(&readfds);
        //add the selected file descriptor to the selected fd_set
        FD_SET(fd_pipe, &readfds);
        while (select(FD_SETSIZE + 1, &readfds, NULL, NULL, &timeout) < 0)
        {
            ;
        }

        //read random string from producer
        char segment[max_write_size];
        read(fd_pipe, segment, max_write_size);
        // printf("read: %ld\n", read(fd_pipe, segment, SIZE));

        //add every segment to entire buffer
        if (i == cycles - 1)
        {
            int j=0;
            while((i * max_write_size + j) < SIZE){
                buffer[i*max_write_size+j]=segment[j];
                j++;
            }
        }
        else
        {

            strcat(buffer, segment);
        }

        // for (int j = 0; j < max_write_size && ((i * max_write_size + j) < SIZE); j++)
        // {
        //     buffer[i * max_write_size + j] = segment[j];
        // }
    }
    //     FILE *file = fopen("cons.txt", "w");
    // fprintf(file, "%s", buffer);
    // fflush(file);
    // fclose(file);
}

int main(int argc, char *argv[])
{

    //defining fifo path
    char *fifo_named_pipe = "/tmp/named_pipe";
    char *fifo_named_producer_pid = "/tmp/named_producer_pid";

    //create fifo
    mkfifo(fifo_named_pipe, 0666);
    mkfifo(fifo_named_producer_pid, 0666);

    //receiving pid from producer
    int fd_pid_producer = open(fifo_named_producer_pid, O_RDONLY);
    read(fd_pid_producer, &producer_pid, sizeof(producer_pid));
    close(fd_pid_producer);
    unlink(fifo_named_producer_pid);

    //open fifo
    fd_pipe = open(fifo_named_pipe, O_RDONLY);

    //defining max size for operations and files
    struct rlimit limit;
    getrlimit(RLIMIT_NOFILE, &limit);
    max_write_size = limit.rlim_max;
    fcntl(fd_pipe, F_SETPIPE_SZ, max_write_size);

    //receive array from producer
    receive_array();

    //transfer complete. Sends signal to notify the producer
    kill(producer_pid, SIGUSR1);

    //close and delete fifo
    close(fd_pipe);
    unlink(fifo_named_pipe);

    return 0;
}