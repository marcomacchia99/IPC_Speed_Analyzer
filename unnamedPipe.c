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

int fd_pipe[2];

struct timeval start_time, stop_time;
int flag_transfer_complete = 0;
int transfer_time;
int max_write_size;

int size;

void random_string_generator(char buffer_producer[])
{
    // printf("generating random array...");
    for (int i = 0; i < size; i++)
    {
        int char_index = 32 + rand() % 94;
        buffer_producer[i] = char_index;
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

void send_array(char buffer_producer[])
{
    // FILE * file = fopen("prod.txt","w");
    // fprintf(file,"%s",buffer_producer);
    // fflush(file);
    // fclose(file);

    int cycles = size / max_write_size + (size % max_write_size != 0 ? 1 : 0);
    for (int i = 0; i < cycles; i++)
    {
        char segment[max_write_size];
        for (int j = 0; j < max_write_size && ((i * max_write_size + j) < size); j++)
        {
            segment[j] = buffer_producer[i * max_write_size + j];
        }
        // printf("write: %ld\n", write(fd_pipe[1], segment, max_write_size));
        write(fd_pipe[1], segment, max_write_size);
    }
}

void receive_array(char buffer_consumer[])
{
    //variables for select function
    struct timeval timeout;
    fd_set readfds;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;

    int cycles = size / max_write_size + (size % max_write_size != 0 ? 1 : 0);
    for (int i = 0; i < cycles; i++)
    {
        FD_ZERO(&readfds);
        //add the selected file descriptor to the selected fd_set
        FD_SET(fd_pipe[0], &readfds);

        while (select(FD_SETSIZE + 1, &readfds, NULL, NULL, &timeout) < 0)
        {
            ;
        }

        //read random string from producer
        char segment[max_write_size];
        read(fd_pipe[0], segment, max_write_size);
        // printf("read: %ld\n", read(fd_pipe[0], segment, size));

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

        // for (int j = 0; j < max_write_size && ((i * max_write_size + j) < size); j++)
        // {
        //     buffer[i * max_write_size + j] = segment[j];
        // }
    }
    //     FILE *file = fopen("cons.txt", "w");
    // fprintf(file, "%s", buffer_consumer);
    // fflush(file);
    // fclose(file);
}

int main(int argc, char *argv[])
{
    //getting size from console
    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no size provided\n");
        exit(0);
    }
    size = atoi(argv[1]) * 1000000;

    //increasing stack limit to let the buffers be instantieted correctly
    struct rlimit limit;
    limit.rlim_cur = 105 * 1000000;
    limit.rlim_max = 105 * 1000000;
    setrlimit(RLIMIT_STACK, &limit);

    //randomizing seed for random string generator
    srand(time(NULL));

    //generating pipe
    if (pipe(fd_pipe) < 0)
    {
        perror("Error while creating unnamed pipe: ");
    }
    else
    {
        //defining max size for operations and files
        struct rlimit limit;
        getrlimit(RLIMIT_NOFILE, &limit);
        max_write_size = limit.rlim_max;
        fcntl(fd_pipe[1], F_SETPIPE_SZ, max_write_size);

        pid_t pid;
        if ((pid = fork()) == 0) //consumer
        {
            char buffer_consumer[size];

            close(fd_pipe[1]);

            //receiving producer pid
            pid_t pid_producer;
            read(fd_pipe[0], &pid_producer, sizeof(pid_t));

            // printf("read: %ld\n",read(fd_pipe[0], buffer_consumer, size));
            receive_array(buffer_consumer);
            kill(pid_producer, SIGUSR1);
            close(fd_pipe[0]);
        }
        else
        { //producer

            char buffer_producer[size];

            close(fd_pipe[0]);

            signal(SIGUSR1, transfer_complete);

            //sending pid to consumer
            pid_t pid_producer = getpid();
            write(fd_pipe[1], &pid_producer, sizeof(pid_t));

            //generating random string
            random_string_generator(buffer_producer);

            //get time of when the transfer has started
            gettimeofday(&start_time, NULL);

            //writing buffer on pipe

            send_array(buffer_producer);
            // printf("write: %ld\n",write(fd_pipe[1], buffer_producer, size));

            while (flag_transfer_complete == 0)
            {
                ;
            }

            printf("unnamed pipe time: %d ms\n", transfer_time);
            fflush(stdout);

            //close and delete fifo
            close(fd_pipe[1]);
        }
    }

    return 0;
}