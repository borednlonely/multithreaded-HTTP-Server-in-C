#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

struct Server server_constructor(char *port, int backlog,
    void (*launch)(struct Server *server)) {
    struct Server server;

    server.port = port;
    server.backlog = backlog;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC; //IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags    = AI_PASSIVE; //fill in own IP

    int rv;
    //rv stores result of getaddrinfo and checks if it got something other than 0, which means something failed
    if ((rv = getaddrinfo(NULL, server.port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv)); //gai_strerror makes error # into readable string 
        exit(1);
    }

    int opt = 1;
    //loop through each address and stop at the one that works
    // after each pass follow pointer to the next one 
    for (p = servinfo; p != NULL; p = p->ai_next) {
        server.socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol); //creates a socket for this address option 
        if (server.socket < 0) {
            perror("Failed to connect socket");
            continue; //skips if error and goes onto next address
        }
        //restart server immedietely. Makes bind accept port that's still in TIME_WAIT
        //sets REUSEADDR to on (val of opt which is 1) which makes this work
        setsockopt(server.socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        if (bind(server.socket, p->ai_addr, p->ai_addrlen) < 0) {
            close(server.socket); //closing socket bcs bind failed, but socket was created, so close it to avoid fd leakage :P
            perror("Failed to bind socket");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "Failed to bind to any address\n");
        exit(1);
    }

    if (listen(server.socket, server.backlog) < 0) {
        perror("Failed to start listening");
        exit(1);
    }

    server.launch = launch;
    return server;
}