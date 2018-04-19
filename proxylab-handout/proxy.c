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

void initialize();





int efd;


FILE *fp;



/* You won't lose style points for including this long line in your code */
//static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char **argv)
{
    int listenfd, connfd;
    struct sockaddr_storage clientaddr;
    socklen_t clientlen;
    fp = fopen("log.txt", "w");

    while(1) {

        listenfd = Open_listenfd(argv[1]);

        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);


        char type[MAXLINE], uri[MAXLINE], version[MAXLINE];
        char* input = malloc(MAX_OBJECT_SIZE);

        rio_t rio;
        Rio_readinitb(&rio, connfd);
        if(!Rio_readlineb(&rio, input, MAXLINE))
            return 0;

        sscanf(input, "%s %s %s", type, uri, version);

        char* request = strrchr(uri + 7, '/');


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

        logURI(finalRequest);

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



    fclose(fp);
    return 0;
}




void logURI(char* uri) {

    fputs(uri, fp);
    fputs("\n", fp);

}
