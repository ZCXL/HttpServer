/* HttpServer.cpp */

#include<iostream>
#include<fstream>
#include<string>
#include<cstring>
#include<stdexcept>
#include<sstream>
#include<algorithm>
#include<ctime>

#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<sys/socket.h>

#include"HttpServer.h"
#include"Config.h"

extern int errno;

using namespace std;

HttpServer::HttpServer(): svrPort(Port)
{
}

HttpServer::HttpServer(int port)
{
	string funcName = "HttpServer::HttpServer: ";

	if(setPort(port)){
		cerr<<funcName<<"Failed to set port"<<endl;
	}
}

HttpServer::~HttpServer()
{
	close(newsockfd);
	close(sockfd);
}

int HttpServer::setPort(size_t port)
{
	string funcName = "setPort: ";
	//Validation
	if(port<1024||port>65535){
		cerr<<funcName<<"Invalid port value. Cannot bind. Enter a value between 1024 and 65535"<<endl;
		return -1;
	}

	svrPort = port;
	
	return 0;
}

int HttpServer::initSocket()
{
	string funcName = "initSocket: ";

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0){
		cerr<<funcName<<"Failed to create socket"<<endl;
		return -1;
	}

	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(svrPort);
	servAddr.sin_addr.s_addr = INADDR_ANY;

	/* Bind */
	if((bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)))<0){
		cerr<<funcName<<"Failed to bind to port "<<svrPort<<endl;
		return -1;
	}

	/* Set to listen on control socket */
	if(listen(sockfd, BackLog)){
		cerr<<funcName<<"Listen on port "<<svrPort<<" failed"<<endl;
		return -1;
	}

	return 0;
}

int HttpServer::run()
{
	cout<<"Server is Running"<<endl;
	string funcName = "run: ";

	if(initSocket()){
		cerr<<funcName<<"Failed to initialize socket"<<endl;
		return -1;
	}

	while(1){
		cliLen = sizeof(cliAddr);

		if((newsockfd = accept(sockfd, (struct sockaddr *)&cliAddr, &cliLen))<0){
			cerr<<funcName<<"Accept call failed"<<endl;
			return -1;
		}

		if(fork() == 0){
			if(handleRequest()){
				cerr<<funcName<<"Failed handling request"<<endl;
				exit(-1);
			}

			exit(0);
		}
		close(newsockfd);
	}

	return 0;
}

int HttpServer::handleRequest()
{
	string funcName = "handleRequest: ";

	httpRequest = new HttpRequest();
	httpResponse = new HttpResponse();
	 
	if(recvRequest()){
		cerr<<funcName<<"Receiving request failed"<<endl;
		return -1;
	}

	httpRequest->printRequest();

	if(parseRequest()){
		cerr<<funcName<<"Parsing HTTP Request failed"<<endl;
		return -1;
	}

	if(processRequest()){
		cerr<<funcName<<"Processing HTTP Request failed"<<endl;
		return -1;
	}

	if(prepareResponse()){
		cerr<<funcName<<"Preparing reply failed"<<endl;
		return -1;
	}

	httpResponse->printResponse();

	if(sendResponse()){
		cerr<<funcName<<"Sending reply failed"<<endl;
		return -1;
	}
	
	delete httpRequest;
	delete httpResponse;
	return 0;
}

int HttpServer::recvRequest()
{
	string funcName = "recvRequest: ";
	int recvLength;
	char* buf = new char[BUF_SIZE];
	memset(buf, '\0', BUF_SIZE);

	if(!(recvLength = recv(newsockfd, buf, BUF_SIZE, 0))){
		cerr<<funcName<<"Failed to receive request (blocking)"<<endl;
		return -1;
	}
	httpRequest->addData(buf, recvLength);

	while(1){
		memset(buf, '\0', BUF_SIZE);
		recvLength = recv(newsockfd, buf, BUF_SIZE, MSG_DONTWAIT);

		if(recvLength < 0){
			if(errno == EWOULDBLOCK || errno == EAGAIN){
				break;
			} else {
				cerr<<funcName<<"Failed receiving request (nonblocking)"<<endl;
				return -1;
			}
		}
	
		httpRequest->addData(buf, recvLength);

		if(recvLength<BUF_SIZE)
			break;
	}

	return 0;
}

int HttpServer::parseRequest()
{
	string funcName = "parseRequest: ";

	if(httpRequest->parseRequest()){
		cerr<<funcName<<"Failed parsing request"<<endl;
		return -1;
	}

	return 0;
}

int HttpServer::processRequest()
{
	string funcName = "processRequest: ";
	Method method = httpRequest->getMethod();

	ifstream ifs, errfs;
	ofstream ofs;
	size_t contentLength;
	ostringstream os;

	if(httpRequest->getProtocol() == HTTP_UNSUPPORTED){
		httpResponse->setStatusCode(505);
		return 0;
	}

	switch(method){
		case GET:
			m_url = SVR_ROOT + httpRequest->getURL();
			m_mimeType = getMimeType(m_url);

			ifs.open(m_url.c_str(), ifstream::in);
			if(ifs.is_open()){
				ifs.seekg(0, ifstream::end);
				contentLength = ifs.tellg();
				ifs.seekg(0, ifstream::beg);
				os<<contentLength;

				if(httpResponse->copyFromFile(ifs, contentLength)){
					cerr<<funcName<<"Failed to copy file to Response Body"<<endl;
					httpResponse->setStatusCode(500);
					return 0;
				}

				httpResponse->setHTTPHeader("Content-Length", os.str());
			}else{
				ifs.close();

				string file404 = SVR_ROOT;
				file404 += "/404.html";
				errfs.open(file404.c_str(), ifstream::in);
				if(errfs.is_open()){
					errfs.seekg(0, ifstream::end);
					contentLength = errfs.tellg();
					errfs.seekg(0, ifstream::beg);
					os<<contentLength;

					if(httpResponse->copyFromFile(errfs, contentLength)){
						cerr<<funcName<<"Failed to copy file to Response Body"<<endl;
						httpResponse->setStatusCode(500);
						return 0;
					}

					httpResponse->setHTTPHeader("Content-Length", os.str());
					httpResponse->setStatusCode(404);
					return 0;
				}else{
					cerr<<"Critical error. Shutting down"<<endl;
					return -1;
				}
			}

			ifs.close();
			httpResponse->setStatusCode(200);
			break;
		case PUT:
			m_url = SVR_ROOT + httpRequest->getURL();
			m_mimeType = getMimeType(m_url);

			ofs.open(m_url.c_str(), ofstream::out|ofstream::trunc);

			if(ofs.is_open()){
				if(httpRequest->copyToFile(ofs))
					httpResponse->setStatusCode(411);
				else
					httpResponse->setStatusCode(201);
			}
			else{
				httpResponse->setStatusCode(403);
			}
			ofs.close();

			break;
		default:
			httpResponse->setStatusCode(501);
			break;
	}

	return 0;
}

int HttpServer::prepareResponse()
{
	string funcName = "prepareResponse: ";
	time_t curTime;
	time(&curTime);
	string curTimeStr = ctime(&curTime);
	replace(curTimeStr.begin(), curTimeStr.end(), '\n', '\0');

	httpResponse->setProtocol(httpRequest->getProtocol());
	httpResponse->setReasonPhrase();

	httpResponse->setHTTPHeader("Date", curTimeStr);
	httpResponse->setHTTPHeader("Server", "Zhuchao's HTTP Server");
	httpResponse->setHTTPHeader("Accept-Ranges", "bytes");
	httpResponse->setHTTPHeader("Content-Type", m_mimeType);
	httpResponse->setHTTPHeader("Connection", "close");

	if(httpResponse->prepareResponse()){
		cerr<<funcName<<"Failed to prepare response"<<endl;
		return -1;
	}

	return 0;
}

int HttpServer::sendResponse()
{
	string funcName = "sendResponse: ";

	size_t responseSize = httpResponse->getResponseSize();
	string* responseDataPtr = httpResponse->getResponseDataPtr();

	char *buf = new char[responseSize];
	memset(buf, '\0', responseSize);
	memcpy(buf, responseDataPtr->data(), responseSize);

	if((send(newsockfd, buf, responseSize, 0))<0){
		cerr<<funcName<<"Sending response failed"<<endl;
	}

	delete buf;

	return 0;
}

string HttpServer::getMimeType(string fileName)
{
	size_t extPos = fileName.find_last_of(".");
	string extension;
	string mimeType = "text/plain, charset=us-ascii";

	if(extPos == string::npos){
		extension = "";
	}else{
		extension = fileName.substr(extPos+1);
	}

	/* Compare and return mimetype */
	switch(extension[0]){
		case 'b':
			if(extension == "bmp")
				mimeType = "image/bmp";
			if(extension == "bin")
				mimeType = "application/octet-stream";

			break;
		case 'c':
			if(extension == "csh")
				mimeType = "application/csh";
			if(extension == "css")
				mimeType = "text/css";

			break;
		case 'd':
			if(extension == "doc")
				mimeType = "application/msword";
			if(extension == "dtd")
				mimeType = "application/xml-dtd";
			break;
		case 'e':
			if(extension == "exe")
				mimeType = "application/octet-stream";
			break;
		case 'h':
			if(extension == "html" || extension == "htm")
				mimeType = "text/html";
			break;
		case 'i':
			if(extension == "ico")
				mimeType = "image/x-icon";
			break;
		case 'g':
			if(extension == "gif")
				mimeType = "image/gif";
			break;
		case 'j':
			if(extension == "jpeg" || extension == "jpg")
				mimeType = "image/jpeg";
			break;
		case 'l':
			if(extension == "latex")
				mimeType = "application/x-latex";
			break;
		case 'p':
			if(extension == "png")
				mimeType = "image/png";
			if(extension == "pgm")
				mimeType = "image/x-portable-graymap";
			break;	
		case 'r':
			if(extension == "rtf")
				mimeType  = "text/rtf";
			break;
		case 's':
			if(extension == "svg")
				mimeType = "image/svg+xml";
			if(extension == "sh")
				mimeType = "application/x-sh";
			break;
		case 't':
			if(extension == "tar")
				mimeType = "application/x-tar";
			if(extension == "tex")
				mimeType = "application/x-tex";
			if(extension == "tif" || extension == "tiff")
				mimeType = "image/tiff";
			if(extension == "txt")
				mimeType = "text/plain";
			break;
		case 'x':
			if(extension == "xml")
				mimeType = "application/xml";
			break;
		default:
			break;
	}

	return mimeType;
}