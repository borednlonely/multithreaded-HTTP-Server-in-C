
#ifndef server_h
#define server_h

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

struct Server {
    char *port;
    int   backlog;
    int   socket;
    void (*launch)(struct Server *server);
};

struct Server server_constructor(char *port, int backlog,
    void (*launch)(struct Server *server));

#endif