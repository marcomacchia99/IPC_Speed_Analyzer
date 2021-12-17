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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SIZE 100 * 1000000

int fd_socket;
int portno;
struct sockaddr_in server_addr;
struct hostent *server;

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
        FD_SET(fd_socket, &readfds);
        while (select(FD_SETSIZE + 1, &readfds, NULL, NULL, &timeout) < 0)
        {
            ;
        }

        //read random string from producer
        char segment[max_write_size];
        // read(fd_socket, segment, max_write_size);
        printf("read: %ld\n", read(fd_socket, segment, max_write_size));

        //add every segment to entire buffer
        if (i == cycles - 1)
        {
            int j = 0;
            while ((i * max_write_size + j) < SIZE)
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
    //     FILE *file = fopen("cons.txt", "w");
    // fprintf(file, "%s", buffer);
    // fflush(file);
    // fclose(file);
}

int main(int argc, char *argv[])
{

    if (argc < 3)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);
    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_socket < 0)
    {
        perror("Consumer - ERROR opening socket");
        exit(0);
    }

    server = gethostbyname(argv[1]);
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
    read(fd_socket, &producer_pid, sizeof(producer_pid));

    //defining max size for operations and files

    int socklen = 4;
    int send_buffer_size;
    int receive_buffer_size;
    getsockopt(fd_socket, SOL_SOCKET, SO_SNDBUF, &send_buffer_size, &socklen);
    getsockopt(fd_socket, SOL_SOCKET, SO_RCVBUF, &receive_buffer_size, &socklen);
    max_write_size = send_buffer_size<=receive_buffer_size? send_buffer_size: receive_buffer_size;

    //receive array from producer
    receive_array();

    //transfer complete. Sends signal to notify the producer
    kill(producer_pid, SIGUSR1);

    //close and delete fifo
    close(fd_socket);

    return 0;
}