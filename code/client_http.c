#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>

#define BUFSIZE 100

int main(int argc, char **argv){
  if(argc != 4){
    fprintf(stderr, "Usage: %s [bla bla ....]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int fd, sc, n;
  char buf[BUFSIZE];
  char requete[200];
  unsigned short port;
  struct sockaddr_in dest;
  socklen_t destlen = sizeof(destlen);

  port = atoi(argv[2]);
  
  sprintf(requete, "GET /%s HTTP/1.1\nHost: 127.0.0.1\n\n", argv[3]);

  if((sc = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    perror("socket");
    exit(1);
  }

  memset((char *) &dest, 0, sizeof(struct sockaddr_in));
  dest.sin_family = AF_INET;
  dest.sin_addr.s_addr = inet_addr(argv[1]);
  dest.sin_port = htons(port);

  if(connect(sc, (struct sockaddr *) &dest, sizeof(dest)) == -1){
    perror("connect");
    exit(1);
  }

  if(write(sc, requete, strlen(requete)) == -1){
    perror("write");
    exit(1);
  }

  while((n = read(sc, buf, BUFSIZE)) > 0){
    if(write(1, buf, n) == -1){
      perror("write");
      exit(1);
    }
  }
  printf("\n");
  shutdown(sc, 2);
  close(sc);
  return EXIT_SUCCESS;
}
