
#ifndef http_response_h
#define http_response_h

#include <stddef.h>   // for size_t

//decides what to send
//fd = connected socket to write the response to 
//method = slice pointing into the request buffer 
//method_len = length of method 
// path = same as method, slice pointing into request buffer 
// path_len = lenght of path
void handle_request(int fd,
                    const char *method, size_t method_len,
                    const char *path, size_t path_len);

#endif