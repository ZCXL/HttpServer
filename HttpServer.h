/**********************************************/
/* This class is used to get client's request */
/* ,process request,and response              */
/* Author:zhuchao 2015-11-3                   */
/**********************************************/
#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_
#include <arpa/inet.h>
#include <sys/socket.h>
#include "HttpResponse.h"
#include "HttpRequest.h"

/* buffer size */
#define BUF_SIZE 512
class HttpServer{
	public:
		HttpServer();
		HttpServer(int);
		~HttpServer();
		
		int run(void);
		int setPort(size_t);
		int initSocket(void);
		
		int handleRequest(void);
		int recvRequest(void);
		int parseRequest(void);
		int processRequest(void);
		int prepareResponse(void);
		int sendResponse(void);
	private:
		string getMimeType(string);
		
		size_t svrPort;
		int sockfd,newsockfd;
		socklen_t cliLen;
		struct sockaddr_in servAddr,cliAddr;
		
		string m_url;
		string m_mimeType;
		HttpRequest* httpRequest;
		HttpResponse* httpResponse;
};
#endif