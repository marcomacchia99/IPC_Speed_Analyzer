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

//pipes file descriptor
int fd_pipe[2];
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

//pointer to log file
FILE *logfile;

//This function checks if something failed, exits the program and prints an error in the logfile
int check(int retval)
{
    if (retval == -1)
    {
        fprintf(logfile, "\nERROR (" __FILE__ ":%d) -- %s\n", __LINE__, strerror(errno));
        fflush(logfile);
        fclose(logfile);
        printf("\tAn error has been reported on log file.\n");
        fflush(stdout);
        exit(-1);
    }
    return retval;
}

void random_string_generator(char buffer_producer[])
{
    for (int i = 0; i < size; i++)
    {
        int char_index = 32 + rand() % 94;
        buffer_producer[i] = char_index;
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

void send_array(char buffer_producer[])
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
            segment[j] = buffer_producer[i * max_write_size + j];
        }

        check(write(fd_pipe[1], segment, max_write_size));
    }
}

void receive_array(char buffer_consumer[])
{

    //variables for select function
    struct timeval timeout;
    fd_set readfds;

    //number of cycles needed to send all the do
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
            FD_SET(fd_pipe[0], &readfds);

        } while (check(select(FD_SETSIZE + 1, &readfds, NULL, NULL, &timeout)) <= 0);

        //read string from producer
        char segment[max_write_size];
        check(read(fd_pipe[0], segment, max_write_size));

        //add every segment to entire buffer
        if (i == cycles - 1)
        {
            int j = 0;
            while ((i * max_write_size + j) < size)
            {
                buffer_consumer[i * max_write_size + j] = segment[j];
                j++;
            }
        }
        else
        {

            strcat(buffer_consumer, segment);
        }
    }
    //write on log file
    fprintf(logfile, "consumer - array received!\n");
    fflush(logfile);
}

int main(int argc, char *argv[])
{
    //open Log file
    logfile = fopen("./../logs/unnamed_pipe_log.txt", "a");
    if (logfile == NULL)
    {
        printf("an error occured while creating Unnamed_pipe's log File\n");
        return 0;
    }
    fprintf(logfile, "starting process\n");
    fflush(logfile);

    //getting size from console
    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no size provided\n");
        exit(0);
    }
    size = atoi(argv[1]) * 1000000;
    //write on log file
    fprintf(logfile, "received size of %dMB\n", size);
    fflush(logfile);

    //getting mode from console
    if (argc < 3)
    {
        fprintf(stderr, "ERROR, no mode provided\n");
        exit(0);
    }
    mode = atoi(argv[2]);
    //write on log file
    fprintf(logfile, "received mode %d\n", mode);
    fflush(logfile);

    //randomizing seed for random string generator
    srand(time(NULL));

    //generating pipe
    check(pipe(fd_pipe));

    //write on log file
    fprintf(logfile, "open pipe for communication\n");
    fflush(logfile);

    //defining max size for operations and files
    struct rlimit limit;
    check(getrlimit(RLIMIT_NOFILE, &limit));
    max_write_size = limit.rlim_max;
    check(fcntl(fd_pipe[1], F_SETPIPE_SZ, max_write_size));

    //write on log file
    fprintf(logfile, "limit extended\n");
    fflush(logfile);

    pid_t pid;
    if ((pid = fork()) == 0) //consumer
    {

        pid_t pid_producer;

        check(close(fd_pipe[1]));

        //variables for select function
        struct timeval timeout;
        fd_set readfds;

        //receiving producer pid
        int sel_val;
        do //wait until pid is ready
        {
            timeout.tv_sec = 0;
            timeout.tv_usec = 1000;

            FD_ZERO(&readfds);
            //add the selected file descriptor to the selected fd_set
            FD_SET(fd_pipe[0], &readfds);

            sel_val = check(select(FD_SETSIZE + 1, &readfds, NULL, NULL, &timeout));

        } while (sel_val <= 0);

        check(read(fd_pipe[0], &pid_producer, sizeof(pid_t)));

        //write on log file
        fprintf(logfile, "consumer - pid %d readed\n", pid_producer);
        fflush(logfile);

        //switch between dynamic allocation or standard allocation
        if (mode == 0)
        {
            //dynamic allocation of buffer
            char *buffer_consumer = (char *)malloc(size);

            //receive array from producer
            receive_array(buffer_consumer);

            //delete buffer from memory
            free(buffer_consumer);
        }
        else
        {
            //increasing stack limit to let the buffer be instantieted correctly
            struct rlimit limit;
            limit.rlim_cur = (size + 5) * 1000000;
            limit.rlim_max = (size + 5) * 1000000;
            check(setrlimit(RLIMIT_STACK, &limit));

            char buffer_consumer[size];
            //receive array from producer
            receive_array(buffer_consumer);
        }

        //transfer complete. Sends signal to notify the producer
        check(kill(pid_producer, SIGUSR1));
        check(close(fd_pipe[0]));

        //write on log file
        fprintf(logfile, "consumer - pipe closed\n");
        fflush(logfile);

        //close log file
        fclose(logfile);
    }
    else
    { //producer

        check(close(fd_pipe[0]));

        //the process must handle SIGUSR1 signal
        signal(SIGUSR1, transfer_complete);

        //sending pid to consumer
        pid_t pid_producer = getpid();
        check(write(fd_pipe[1], &pid_producer, sizeof(pid_t)));

        //write on log file
        fprintf(logfile, "producer - pid %d sent and open pipe for communication\n", pid);
        fflush(logfile);

        //switch between dynamic allocation or standard allocation
        if (mode == 0)
        {
            //dynamic allocation of buffer
            char *buffer_producer = (char *)malloc(size);

            //generating random string
            random_string_generator(buffer_producer);

            //get time of when the transfer has started
            check(gettimeofday(&start_time, NULL));

            //writing buffer on pipe
            send_array(buffer_producer);

            //delete buffer from memory
            free(buffer_producer); // STESSO DISCORSO SOPRA
        }
        else
        {
            //increasing stack limit to let the buffer be instantieted correctly
            struct rlimit limit;
            limit.rlim_cur = (size + 5) * 1000000;
            limit.rlim_max = (size + 5) * 1000000;
            setrlimit(RLIMIT_STACK, &limit);

            char buffer_producer[size];

            //generating random string
            random_string_generator(buffer_producer);

            //get time of when the transfer has started
            gettimeofday(&start_time, NULL); //************

            //writing buffer on pipe
            send_array(buffer_producer);
        }

        //wait until transfer is complete
        while (flag_transfer_complete == 0)
        {
            ;
        }

        printf("\tunnamed pipe time: %d ms\n", transfer_time);
        fflush(stdout);
        //write on log file
        fprintf(logfile, "time: %d ms\n", transfer_time);
        fflush(logfile);

        //close and delete fifo
        check(close(fd_pipe[1]));

        //write on log file
        fprintf(logfile, "consumer - pipe closed\n");
        fflush(logfile);

        //close log file
        fclose(logfile);
    }

    return 0;
}
