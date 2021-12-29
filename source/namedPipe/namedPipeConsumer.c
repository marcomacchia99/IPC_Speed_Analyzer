#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

//pipe file descriptor
int fd_pipe;
//maximum size of writed data
int max_write_size;

//pid of producer process used to send signals
pid_t producer_pid;

//buffer size
int size;

//memory mode
int mode;

//variable use to get and set resource limits
struct rlimit limit;

//variables for select function

struct timeval timeout;
fd_set readfds;

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
    //set timeout for select
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;

    //number of cycles needed to receive all the data
    int cycles = size / max_write_size + (size % max_write_size != 0 ? 1 : 0);

    //write on log file
    fprintf(logfile, "consumer - starting receiving array...\n");
    fprintf(logfile, "consumer - there will be %d cycles\n", cycles);
    fflush(logfile);

    for (int i = 0; i < cycles; i++)
    {
        //wait until data is ready
        do
        {
            //set timeout for select
            timeout.tv_sec = 0;
            timeout.tv_usec = 1000;

            FD_ZERO(&readfds);
            //add the selected file descriptor to the selected fd_set
            FD_SET(fd_pipe, &readfds);
        } while (check(select(FD_SETSIZE + 1, &readfds, NULL, NULL, &timeout)) <= 0);

        //read string from producer
        char segment[max_write_size];
        check(read(fd_pipe, segment, max_write_size));

        //add every segment to entire buffer
        if (i == cycles - 1)
        {
            int j = 0;
            while ((i * max_write_size + j) < size)
            {
                buffer[i * max_write_size + j] = segment[j];
                j++;
            }
        }
        else
        {

            strcat(buffer, segment);
        }
    }
    //write on log file
    fprintf(logfile, "consumer - array received!\n");
    fflush(logfile);
}

int main(int argc, char *argv[])
{
    //open Log file
    logfile = fopen("./../logs/named_pipe_log.txt", "a");
    if (logfile == NULL)
    {
        printf("an error occured while creating Named_pipe's log File\n");
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

    //defining fifo path
    char *fifo_named_pipe = "/tmp/named_pipe";
    char *fifo_named_producer_pid = "/tmp/named_producer_pid";

    //create fifo
    mkfifo(fifo_named_pipe, 0666);
    mkfifo(fifo_named_producer_pid, 0666);

    //write on log file
    fprintf(logfile, "consumer - open pipe for pid\n");
    fflush(logfile);

    //receiving pid from producer
    int fd_pid_producer = check(open(fifo_named_producer_pid, O_RDONLY));

    int sel_val;
    do //wait until pid is ready
    {
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;

        FD_ZERO(&readfds);
        //add the selected file descriptor to the selected fd_set
        FD_SET(fd_pid_producer, &readfds);

        sel_val = check(select(FD_SETSIZE + 1, &readfds, NULL, NULL, &timeout));

    } while (sel_val <= 0);

    check(read(fd_pid_producer, &producer_pid, sizeof(producer_pid)));
    check(close(fd_pid_producer));
    unlink(fifo_named_producer_pid);

    //write on log file
    fprintf(logfile, "consumer - pid %d readed and open pipe for communication\n", producer_pid);
    fflush(logfile);

    //open fifo
    fd_pipe = check(open(fifo_named_pipe, O_RDONLY));

    //defining max size for operations and files
    check(getrlimit(RLIMIT_NOFILE, &limit));
    max_write_size = limit.rlim_max;
    check(fcntl(fd_pipe, F_SETPIPE_SZ, max_write_size));

    //write on log file
    fprintf(logfile, "consumer - limit extended\n");
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
        limit.rlim_cur = (size + 5) * 1000000;
        limit.rlim_max = (size + 5) * 1000000;
        check(setrlimit(RLIMIT_STACK, &limit));

        char buffer[size];
        //receive array from producer
        receive_array(buffer);
    }

    //transfer complete. Sends signal to notify the producer
    check(kill(producer_pid, SIGUSR1));

    //close and delete fifo
    check(close(fd_pipe));
    unlink(fifo_named_pipe);

    //write on log file
    fprintf(logfile, "consumer - pipe closed\n");
    fflush(logfile);

    //close log file
    fclose(logfile);

    return 0;
}
