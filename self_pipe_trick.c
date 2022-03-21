#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h> 

#define max(m,n) ((m) > (n) ? (m) : (n))


/* File descriptors for pipe */
static int pfd[2];

static void
handler(int sig)
{
    int savedErrno;                    
    printf("[enter] sig handler\n");

    savedErrno = errno;
    if (write(pfd[1], "x", 1) == -1 && errno != EAGAIN) {
        printf("failed to write pipe\n");
        fflush(stdout);
        exit(1);
    }
    printf("[exit] sig handler\n");
    errno = savedErrno;
}

int main(int argc, char *argv[])
{
    /* Initialize 'timeout' */
    struct timeval *pto = NULL;

    /* [SELECT] Build the 'readfds' */
    int fd = 0;
    int nfds = 0;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);

    if (fd >= nfds)
      nfds = fd + 1;

    /* [PIPE] Create pipe before establishing signal handler to prevent race */
    if (pipe(pfd) == -1) {
        fflush(stdout);
        printf("failed to create pipe\n");
        fflush(stdout);
        exit(1);
    }

    FD_SET(pfd[0], &readfds);
    nfds = max(nfds, pfd[0] + 1);
    printf("nfds: %d\n", nfds);

    /* Make read and write ends of pipe nonblocking */
    int flags;
    flags = fcntl(pfd[0], F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(pfd[0], F_SETFL, flags);  

    flags = fcntl(pfd[1], F_GETFL);  
    flags |= O_NONBLOCK;               
    fcntl(pfd[1], F_SETFL, flags);


    struct sigaction sa;    
    sigemptyset(&sa.sa_mask);
     /* Restart interrupted reads()s */
    sa.sa_flags = SA_RESTART;          
    sa.sa_handler = handler;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        printf("failed to sigaction\n");
        fflush(stdout);
        exit(1);
    }

    int ready;
    while ((ready = select(nfds, &readfds, NULL, NULL, pto)) == -1 && errno == EINTR) {
        printf("select continue ready: %d, errno[%d]: %s\n", ready, errno, strerror(errno));
        fflush(stdout);
        continue;
    }
    printf("select break ready: %d, errno[%d]: %s\n", ready, errno, strerror(errno));

    if (ready == -1) {
        printf("failed to get ready: %d\n", ready);
        fflush(stdout);
        exit(1);
    }

    /* Handler was called */
    char ch;
    if (FD_ISSET(pfd[0], &readfds)) {   
        printf("A signal was caught\n");
        fflush(stdout);

        /* Consume bytes from pipe */
        for (;;) {                      
            if (read(pfd[0], &ch, 1) == -1) {
                if (errno == EAGAIN) {
                    break;
                } else {
                    printf("failed to read\n");
                    fflush(stdout);
                    exit(1);
                }
            }
            /* Perform any actions that should be taken in response to signal */
        }
    }
        

    /* Examine file descriptor sets returned by select() to see
       which other file descriptors are ready */
    /* And check if read end of pipe is ready */
    exit(0);
}

