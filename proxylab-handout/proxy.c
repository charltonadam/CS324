#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "csapp.h"

#include<errno.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/epoll.h>
#include<sys/socket.h>

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define MAXEVENTS 64

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

static const char *static_headers = "Connection: close\r\nProxy-Connection: close\r\n\r\n";


typedef struct {
    char* request;
    char* response;
    int maxSize;
} request_response;

int currentAddition;

request_response cache_data[10];

void threadHandleStuff(int connfd);

void logURI(char* uri);

struct stateSaver{
    int state;
    int clientFd;
    int sendingFd;
    char buf[MAXLINE];  //read from client
    int bufLen;
    char requestString[MAX_OBJECT_SIZE];     //what you are sending to the server
    char fullResp[MAX_OBJECT_SIZE];
    int respLen;
    int numWritten;
    int numToWrite;

};

void initialize();

void readRequest(struct stateSaver *ea);
void sendRequest(struct stateSaver *ea);
void readResponse(struct stateSaver *ea);
void sendResponse(struct stateSaver *ea);





int efd;


FILE *fp;
int listenfd;



/* You won't lose style points for including this long line in your code */
//static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char **argv)
{
    fp = fopen("log.txt", "w");



    struct stateSaver *ea;
    struct epoll_event event;
    struct epoll_event *events;
    int i, n;

    listenfd = Open_listenfd(argv[1]);

    if (fcntl(listenfd, F_SETFL, fcntl(listenfd, F_GETFL, 0) | O_NONBLOCK) < 0) {
		fprintf(stderr, "error setting socket option\n");
		exit(1);
	}

    if ((efd = epoll_create1(0)) < 0) {
		fprintf(stderr, "error creating epoll fd\n");
		exit(1);
	}

    ea = malloc(sizeof(struct stateSaver));
    ea->state = 0;
    ea->clientFd = listenfd;
    ea->bufLen = 0;
    ea->respLen = 0;
    ea->numWritten = 0;

    event.data.ptr = ea;
    event.events = EPOLLIN | EPOLLET;

    if (epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &event) < 0) {
		fprintf(stderr, "error adding event\n");
		exit(1);
	}

    events = calloc(MAXEVENTS, sizeof(event));


    while(1) {

        n = epoll_wait(efd, events, MAXEVENTS, -1);

		for (i = 0; i < n; i++) {

            if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
				/* An error has occured on this fd */
				//fprintf (stderr, "epoll error on fd %d\n", *argptr);
				close(ea->clientFd);
                close(ea->sendingFd);
				free(ea);
				continue;
			}


			ea = (struct stateSaver *)events[i].data.ptr;

            if(ea->state == 0) {
                initialize();
            } else if(ea->state == 1) {
                readRequest(ea);
            } else if(ea->state == 2) {
                sendRequest(ea);
            } else if(ea->state == 3) {
                readResponse(ea);
            } else if(ea->state == 4) {
                sendResponse(ea);
            }



		}


    }



    fclose(fp);
    return 0;
}


void initialize() {
    socklen_t clientlen;
	int connfd;
	struct sockaddr_storage clientaddr;
	struct epoll_event event;
	struct stateSaver *ea;

    clientlen = sizeof(struct sockaddr_storage);


    while ((connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen)) > 0) {

		// set fd to non-blocking (set flags while keeping existing flags)
		if (fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFL, 0) | O_NONBLOCK) < 0) {
			fprintf(stderr, "error setting socket option\n");
			exit(1);
		}

		ea = malloc(sizeof(struct stateSaver));
		ea->state = 1;

		// add event to epoll file descriptor
		ea->clientFd = connfd;
		event.data.ptr = ea;
		event.events = EPOLLIN | EPOLLET; // use edge-triggered monitoring
		if (epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &event) < 0) {
			fprintf(stderr, "error adding event\n");
			exit(1);
		}
	}



}


void readRequest(struct stateSaver *ea) {

    int len;
	char buf[MAXLINE];

    while ((len = recv(ea->clientFd, buf, MAXLINE, 0)) > 0) {
		printf("Received %d bytes\n", len);

        int i;
        for(i = 0; i < len; i++) {
            ea->buf[ea->bufLen+i] = buf[i];
        }
        ea->bufLen += len;

		//send(connfd, buf, len, 0);
	}
    if(len == 0) {
        //check if EOF
        if(ea->buf[ea->bufLen] == '\n' && ea->buf[ea->bufLen - 1] == '\r' && ea->buf[ea->bufLen - 2] == '\n' && ea->buf[ea->bufLen - 3] == '\r') {//EOF
            //EOF

            char type[MAXLINE], uri[MAXLINE], version[MAXLINE];
            sscanf(ea->buf, "%s %s %s", type, uri, version);
            char* request = strrchr(uri + 7, '/');

            printf("%s", type);


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

                    //rio_writen(connfd, temp->response, temp->maxSize);
                    ea->state = 4;
                    for(i = 0; i < temp->maxSize; i++) {
                        ea->fullResp[i] = temp->response[i];
                    }
                    return;
                }
            }
            //otherwise, not cached, do it yourself

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


            ea->sendingFd = open_clientfd(host, port);

            strcat(ea->requestString, "GET ");
            strcat(ea->requestString, request);
            strcat(ea->requestString, " HTTP/1.0\r\n");
            strcat(ea->requestString, "Host: ");
            strcat(ea->requestString, host);
            strcat(ea->requestString, "\r\n");
            strcat(ea->requestString, user_agent_hdr);
            strcat(ea->requestString, static_headers);

            ea->state = 2;
        }
        return;
    } else if(errno == EWOULDBLOCK || errno == EAGAIN) {
        return; //no more data
    }
    return;


}


void sendRequest(struct stateSaver *ea) {

printf("made it somehow");
}


void readResponse(struct stateSaver *ea) {

}


void sendResponse(struct stateSaver *ea) {

}








/*
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
        //TODO: first, wait for request permissions

        request_response* temp = &cache_data[i];


        if(temp == NULL) {
            break;
        }

        if(strcmp(temp->request, uri) == 0) {
            //the request matches the cache, return the cached data
            //TODO: wait for response data

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
    rio_writen(connfd, fullResponse, totalSize);

}*/


void logURI(char* uri) {

    fputs(uri, fp);
    fputs("\n", fp);

}
