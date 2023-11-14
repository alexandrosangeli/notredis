#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib> 
#include <stdio.h>
#include <cerrno>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <cassert> 
#include <errno.h>

const size_t k_max_msg = 4096;

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}


// read all:
// take file descriptor, a buffer to read into and the number of bytes to read
// read all data:
// check if n > 0, read data, check the number of bytes read into the buffer, subtract from n, update buffer to point a further mem address
// make an assertion that the bytes read is less than n
static int8_t read_all(int fd, char* buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0)
        {
            return -1;
        }
        assert((size_t) rv <= n);
        n -= (size_t) rv;
        buf += rv;
    }
    return 0;
}


static int8_t write_all(int fd, const char* buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0)
        {
            return -1;
        }
        assert((size_t) rv <= n);
        n -= (size_t) rv;
        buf += rv;
    }
    return 0;
}


static int8_t process_request(int connfd)
{
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_all(connfd, rbuf, 4);
    if (err) {
        // if errno is 0 then no other error has occured, it's just EOF
        if (errno == 0) {
            std::cout << "EOF" << std::endl;
        } else {
            std::cerr << "error in: read() header" << std::endl;
        }
        return err;
    }
    
    // write the header to a variable using memcpy (uint32)
    uint32_t msg_len;
    memcpy(&msg_len, rbuf, 4);

    // check if msg is too loing
    if (msg_len > k_max_msg)
    {
        std::cerr << "Message too long: " << k_max_msg << std::endl;
        return -1;
    }

    // request the body and print
    err = read_all(connfd, &rbuf[4], msg_len);
    if (err)
    {
        std::cerr << "error in: read() msg" << std::endl;
        return err;
    }

    // print client msg
    rbuf[4 + msg_len] = '\0';
    printf("client msg: %s\n", &rbuf[4]);

    // reply
    // write the len of the msg to wbuf
    const char reply[] = "server is replying";
    char wbuf[4 + sizeof(reply)];
    msg_len = (uint32_t) strlen(reply);

    // write the msg
    // write it to fd/socket
    memcpy(wbuf, &msg_len, 4);
    memcpy(&wbuf[4], reply, msg_len);
    err = write_all(connfd, wbuf, 4 + msg_len);
    if (err)
    {
        std::cerr << "error in: write()" << std::endl;
        return -1;
    }

    return 0;
}


int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0); // acquire a socket: IPv4, stable (SOCK_STREAM, 0) (TCP)

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);

    int rv = bind(fd, (const sockaddr*) &addr, sizeof(addr));
    if (rv) {die("bind");}

    // SOMAXCONN: constant that typically represents the maximum number of pending connections that can be queued for the socket
    rv = listen(fd, SOMAXCONN);
    if (rv) {die("listen()");}

    while(true)
    {
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);

        int connfd = accept(fd, (struct sockaddr*) &client_addr, &socklen);

        std::cout << "Accepting new connection..." << std::endl;
        if (connfd < 0) // error
        {
            continue;
        }

        while (true)
        {
            int32_t err = process_request(connfd);
            
            if (err)
            {
                break;
            }
        }
        close(connfd);
    }
}