#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/select.h>

#include "wrapped.h"


#define BUFSIZE 1024

int counter = 0;

int count_connections(char *buffer){ //return written bytes
        int bwritten = 0;

        const char * msg = "Contatore collegamenti fin ora: %d\n";
        bwritten = snprintf(buffer, BUFSIZE, msg ,counter);

        return bwritten;
}

void current_time(char *buf){
        int hours, minutes, seconds;
        time_t now;
        time(&now);
        struct tm *local = localtime(&now);
        hours = local->tm_hour;
        minutes = local->tm_min;
        seconds = local->tm_sec;
        snprintf(buf, BUFSIZE-1, "%02d:%02d:%02d\n",hours,minutes,seconds);
}

int count_letters(char *string){
    int i=0, c=0;
    int try_size = (int)sizeof(string);

    while (string[i] != '\0'){
        if (string[i] != ' ' && string[i] != '\n' && string[i] != '\0'){
            c++;
        }
        i++;
    }

    return c;
}

int main(int argc, char *argv[]){

        socklen_t len;
        int offset = 0;
        int pid;
        int port;

        int server_sockfd, client_sockfd;

        struct sockaddr_in client_address;
        struct sockaddr_in * ptr_client_address = &client_address;

        struct sockaddr_in server_address;
        struct sockaddr_in * ptr_server_address = &server_address;

        char IPaddress[INET6_ADDRSTRLEN];
        char *ip;

        char i_buffer[BUFSIZE], o_buffer[BUFSIZE];
        socklen_t client_len = sizeof(client_address);

        if (argc < 3){
                fprintf(stderr, "Inserisci un indirizzo e una porta in input\n");
                exit(1);
        }

        port = atoi(argv[2]);
        struct hostent * host = gethostbyname(argv[1]);

        if (host == NULL){
                fprintf(stderr, "Bad Address\n");
                exit(1);
        }

        ip = inet_ntoa( *((struct in_addr*)host->h_addr_list[0]) );

        Socket(&server_sockfd); //socket creation:
        makeSockaddr(ptr_server_address,ip, port, &len);
        Bind(server_sockfd, ptr_server_address, len);
        Listen(server_sockfd);

        signal(SIGCHLD, SIG_IGN);

        fd_set set;
        int list_fd = server_sockfd;
        int fd=-1;
        int maxfd = list_fd; 
        int fd_open[maxfd+5];

        for(int i=0;i<maxfd+5;i++) fd_open[i]=0;

        int n=0,i=0;
        fd_open[maxfd] = 1;
        FD_SET(list_fd, &set);
        
        while(1){
                //Questo blocco di istruzioni fino alla parentesi
                // c'?? perch?? nel manuale di select si legge che bisogna reistanziare il set prima di ogni chiamata select() su un set.
                FD_ZERO(&set);
                for(int i=list_fd;i<=maxfd;i++){ //Setta i file descriptor da list_fd a max_fd dentro set
                    printf("fd_open[%d]: %d\n", i, fd_open[i]);
                    if (fd_open[i]!= 0){
                        FD_SET(i,&set);
                    }

                }

                while ( ((n = select(maxfd+1, &set, NULL, NULL, NULL)) < 0) && (errno == EINTR)); //Aspetta che un descrittore in lettura sia libero
                //In n verr?? messo il numero di descrittori di file disponibili.
                if ( n < 1){ //Non si ?? riusciti a selezionare un file descriptor
                    perror("E: ");
                    exit(1);
                }

                if (FD_ISSET(list_fd, &set)){ //Se il file descriptor in ascolto ?? settato, quindi c'?? una nuova connessione
                    n--;
                    Accept(&fd, list_fd, ptr_client_address, &client_len); //Accetta la nuova connessione
                    FD_SET(fd, &set);
                    fd_open[fd]=1; //Segnala che il relativo file descriptor ?? correntemente aperto da una connessione
                    if(maxfd < fd)
                        maxfd = fd;

                }
                i = list_fd; //parti dal list_fd
                while(n!=0){ //Cicla finch?? ci sono connessioni attive
                    i++; //Tipicamente 0 1 2 sono input,output ed error, 3 ?? listening, partiamo da 4
                    if (fd_open[i] == 0){ //se ?? messo a 0, la connessione ?? chiusa quindi passa al successivo
                        continue;
                    }
                    if (FD_ISSET(i, &set)){ //i ?? un file descriptor attivo cio?? pronto a ricevere/trasmettere dati
                        n--;

                        int nread = read(i, i_buffer, (size_t)BUFSIZE); //Leggi dal file descriptor del socket attivo
                        if (nread < 0){ 
                            perror("Errore lettura");
                            exit(1);
                        }
                        printf("buffer: %s\n",i_buffer);

                        if (nread == 0){ //Se la connessione ?? chiusa
                            fd_open[i]=0; //Setta nell'array che il relativo fd ?? chiuso
                            if (maxfd == i){ //Se il socket attivo ?? quello con numero massimo
                                while(fd_open[--i]==0){
                                    Close(i+1); //Per evitare di fare overflow di descrittori di file dentro fd_open
                                }
                                Close(i+1);            
                                 //cicla decrementato i finch?? non ne trovi uno settato ad 1
                                maxfd=i; //Quello sar?? il nuovo fd
                                break;
                                
                            }
                        continue;
                        }

                        if (nread > 0){
                            snprintf(o_buffer, BUFSIZE, "Numero di caratteri: %d\n", count_letters(i_buffer));
                            FullWrite(i, o_buffer, (size_t)BUFSIZE);
                        }
                    }
                }

                /*
                Accept(&client_sockfd, server_sockfd, ptr_client_address, &client_len );

                
                if( (pid = fork()) == 0){
                    Close(server_sockfd);
                    
                    int c = 5;
                    while (c-->0){
                        snprintf(o_buffer,BUFSIZE, "Inserisci una stringa\n");
                        FullWrite(client_sockfd, o_buffer, (size_t)BUFSIZE);

                        FullRead(client_sockfd, i_buffer, (size_t)BUFSIZE);

                        snprintf(o_buffer, BUFSIZE, "Numero di caratteri: %d\n", count_letters(i_buffer));
                        FullWrite(client_sockfd, o_buffer, (size_t)BUFSIZE);
                    }

                    Close(client_sockfd);
                    exit(0);

                }else if(pid > 0){
                    //Logging
                    char buffer[BUFSIZE];
                    char *ipaddress = inet_ntoa(client_address.sin_addr);
                    printf("Nuova connessione:\tnumero: %d\tindirizzo ip: %s\n", ++counter, ipaddress);

                    Close(client_sockfd);
                }else{
                    perror("Error while forking\n");
                    exit(1);
                }
                */
            
	}

	exit(0);
}
