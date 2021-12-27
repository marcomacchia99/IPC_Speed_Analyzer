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
#include <semaphore.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_WRITE_SIZE 65535 //max tcp data length

int fd_socket;
int fd_socket_new;
int portno;
struct sockaddr_in server_addr, client_addr;

struct timeval start_time, stop_time;
int flag_transfer_complete = 0;
int transfer_time;

sem_t mutex;

int size;
int mode;

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
    //number of cycles needed to send all the data
    int cycles = size / MAX_WRITE_SIZE + (size % MAX_WRITE_SIZE != 0 ? 1 : 0);

    for (int i = 0; i < cycles; i++)
    {
        char segment[MAX_WRITE_SIZE];
        for (int j = 0; j < MAX_WRITE_SIZE && ((i * MAX_WRITE_SIZE + j) < size); j++)
        {
            segment[j] = buffer[i * MAX_WRITE_SIZE + j];
        }

        write(fd_socket_new, segment, MAX_WRITE_SIZE);
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

    //getting mode from console
    if (argc < 3)
    {
        fprintf(stderr, "Producer - ERROR, no mode provided\n");
        exit(0);
    }
    mode = atoi(argv[2]);

    if (argc < 4)
    {
        fprintf(stderr, "Producer - ERROR, no port provided\n");
        exit(0);
    }
    portno = atoi(argv[3]);

    //randomizing seed for random string generator
    srand(time(NULL));

    //the process must handle SIGUSR1 signal
    signal(SIGUSR1, transfer_complete);

    //create socket
    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_socket < 0)
    {
        perror("Producer - ERROR opening socket");
        exit(0);
    }

    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portno);

    if (bind(fd_socket, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {

        perror("Producer - ERROR on binding");
        exit(0);
    }

    listen(fd_socket, 5);

    int client_length = sizeof(client_addr);
    fd_socket_new = accept(fd_socket, (struct sockaddr *)&client_addr, &client_length);
    if (fd_socket_new < 0)
    {
        perror("Producer - ERROR accepting connection");
        exit(0);
    }

    //sending pid to consumer
    pid_t pid = getpid();
    write(fd_socket_new, &pid, sizeof(pid));

    //switch between dynamic allocation or standard allocation
    if (mode == 0)
    {
        //dynamic allocation of buffer
        char *buffer = (char *)malloc(size);

        //generating random string
        random_string_generator(buffer);

        //get time of when the transfer has started
        gettimeofday(&start_time, NULL);

        //writing buffer on pipe
        send_array(buffer);

        //delete buffer from memory
        free(buffer);
    }
    else
    {
        //increasing stack limit to let the buffer be instantieted correctly
        struct rlimit limit;
        limit.rlim_cur = (size+5) * 1000000;
        limit.rlim_max = (size+5) * 1000000;
        setrlimit(RLIMIT_STACK, &limit);

        char buffer[size];

        //generating random string
        random_string_generator(buffer);

        //get time of when the transfer has started
        gettimeofday(&start_time, NULL);

        //writing buffer on pipe
        send_array(buffer);
    }
    while (flag_transfer_complete == 0)
    {
        ;
    }

    printf("\tsocket time: %d ms\n", transfer_time);
    fflush(stdout);

    //close socket
    close(fd_socket);
    close(fd_socket_new);

    return 0;
}