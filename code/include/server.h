#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>		/*for struct sockaddr_un*/
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/wait.h>		/*for waitpid()*/
#include <arpa/inet.h>		/*for htons()*/
#include <fcntl.h>		/*for O_RDWR*/
#include <pthread.h>
#include <signal.h>

#define MESSG_SIZE 100
#define MAX_PENDING 15
#define BUFF_SIZE 1000
#define SHUT_TYPE 2 		/*mode de deconnexion*/
#define RQ_NUM_MAX 3
