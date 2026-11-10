# 💽 Multithreaded HTTP Server in C 📁
Small Multithreaded HTTP static file server in C using POSIX sockets and picohttpparser. It accepts connections over IPv4 and IPv6 and hands each one to its own thread then parses the request with [picohttpparser](https://github.com/h2o/picohttpparser), then serves files from a `www/` directory.

This is just a fun project I wanted to attempt, it's not ready for internet use! 😅

## Features 📋🧐
Thread per connection concurrency with detached pthreads, so a slow client only blocks its own thread

IPv4/IPv6 socket setup using `getaddrinfo`

Incremental request reading: loops on read until the parser reports a completed request. So requests split across TCP segments are handled

Zero copy parsing: method and path are pointer and length slices into the read buffer and never copied

Static file serving for `GET`, streamed in 4KB chunks with a Content-Length header

`SO_REUSEADDR` so the server can restart immediately on the same port!

## Get started 😶‍🌫️
Demo video coming soon!

This project requires a POSIX system (Linux or macOS or WSL) and a C compiler. 

to compile:
`gcc -Wall -Wextra -o server test.c server.c http_response.c picohttpparser.c -pthread`
`./server`

The server listens on port 8080 so in another terminal you do:
sh curl -i http://localhost:8080/index.html OR whatever you named your file 

If you make a custom webpage you can check it out in your browser at http://localhost:8080/[filename.html]

## Limitations 🤔
 Right now the project sends every file as text/html, so whatever styling you want to do has to be inside the webpage instead of linking to sep CSS file....
  
 I also found a bug when you do http://localhost:8080/ where i only get the content length but not the body. TBD.

 Also no thread pools so it's just thread per connection

## Credits 🥳
- [picohttpparser](https://github.com/h2o/picohttpparser) by Kazuho Oku and others, MIT license
- `sendall` and `get_in_addr` are adapted from this awesome guide that really helped me make this project, so please check it out! [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
