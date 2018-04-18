/* $begin select */
#define MAXEVENTS 64

#include "csapp.h"
#include "sbuf.h"

#include<errno.h>
#include <unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/epoll.h>
#include<sys/socket.h>
#include<string.h>

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

static const char *static_headers = "Connection: close\r\nProxy-Connection: close\r\n\r\n";

int currentAddition;


sbuf_t sbuf;
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct {
    char* request;
    char* response;
    int maxSize;
} request_response;

request_response cache_data[10];

//void command(void);

int main(int argc, char **argv)
{
	int listenfd, connfd;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	int efd;
	struct epoll_event event;
	struct epoll_event *events;
	int i;
	int len;

	size_t n;
	char buf[MAXLINE];

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}

	listenfd = Open_listenfd(argv[1]);

	if ((efd = epoll_create1(0)) < 0) {
		fprintf(stderr, "error creating epoll fd\n");
		exit(1);
	}

	event.data.fd = listenfd;
	event.events = EPOLLIN;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &event) < 0) {
		fprintf(stderr, "error adding event\n");
		exit(1);
	}

	/* Buffer where events are returned */
	events = calloc(MAXEVENTS, sizeof(event));

	while (1) {
		// wait for event to happen (no timeout)
		n = epoll_wait(efd, events, MAXEVENTS, -1);

		for (i = 0; i < n; i++) {
			if ((events[i].events & EPOLLERR) ||
					(events[i].events & EPOLLHUP) ||
					(events[i].events & EPOLLRDHUP)) {
				/* An error has occured on this fd */
				fprintf (stderr, "epoll error on fd %d\n", events[i].data.fd);
				close(events[i].data.fd);
				continue;
			}

			if (listenfd == events[i].data.fd) { //line:conc:select:listenfdready
				clientlen = sizeof(struct sockaddr_storage);
				connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);

				// add event to epoll file descriptor
				event.data.fd = connfd;
				event.events = EPOLLIN;
				if (epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &event) < 0) {
					fprintf(stderr, "error adding event\n");
					exit(1);
				}
			} else { //line:conc:select:listenfdready
				len = recv(events[i].data.fd, buf, MAXLINE, 0);
				if (len == 0) { // EOF received
					// closing the fd will automatically unregister the fd
					// from the efd
					close(events[i].data.fd);
				} else {
					printf("Received %d bytes\n", len);
					send(events[i].data.fd, buf, len, 0);
				}
			}
		}
	}
	free(events);
}






void threadHandleStuff(int connfd) {

    char type[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char* input = malloc(MAX_OBJECT_SIZE);

    rio_t rio;
    Rio_readinitb(&rio, connfd);
    if(!Rio_readlineb(&rio, input, MAXLINE))
        return;

    sscanf(input, "%s %s %s", type, uri, version);

    char* request = strrchr(uri + 7, '/');
    //printf("Request: %s\n", request);


    //TODO: right here, check the cache for similar URI's
    int i;
    for(i = 0; i < 10; i++) {
        if(i >= currentAddition) {
            break;
        }
        request_response* temp = &cache_data[i];

        if(temp == NULL) {
            break;
        }

        if(strcmp(temp->request, uri) == 0) {
            //the request matches the cache, return the cached data
            rio_writen(connfd, temp->response, temp->maxSize);
            printf("cached response: %s\n", uri);
            return;
        }
    }
    //if we get here, then there is no cached information for the request, get it new
    printf("non-cached response: %s\n", uri);



    int requestLocation = request - uri;

    char* host = Malloc(sizeof(char) * (requestLocation-7));
    memcpy(host, uri+7, sizeof(char) * (requestLocation-7));

    char* default_port = "80";

    char* port = strrchr(host, ':');
    if(port == NULL) {
        //error stuff
        port = default_port;
    } else {
        //normal, there is a port, refresh how the host looks

        //TODO: increment the pointer by one somehow
        port = port + 1;


        int portLocation = (port - host) - 1;
        char* tempHost = Malloc(sizeof(char) * (portLocation));
        memcpy(tempHost, host, sizeof(char) * (portLocation));
        host = tempHost;
    }

    //printf("Host: %s\n", host);
    //printf("Port: %s\n", port);
    //printf("Request: %s\n", request);

    int sendingFD = open_clientfd(host, port);

    char* finalRequest = malloc(MAX_OBJECT_SIZE);
    memset(finalRequest, 0, MAX_OBJECT_SIZE);

    strcat(finalRequest, "GET ");
    strcat(finalRequest, request);
    strcat(finalRequest, " HTTP/1.0\r\n");
    strcat(finalRequest, "Host: ");
    strcat(finalRequest, host);
    strcat(finalRequest, "\r\n");
    strcat(finalRequest, user_agent_hdr);
    strcat(finalRequest, static_headers);

    //printf("%s", finalRequest);

    rio_writen(sendingFD, finalRequest, strlen(finalRequest));

    char tempResponse[MAX_OBJECT_SIZE];
    char fullResponse[MAX_OBJECT_SIZE];
    int size;

    int totalSize = 0;
    while((size = recv(sendingFD, tempResponse, MAX_OBJECT_SIZE, 0)) > 0) {

        int i;
        for(i = 0; i < size; i++) {
            fullResponse[totalSize] = tempResponse[i];
            totalSize++;
        }
    }



    request_response * temp3 = &cache_data[currentAddition];
    temp3->request = uri;
    temp3->response = fullResponse;
    temp3->maxSize = totalSize;
    currentAddition++;
    if(currentAddition > 9) {
        currentAddition = 0;
    }


    /*int i;
    for(i = 0; i < totalSize; i++) {
        //printf("%c", fullResponse[i]);
    }*/

    rio_writen(connfd, fullResponse, totalSize);

}




/* $end select */
