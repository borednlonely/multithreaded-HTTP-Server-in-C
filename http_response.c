
#include "http_response.h"
#include <string.h>     // memcmp, strlen
#include <unistd.h>     // close
#include <stdio.h>      // perror

// sendall is defined over in test.c. This file needs to call it, so we declare here for compiler. 
//This call connects to the real sendall in test.c at build time

int sendall(int s, char *buf, int *len);



void serve_file(int fd, const char *path, size_t path_len)
{
  //this is for building the file path, and we're looking through the www directory for the file.  so www/index.html
    char filepath[1024];
    // %.*s prints exact path_len chars
    snprintf(filepath, sizeof(filepath), "www%.*s", (int)path_len, path);

    // open the file
    FILE *f = fopen(filepath, "rb");


    //returns NULL if file doesn't exist 
    if (f == NULL) {
            char *notfound = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            int len = strlen(notfound);
            sendall(fd, notfound, &len);
            return;
        }


    // finding the file size:
    fseek(f, 0, SEEK_END); //jumps to the very end 
    long filesize = ftell(f); //tells us where we are which is the size 
    fseek(f, 0, SEEK_SET); //jumps back and sets read position at the start

    // builds and sends the response header, content type set to html
    char header[256];
    int header_len = snprintf(header, sizeof(header), //returns how many characters it wrote for sendall 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %ld\r\n"
        "\r\n",
        filesize);
    sendall(fd, header, &header_len); 

    // send the file body in chunks (sendall handles partial sends)
    char filebuf[4096];
    size_t bytes_read;
    //stores bytes in filebuf, size of bytes: 1, sizeof (4096) gives us this many items, f is our file, > 0 means we found something to send
    while ((bytes_read = fread(filebuf, 1, sizeof(filebuf), f)) > 0) {
        int chunk = (int)bytes_read; //gives sendall copy of bytes_read so sendall changes this instead of loop counter 
        sendall(fd, filebuf, &chunk);
    }

    fclose(f);
}

void handle_request(int fd,
                    const char *method, size_t method_len,
                    const char *path, size_t path_len)
{

    //look at the method and pick the right response
    //method is: (pointer + length). We compare with length check + memcmp 
    
    char *response; //each branch sets this 

    if (method_len == 3 && memcmp(method, "GET", 3) == 0) {
        serve_file(fd, path, path_len);
        close(fd);
        return;                 // GET fully handled by serve_file
    }
    else if (method_len == 4 && memcmp(method, "HEAD", 4) == 0) {
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n";
    }
    else if (method_len == 4 && memcmp(method, "POST", 4) == 0) {
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 12\r\n\r\nHELLO (POST)";
    }
    else {
        response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
    }

    int len = strlen(response);
    if (sendall(fd, response, &len) == -1) {
        perror("sendall");
    }
    close(fd); //conection ends here
}