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
#include <semaphore.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_WRITE_SIZE 65535 //max tcp data length

//socket file descriptor
int fd_socket;
//new socket connection file descriptor
int fd_socket_new;
//port number
int portno;

//server and client addresses
struct sockaddr_in server_addr, client_addr;

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
        fprintf(logfile, "\nProducer - ERROR (" __FILE__ ":%d) -- %s\n", __LINE__, strerror(errno));
        fflush(logfile);
        fclose(logfile);
        printf("\tAn error has been reported on log file.\n");
        fflush(stdout);
        exit(-1);
    }
    return retval;
}

void random_string_generator(char buffer[])
{
    for (int i = 0; i < size; i++)
    {
        int char_index = 32 + rand() % 94;
        buffer[i] = char_index;
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

void send_array(char buffer[])
{
    //number of cycles needed to send all the data
    int cycles = size / MAX_WRITE_SIZE + (size % MAX_WRITE_SIZE != 0 ? 1 : 0);

    //write on log file
    fprintf(logfile, "producer - starting sending array...\n");
    fprintf(logfile, "producer - there will be %d cycles\n", cycles);
    fflush(logfile);

    //sending data to consumer divided into blocks of dimension max_write_size
    for (int i = 0; i < cycles; i++)
    {
        char segment[MAX_WRITE_SIZE];
        for (int j = 0; j < MAX_WRITE_SIZE && ((i * MAX_WRITE_SIZE + j) < size); j++)
        {
            segment[j] = buffer[i * MAX_WRITE_SIZE + j];
        }

        check(write(fd_socket_new, segment, MAX_WRITE_SIZE));
    }
    //write on log file
    fprintf(logfile, "producer - array sent!\n");
    fflush(logfile);
}

int main(int argc, char *argv[])
{
    //open log file in write mode
    logfile = fopen("./../logs/socket_log.txt", "a");
    if (logfile == NULL)
    {
        printf("an error occured while creating sockets's log File\n");
        return 0;
    }
    fprintf(logfile, "starting producer\n");
    fflush(logfile);

    //getting size from console
    if (argc < 2)
    {
        fprintf(stderr, "Producer - ERROR, no size provided\n");
        exit(0);
    }
    size = atoi(argv[1]) * 1000000;
    //write on log file
    fprintf(logfile, "producer - received size of %dMB\n", size/1000000);
    fflush(logfile);

    //getting mode from console
    if (argc < 3)
    {
        fprintf(stderr, "Producer - ERROR, no mode provided\n");
        exit(0);
    }
    mode = atoi(argv[2]);
    //write on log file
    fprintf(logfile, "producer - received mode %d\n", mode);
    fflush(logfile);

    //getting port number
    if (argc < 4)
    {
        fprintf(stderr, "Producer - ERROR, no port provided\n");
        exit(0);
    }
    portno = atoi(argv[3]);
    //write on log file
    fprintf(logfile, "producer - received portno %d\n", portno);
    fflush(logfile);

    //randomizing seed for random string generator
    srand(time(NULL));

    //the process must handle SIGUSR1 signal
    signal(SIGUSR1, transfer_complete);

    //create socket
    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_socket < 0)
    {
        check(-1);
    }

    //write on log file
    fprintf(logfile, "producer - socket created\n");
    fflush(logfile);

    //set server address for connection
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portno);

    //bind socket
    if (check(bind(fd_socket, (struct sockaddr *)&server_addr,
                   sizeof(server_addr))) < 0)
    {

        check(-1);
    }

    //write on log file
    fprintf(logfile, "producer - socket bound\n");
    fflush(logfile);

    //wait for connections
    check(listen(fd_socket, 5));

    //enstablish connection
    int client_length = sizeof(client_addr);
    fd_socket_new = check(accept(fd_socket, (struct sockaddr *)&client_addr, &client_length));
    if (fd_socket_new < 0)
    {
        check(-1);
    }

    //write on log file
    fprintf(logfile, "producer - connection accepted\n");
    fflush(logfile);

    //sending pid to consumer
    pid_t pid = getpid();
    check(write(fd_socket_new, &pid, sizeof(pid)));

    //write on log file
    fprintf(logfile, "producer - pid %d sent\n", pid);
    fflush(logfile);

    //switch between dynamic allocation or standard allocation
    if (mode == 0)
    {
        //dynamic allocation of buffer
        char *buffer = (char *)malloc(size);

        //generating random string
        random_string_generator(buffer);

        //get time of when the transfer has started
        check(gettimeofday(&start_time, NULL));

        //writing buffer on pipe
        send_array(buffer);

        //delete buffer from memory
        free(buffer);
    }
    else
    {
        //increasing stack limit to let the buffer be instantieted correctly
        struct rlimit limit;
        limit.rlim_cur = (size + 5) * 1000000;
        limit.rlim_max = (size + 5) * 1000000;
        check(setrlimit(RLIMIT_STACK, &limit));

        char buffer[size];

        //generating random string
        random_string_generator(buffer);

        //get time of when the transfer has started
        check(gettimeofday(&start_time, NULL));

        //writing buffer on pipe
        send_array(buffer);
    }

    //wait until transfer is complete
    while (flag_transfer_complete == 0)
    {
        ;
    }

    printf("\tsocket time: %d ms\n", transfer_time);
    fflush(stdout);
    //write on log file
    fprintf(logfile, "time: %d ms\n", transfer_time);
    fflush(logfile);

    //writing elapsed time to general time file
    FILE *timefile = fopen("./../logs/times.txt", "a");
    if (timefile == NULL)
    {
        printf("\tan error occured while opening times file\n");
        return 0;
    }
    fprintf(timefile, "socket - data size %dMB - time: %d ms\n", size/1000000, transfer_time);
    fflush(timefile);
    fclose(timefile);

    //close socket
    check(close(fd_socket));
    check(close(fd_socket_new));

    //write on log file
    fprintf(logfile, "producer - socket closed\n");
    fflush(logfile);

    //close log file
    fclose(logfile);

    return 0;
}