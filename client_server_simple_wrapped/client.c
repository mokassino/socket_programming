#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "wrapped.h"


#define BUFSIZE 1024

int main(int argc, char *argv[]){

        int err, sockfd;
        socklen_t len;
        struct sockaddr_in address; //address

        char buffer[BUFSIZE];

        struct sockaddr_in * ptr_address = &address;

        Socket(&sockfd);

        makeSockaddr(ptr_address,"127.0.0.1", 9736, &len);

        Connect(sockfd, ptr_address, len);

        FullRead(sockfd, buffer, BUFSIZE);

        printf("%s", buffer);
        FullRead(sockfd, buffer, BUFSIZE);
        printf("%s",buffer);
        close(sockfd);


        exit(0);
}
