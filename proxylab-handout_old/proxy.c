#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "csapp.h"
#include "sbuf.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define NTHREADS  80
#define SBUFSIZE  20

void *thread(void *vargp);
void threadHandleStuff(int connfd);

sbuf_t sbuf;

typedef struct {
    char* request;
    char* response;
    int maxSize;
} request_response;

typedef struct {
    sem_t request;
    sem_t response;
} cache_mutex;

int currentAddition;
sem_t cache_addition;


/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

static const char *static_headers = "Connection: close\r\nProxy-Connection: close\r\n\r\n";


sem_t request_locks[10];
sem_t response_locks[10];
request_response cache_data[10];


int main(int argc, char * argv[])
{
    int i, listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    sem_t * temp;
    sem_t *temp2;
    Sem_init(&cache_addition, 0, 1);
    for(i = 0; i < 10; i++) {
        temp = &request_locks[i];
        temp2 = &response_locks[i];
        Sem_init(temp, 0, 1);     //parameters of this might be different
        Sem_init(temp2, 0, 1);
    }

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    listenfd = Open_listenfd(argv[1]);

    sbuf_init(&sbuf, SBUFSIZE); //line:conc:pre:initsbuf
    for (i = 0; i < NTHREADS; i++)  /* Create worker threads */ //line:conc:pre:begincreate
	   Pthread_create(&tid, NULL, thread, NULL);               //line:conc:pre:endcreate

    while (1) {
       clientlen = sizeof(struct sockaddr_storage);
	   connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
	   sbuf_insert(&sbuf, connfd); /* Insert connfd in buffer */
    }
}

void *thread(void *vargp)
{
    Pthread_detach(pthread_self());
    while (1) {
	int connfd = sbuf_remove(&sbuf); /* Remove connfd from buffer */ //line:conc:pre:removeconnfd
	threadHandleStuff(connfd);                /* Service client */
	Close(connfd);
    }
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
            V(&request_locks[i]);
            break;
        }
        //TODO: first, wait for request permissions

        P(&request_locks[i]);
        request_response* temp = &cache_data[i];


        if(temp == NULL) {
            V(&request_locks[i]);
            break;
        }

        if(strcmp(temp->request, uri) == 0) {
            //the request matches the cache, return the cached data
            //TODO: wait for response data

            V(&request_locks[i]);
            P(&response_locks[i]);
            rio_writen(connfd, temp->response, temp->maxSize);
            printf("cached response: %s\n", uri);
            V(&response_locks[i]);
            return;
        }
        V(&request_locks[i]);
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


    P(&cache_addition);
    P(&request_locks[currentAddition]);
    P(&response_locks[currentAddition]);

    request_response * temp3 = &cache_data[currentAddition];
    temp3->request = uri;
    temp3->response = fullResponse;
    temp3->maxSize = totalSize;
    V(&response_locks[currentAddition]);
    V(&response_locks[currentAddition]);
    currentAddition++;
    if(currentAddition > 9) {
        currentAddition = 0;
    }
    V(&cache_addition);


    /*int i;
    for(i = 0; i < totalSize; i++) {
        //printf("%c", fullResponse[i]);
    }*/

    rio_writen(connfd, fullResponse, totalSize);

}
