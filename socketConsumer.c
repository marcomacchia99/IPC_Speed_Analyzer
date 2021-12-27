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
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int fd_socket;
int portno;
struct sockaddr_in server_addr;
struct hostent *server;

int max_write_size;

pid_t producer_pid;

sem_t mutex;

//variables for select function
struct timeval timeout;
fd_set readfds;

int size;

void receive_array(char buffer[])
{

    int cycles = size / max_write_size + (size % max_write_size != 0 ? 1 : 0);
    for (int i = 0; i < cycles; i++)
    {

        int sel;
        do
        {
            timeout.tv_sec = 0;
            timeout.tv_usec = 1000;
            FD_ZERO(&readfds);
            //add the selected file descriptor to the selected fd_set
            FD_SET(fd_socket, &readfds);
            sel = select(FD_SETSIZE + 1, &readfds, NULL, NULL, &timeout);
        } while (sel <= 0);
        // {
        //     printf("select\n");
        //     ;
        // }

        //read random string from producer
        char segment[max_write_size];

        // sem_wait(&mutex);
        // printf("%d - read: %ld\n", i, read(fd_socket, segment, max_write_size));
        read(fd_socket, segment, max_write_size);

        // sem_post(&mutex);

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
    // FILE *file = fopen("cons.txt", "w");
    // fprintf(file, "%s", buffer);
    // fflush(file);
    // fclose(file);
}

int main(int argc, char *argv[])
{
    //getting size from console
    if (argc < 3)
    {
        fprintf(stderr, "Consumer - ERROR, no size provided\n");
        exit(0);
    }
    size = atoi(argv[1]) * 1000000;

    //increasing stack limit to let the buffer be instantieted correctly
    struct rlimit limit;
    limit.rlim_cur = 105 * 1000000;
    limit.rlim_max = 105 * 1000000;
    setrlimit(RLIMIT_STACK, &limit);

    char buffer[size];

    if (argc < 4)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[3]);
    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_socket < 0)
    {
        perror("Consumer - ERROR opening socket");
        exit(0);
    }

    server = gethostbyname(argv[2]);
    if (server == NULL)
    {
        fprintf(stderr, "Consumer - ERROR, no such host\n");
        exit(0);
    }

    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&server_addr.sin_addr.s_addr,
          server->h_length);
    server_addr.sin_port = htons(portno);

    if (connect(fd_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Consumer - ERROR connecting to server");
        exit(0);
    }

    //receiving pid from producer
    int sel;
    do
    {
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;
        FD_ZERO(&readfds);
        //add the selected file descriptor to the selected fd_set
        FD_SET(fd_socket, &readfds);
        sel = select(FD_SETSIZE + 1, &readfds, NULL, NULL, &timeout);
    } while (sel <= 0);
    read(fd_socket, &producer_pid, sizeof(producer_pid));

    //initialize semaphore for coordinating reading and writing
    // if (sem_init(&mutex, 1, 1) == 1)
    // {
    //     perror("semaphore initialization failed");
    // }

    //defining max size for operations and files

    int socklen = 4;
    int send_buffer_size;
    int receive_buffer_size;
    getsockopt(fd_socket, SOL_SOCKET, SO_SNDBUF, &send_buffer_size, &socklen);
    getsockopt(fd_socket, SOL_SOCKET, SO_RCVBUF, &receive_buffer_size, &socklen);
    max_write_size = send_buffer_size <= receive_buffer_size ? send_buffer_size : receive_buffer_size;
    max_write_size = 65000;

    //receive array from producer
    receive_array(buffer);

    //transfer complete. Sends signal to notify the producer
    kill(producer_pid, SIGUSR1);

    //close and delete fifo
    close(fd_socket);

    return 0;
}