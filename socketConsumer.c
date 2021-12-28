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
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_WRITE_SIZE 65535 //max tcp data length

//socket file descriptor
int fd_socket;
//port number
int portno;
//server address
struct sockaddr_in server_addr;
//socket server
struct hostent *server;


//pid of producer process used to send signals
pid_t producer_pid;

//variables for select function

struct timeval timeout;
fd_set readfds;

//buffer size
int size;

//memory mode
int mode;

void receive_array(char buffer[])
{
    //number of cycles needed to send all the data
    int cycles = size / MAX_WRITE_SIZE + (size % MAX_WRITE_SIZE != 0 ? 1 : 0);

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
            FD_SET(fd_socket, &readfds);

        } while (select(FD_SETSIZE + 1, &readfds, NULL, NULL, &timeout) < 0);

        //read string from producer
        char segment[MAX_WRITE_SIZE];
        read(fd_socket, segment, MAX_WRITE_SIZE);

        //add every segment to entire buffer
        if (i == cycles - 1)
        {

            int j = 0;
            while ((i * MAX_WRITE_SIZE + j) < size)
            {
                buffer[i * MAX_WRITE_SIZE + j] = segment[j];
                j++;
            }
        }
        else
        {
            strcat(buffer, segment);
        }
    }
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

    //getting mode from console
    if (argc < 4)
    {
        fprintf(stderr, "Consumer - ERROR, no mode provided\n");
        exit(0);
    }
    mode = atoi(argv[2]);

    //getting port number
    if (argc < 5)
    {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[4]);

    //create socket
    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_socket < 0)
    {
        perror("Consumer - ERROR opening socket");
        exit(0);
    }

    //get host
    server = gethostbyname(argv[3]);
    if (server == NULL)
    {
        fprintf(stderr, "Consumer - ERROR, no such host\n");
        exit(0);
    }

    //set server address for connection
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&server_addr.sin_addr.s_addr,
          server->h_length);
    server_addr.sin_port = htons(portno);

    //open new connection
    if (connect(fd_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Consumer - ERROR connecting to server");
        exit(0);
    }

    //receiving pid from producer
    int sel_val;
    do //wait until pid is ready
    {
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;

        FD_ZERO(&readfds);
        //add the selected file descriptor to the selected fd_set
        FD_SET(fd_socket, &readfds);

        sel_val = select(FD_SETSIZE + 1, &readfds, NULL, NULL, &timeout);

    } while (sel_val <= 0);

    read(fd_socket, &producer_pid, sizeof(producer_pid));

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
        struct rlimit limit;
        limit.rlim_cur = (size + 5) * 1000000;
        limit.rlim_max = (size + 5) * 1000000;
        setrlimit(RLIMIT_STACK, &limit);

        char buffer[size];
        //receive array from producer
        receive_array(buffer);
    }

    //transfer complete. Sends signal to notify the producer
    kill(producer_pid, SIGUSR1);

    //close socket
    close(fd_socket);

    return 0;
}
