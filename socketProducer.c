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

#define SIZE 100 * 1000000

int fd_socket;
int fd_socket_new;
int portno;
struct sockaddr_in server_addr, client_addr;

char buffer[SIZE] = "";
struct timeval start_time, stop_time;
int flag_transfer_complete = 0;
int transfer_time;
int max_write_size;

sem_t mutex;

void random_string_generator()
{
    printf("generating random array...");
    for (int i = 0; i < SIZE; i++)
    {
        int char_index = 32 + rand() % 94;
        buffer[i] = char_index;
    }
    printf("\n\nrandom array generated!\n\n");
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

void send_array()
{
    // FILE *file = fopen("prod.txt", "w");
    // fprintf(file, "%s", buffer);
    // fflush(file);
    // fclose(file);
    int cycles = SIZE / max_write_size + (SIZE % max_write_size != 0 ? 1 : 0);
    //sending data divided in blocks of max_write_size size
    for (int i = 0; i < cycles; i++)
    {
        char segment[max_write_size];
        for (int j = 0; j < max_write_size && ((i * max_write_size + j) < SIZE); j++)
        {
            segment[j] = buffer[i * max_write_size + j];
        }
        // while(sem_trywait(&mutex)==0){
        //     usleep(1000);
        // }
        // sem_wait(&mutex);
        // printf("%d - write: %ld\n", i, write(fd_socket_new, segment, max_write_size));
        write(fd_socket_new, segment, max_write_size);
        // sem_post(&mutex);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Producer - ERROR, no port provided\n");
        exit(0);
    }
    portno = atoi(argv[1]);

    //randomizing seed for random error generator
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

    //initialize semaphore for coordinating reading and writing
    if (sem_init(&mutex, 1, 1) == 1)
    {
        perror("semaphore initialization failed");
    }

    //defining max size for operations and files

    int socklen = 4;
    int send_buffer_size;
    int receive_buffer_size;
    getsockopt(fd_socket_new, SOL_SOCKET, SO_SNDBUF, &send_buffer_size, &socklen);
    getsockopt(fd_socket_new, SOL_SOCKET, SO_RCVBUF, &receive_buffer_size, &socklen);
    max_write_size = send_buffer_size <= receive_buffer_size ? send_buffer_size : receive_buffer_size;
    max_write_size = 65000;
    //generating random strings

    random_string_generator();
    

    //get time of when the transfer has started
    gettimeofday(&start_time, NULL);

    //writing buffer on socket
    send_array();

    while (flag_transfer_complete == 0)
    {
        ;
    }

    printf("time: %d ms\n", transfer_time);
    fflush(stdout);

    //close socket
    close(fd_socket);
    close(fd_socket_new);

    printf("prod %ld\n", strlen(buffer));
    return 0;
}