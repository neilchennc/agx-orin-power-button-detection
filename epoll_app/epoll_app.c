/**
 * Simple user space application used to monitor interrupts/events occurred
 * from custom Linux device driver via epoll.
 *
 * Author: Neil Chen <neilchennc@gmail.com>
 * Github: https://github.com/neilchennc/agx-orin-power-button-detection
 * Created at: 2023-02-22
 * Version: 1.0
 */

#include <fcntl.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define EPOLL_SIZE 128
#define MAX_EVENTS 16
#define BUFFER_SIZE 64

#define TARGET_DEVICE "/dev/neil-dev"

int main()
{
    char buff[BUFFER_SIZE];
    int fd, epoll_fd, event_count;
    ssize_t i, n;
    struct epoll_event ev, events[MAX_EVENTS];

    fd = open(TARGET_DEVICE, O_RDWR | O_NONBLOCK);

    if (fd < 0)
    {
        perror("open device failed");
        exit(EXIT_FAILURE);
    }

    epoll_fd = epoll_create(EPOLL_SIZE);

    if (epoll_fd < 0)
    {
        perror("epoll_create failed");
        exit(EXIT_FAILURE);
    }

    ev.data.fd = fd;
    ev.events = (EPOLLIN | EPOLLOUT);

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev))
    {
        perror("failed to add file descriptor to epoll\n");
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        puts("epoll_wait...");

        event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        if (event_count < 0)
        {
            perror("epoll_wait failed");
            break;
        }

        printf("event count: %d\n", event_count);

        for (i = 0; i < event_count; i++)
        {
            if (events[i].events & EPOLLIN)
            {
                n = read(events[i].data.fd, &buff, BUFFER_SIZE - 1);
                if (n > 0)
                {
                    buff[n] = '\0';
                    printf("EPOLLIN: buff read: %s\n", buff);
                }
                else
                {
                    perror("read failed");
                }
            }

            if (events[i].events & EPOLLOUT)
            {
                strcpy(buff, "Data from the user space");
                n = write(events[i].data.fd, &buff, strlen(buff));
                if (n > 0)
                {
                    printf("EPOLLOUT: buff wrote: %s\n", buff);
                }
                else
                {
                    perror("write failed");
                }
            }
        }
    }

    if (close(epoll_fd))
    {
        perror("Failed to close epoll file descriptor");
        return EXIT_FAILURE;
    }

    if (close(fd))
    {
        perror("failed to close file descriptor");
        return EXIT_FAILURE;
    }

    return 0;
}
