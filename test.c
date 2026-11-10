
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>    // pthread_create, pthread_detach
#include <stdlib.h>     // malloc, free
#include "server.h"
#include <arpa/inet.h>  
#include "picohttpparser.h"
#include "http_response.h" 



int sendall(int s, char *buf, int *len)
{
    int total = 0;          // how many bytes we've sent
    int bytesleft = *len;   // how many we have left to send
    int n;

    while (total < *len) {
        n = send(s, buf + total, bytesleft, 0);
        if (n == -1) break;
        total += n;
        bytesleft -= n;
    }

    *len = total;    // return number actually sent here

    return n == -1 ? -1 : 0; // -1 on failure, 0 on success
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//IMPORTANT reminder: picohttpparser doesn't copy, allocate, or modify buffer :P

//handle connection() is our worker thread function 
//pthreads take void*, and returns void*
//runs here on its own thread, so a slow client only blcoks its own thread. not the whole server 
void *handle_connection(void *arg)
{
    int new_socket = *((int *)arg);   // copies fd out of the holder
    free(arg);                        // frees holder 

    char buffer[30000];

    // points into the buffer and dies when frame is returned 
    // method + getting method_len allows us to actually reference buffer without allocating or modifying
    // ^ path and path_len work the same.  pointer + length to reference :]

    const char *method, *path; 
    size_t method_len, path_len;
    int minor_version;
    struct phr_header headers[100]; //creating temp storage for parser. limit set to 100 headers, so >100 means the parser fails and returns -1 
    size_t num_headers; // IN: how much goes in/OUT: how many it finds. So needs to reset before every parse 

    size_t buflen = 0, prevbuflen = 0; //prevbuflen helps parser skip re-checks by storing prev bufflen count 
                                        // buflen stores the bytes in buffer so far 
    int pret;

    // loop runs until parser says it's complete 
    while (1) {
        //reads bytes from our new socket, then finds the empty space to write to, and makes sure we don't write over our buffer size 
        //buffer +bufflen = find the next free space in our buffer
        ssize_t rret = read(new_socket, buffer + buflen, sizeof(buffer) - buflen); 
        if (rret <= 0) break; //-1 is error, nothing coming in 

        prevbuflen = buflen; //stores old count 
        buflen += rret; //total bytes in buffer after each read 

        num_headers = sizeof(headers) / sizeof(headers[0]);
        pret = phr_parse_request(buffer, buflen,
                                 &method, &method_len,
                                 &path, &path_len,
                                 &minor_version,
                                 headers, &num_headers,
                                 prevbuflen);

        if (pret > 0) break;   // complete
        if (pret == -1) break; // malformed
        if (buflen == sizeof(buffer)) break; // buffer full
    }

    // hand off to response logic (sends reply, closes fd)
    handle_request(new_socket, method, method_len, path, path_len);

    return NULL;   // worker done, so thread ends
}

void launch(struct Server *server) {
    struct sockaddr_storage client;
    socklen_t address_length = sizeof(client);
    char client_ip[INET6_ADDRSTRLEN];
    int new_socket;

    while (1) {
        printf("============waiting to connect :D ==============\n");
        
        // server -> socket listens for pending connections
        // new socket creates new fd for this one client 
        //blocks until a new client shows up 
        new_socket = accept(server->socket, (struct sockaddr *)&client, &address_length);
        if (new_socket < 0) {
            perror("accept");
            continue;  // tries again if there is a bad accept 
        }
        //inet_ntop creates readable strings for us 
        inet_ntop(client.ss_family,
                  get_in_addr((struct sockaddr *)&client),
                  client_ip, sizeof(client_ip));
        printf("got connection from %s\n", client_ip);

        // give THIS connection its own memory for its fd, so worker doesnt read new_socket after next accept overwrites it 
        int *pfd = malloc(sizeof(int)); //requests new memory 
        if (pfd == NULL) { //checks if theres anymore memory, if NULL and malloc fails then close the new_socket to stop fd leakage
            perror("malloc");
            close(new_socket);
            continue;
        }
        *pfd = new_socket; //copies our fd to the new memory

        // spawn a worker for this client; don't wait for it
        pthread_t worker;
        pthread_create(&worker, NULL, handle_connection, pfd); //creates the new thread. handle_connection is where it starts, and pfd points to copy of the fd
        pthread_detach(worker); // auto cleans when thread is done
    }
}




int main(void) {
    struct Server server = server_constructor("8080", 10, launch);
    server.launch(&server);
    return 0;
}